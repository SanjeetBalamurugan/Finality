#include "ExecutableResources.h"
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#define UNICODE
#define NOMINMAX
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace
{
    using Bytes = std::vector<uint8_t>;
    using Microsoft::WRL::ComPtr;

    void Check(HRESULT result)
    {
        if (FAILED(result))
        {
            throw std::runtime_error("Cannot read or encode player icon (HRESULT " + std::to_string(result) + ")");
        }
    }

    std::wstring Wide(const std::string& text)
    {
        if (text.size() > 2048 || text.find('\0') != std::string::npos)
        {
            throw std::runtime_error("Executable metadata is too long or contains a null character");
        }
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (!count && !text.empty())
        {
            throw std::runtime_error("Executable metadata is not valid UTF-8");
        }
        std::wstring value(count, L'\0');
        if (count)
        {
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), value.data(), count);
        }
        return value;
    }

    void Word(Bytes& out, uint16_t v)
    {
        out.push_back(uint8_t(v));
        out.push_back(uint8_t(v >> 8));
    }

    void Dword(Bytes& out, uint32_t v)
    {
        Word(out, uint16_t(v));
        Word(out, uint16_t(v >> 16));
    }

    void Text(Bytes& out, const std::wstring& text)
    {
        for (const auto c : text)
        {
            Word(out, uint16_t(c));
        }
        Word(out, 0);
    }

    void Align(Bytes& out)
    {
        while (out.size() % 4)
        {
            out.push_back(0);
        }
    }

    size_t Block(Bytes& out, const std::wstring& key, uint16_t valueLength, uint16_t type)
    {
        Align(out);
        const auto start = out.size();
        Word(out, 0);
        Word(out, valueLength);
        Word(out, type);
        Text(out, key);
        Align(out);
        return start;
    }

    void End(Bytes& out, size_t start)
    {
        if (out.size() - start > 65535)
        {
            throw std::runtime_error("Executable version metadata exceeds Windows limits");
        }
        const auto length = static_cast<uint16_t>(out.size() - start);
        out[start] = uint8_t(length);
        out[start + 1] = uint8_t(length >> 8);
    }

    Bytes VersionInfo(const YAML::Node& request)
    {
        const auto version = request["Version"].as<std::string>("0.1.0");
        std::array<uint16_t, 4> numbers{};
        std::istringstream stream(version);
        std::string part;
        size_t count = 0;
        while (std::getline(stream, part, '.'))
        {
            if (count == 4 || part.empty() || part.size() > 5 || part.find_first_not_of("0123456789") != std::string::npos || std::stoul(part) > 65535)
            {
                throw std::runtime_error("Version must be numeric major.minor.patch[.build], each part from 0 to 65535");
            }
            numbers[count++] = static_cast<uint16_t>(std::stoul(part));
        }
        if (count < 3 || version.ends_with('.'))
        {
            throw std::runtime_error("Version must be major.minor.patch[.build]");
        }
        VS_FIXEDFILEINFO info{};
        info.dwSignature = 0xfeef04bd;
        info.dwStrucVersion = 0x10000;
        info.dwFileVersionMS = (uint32_t(numbers[0]) << 16) | numbers[1];
        info.dwFileVersionLS = (uint32_t(numbers[2]) << 16) | numbers[3];
        info.dwProductVersionMS = info.dwFileVersionMS;
        info.dwProductVersionLS = info.dwFileVersionLS;
        info.dwFileFlagsMask = VS_FFI_FILEFLAGSMASK;
        info.dwFileOS = VOS_NT_WINDOWS32;
        info.dwFileType = VFT_APP;
        Bytes out;
        const auto root = Block(out, L"VS_VERSION_INFO", sizeof(info), 0);
        const auto* raw = reinterpret_cast<const uint8_t*>(&info);
        out.insert(out.end(), raw, raw + sizeof(info));
        const auto strings = Block(out, L"StringFileInfo", 0, 1), table = Block(out, L"040904b0", 0, 1);
        const auto product = request["ProductName"].as<std::string>("Finality Game");
        const std::pair<std::wstring, std::string> versionProperties[] =
        {
            { L"CompanyName", request["CompanyName"].as<std::string>("DefaultCompany") },
            { L"ProductName", product },
            { L"FileDescription", product },
            { L"ProductVersion", version },
            { L"FileVersion", version },
            { L"InternalName", "Game.exe" },
            { L"OriginalFilename", "Game.exe" }
        };

        for (const auto& [key, value] : versionProperties)
        {
            const auto text = Wide(value);
            const auto start = Block(out, key, static_cast<uint16_t>(text.size() + 1), 1);
            Text(out, text);
            End(out, start);
        }
        End(out, table);
        End(out, strings);
        const auto vars = Block(out, L"VarFileInfo", 0, 1), translation = Block(out, L"Translation", 4, 0);
        Word(out, 0x0409);
        Word(out, 1200);
        End(out, translation);
        End(out, vars);
        End(out, root);
        return out;
    }

    std::filesystem::path IconPath(const std::filesystem::path& project, const std::string& input)
    {
        const auto path = std::filesystem::weakly_canonical(project / std::filesystem::u8path(input));
        const auto relative = path.lexically_relative(std::filesystem::weakly_canonical(project / "Assets"));
        if (relative.empty() || *relative.begin() == ".." || !std::filesystem::is_regular_file(path))
        {
            throw std::runtime_error("Player icon must reference an existing image inside project Assets: " + input);
        }
        return path;
    }

    Bytes IconPNG(IWICImagingFactory* factory, const std::filesystem::path& path, UINT size, bool exact)
    {
        ComPtr<IWICBitmapDecoder> decoder;
        Check(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder));
        UINT count = 0, best = 0, bestArea = 0;
        Check(decoder->GetFrameCount(&count));
        if (!count)
        {
            throw std::runtime_error("Player icon contains no image frames");
        }
        for (UINT i = 0; i < count; ++i)
        {
            ComPtr<IWICBitmapFrameDecode> candidate;
            Check(decoder->GetFrame(i, &candidate));
            UINT w, h;
            Check(candidate->GetSize(&w, &h));
            if (w <= 4096 && h <= 4096 && w * h > bestArea)
            {
                best = i;
                bestArea = w * h;
            }
        }
        ComPtr<IWICBitmapFrameDecode> frame;
        Check(decoder->GetFrame(best, &frame));
        UINT width, height;
        Check(frame->GetSize(&width, &height));
        if (width != height || width == 0 || width > 4096 || (exact && width != size))
        {
            throw std::runtime_error("Player icon must be square and match its assigned size (" + std::to_string(size) + " x " + std::to_string(size) + "): " + path.string());
        }
        ComPtr<IWICBitmapScaler> scaled;
        Check(factory->CreateBitmapScaler(&scaled));
        Check(scaled->Initialize(frame.Get(), size, size, WICBitmapInterpolationModeFant));
        ComPtr<IWICFormatConverter> converted;
        Check(factory->CreateFormatConverter(&converted));
        Check(converted->Initialize(scaled.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
        ComPtr<IStream> stream;
        Check(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
        ComPtr<IWICBitmapEncoder> encoder;
        Check(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
        Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
        ComPtr<IWICBitmapFrameEncode> output;
        Check(encoder->CreateNewFrame(&output, nullptr));
        Check(output->Initialize(nullptr));
        Check(output->SetSize(size, size));
        auto format = GUID_WICPixelFormat32bppBGRA;
        Check(output->SetPixelFormat(&format));
        Check(output->WriteSource(converted.Get(), nullptr));
        Check(output->Commit());
        Check(encoder->Commit());
        STATSTG stats{};
        Check(stream->Stat(&stats, STATFLAG_NONAME));
        if (stats.cbSize.QuadPart > 4 * 1024 * 1024)
        {
            throw std::runtime_error("Encoded player icon is too large");
        }
        HGLOBAL memory;
        Check(GetHGlobalFromStream(stream.Get(), &memory));
        auto* bytes = static_cast<const uint8_t*>(GlobalLock(memory));
        if (!bytes)
        {
            throw std::runtime_error("Cannot access encoded icon data");
        }
        Bytes result(bytes, bytes + static_cast<size_t>(stats.cbSize.QuadPart));
        GlobalUnlock(memory);
        return result;
    }
} // namespace
#endif

void BakeExecutableResources(const std::filesystem::path& executable, const std::filesystem::path& project,
                             const std::filesystem::path& bin, const YAML::Node& request,
                             const FINALITY::PlayerRuntime& runtime)
{
#ifdef _WIN32
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Check(initialized);

    struct Apartment
    {
        ~Apartment()
        {
            CoUninitialize();
        }
    } apartment;

    ComPtr<IWICImagingFactory> factory;
    Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
    const auto defaultInput = request["DefaultIcon"].as<std::string>("");
    const auto fallback = defaultInput.empty() ? bin / "assets/windows/favicon.ico" : IconPath(project, defaultInput);
    const std::array<UINT, 7> sizes{ 16, 24, 32, 48, 64, 128, 256 };
    std::vector<Bytes> images;
    for (const auto size : sizes)
    {
        const auto key = std::to_string(size);
        const auto input = request["Icons"] ? request["Icons"][key].as<std::string>("") : "";
        images.push_back(IconPNG(factory.Get(), input.empty() ? fallback : IconPath(project, input), size, !input.empty()));
    }
    Bytes ico, group;
    Word(ico, 0);
    Word(ico, 1);
    Word(ico, static_cast<uint16_t>(sizes.size()));
    Word(group, 0);
    Word(group, 1);
    Word(group, static_cast<uint16_t>(sizes.size()));
    uint32_t offset = 6 + static_cast<uint32_t>(sizes.size()) * 16;
    for (size_t i = 0; i < sizes.size(); ++i)
    {
        Bytes* const iconOutputs[] =
        {
            &ico,
            &group
        };

        for (auto* out : iconOutputs)
        {
            out->push_back(sizes[i] == 256 ? 0 : uint8_t(sizes[i]));
            out->push_back(sizes[i] == 256 ? 0 : uint8_t(sizes[i]));
            Word(*out, 0);
            Word(*out, 1);
            Word(*out, 32);
            Dword(*out, static_cast<uint32_t>(images[i].size()));
        }
        Dword(ico, offset);
        offset += static_cast<uint32_t>(images[i].size());
        Word(group, static_cast<uint16_t>(i + 1));
    }
    for (const auto& image : images)
    {
        ico.insert(ico.end(), image.begin(), image.end());
    }
    const auto iconFile = project / ".Finality/Build/Game.ico";
    std::filesystem::create_directories(iconFile.parent_path());
    std::ofstream iconStream(iconFile, std::ios::binary);
    iconStream.write(reinterpret_cast<const char*>(ico.data()), ico.size());
    if (!iconStream)
    {
        throw std::runtime_error("Cannot write generated Game.ico");
    }
    auto version = VersionInfo(request), configuration = FINALITY::EncodePlayerRuntime(runtime);
    HANDLE update = BeginUpdateResourceW(executable.c_str(), FALSE);
    if (!update)
    {
        throw std::runtime_error("Cannot update game executable resources");
    }
    auto put = [&](LPCWSTR type, int id, Bytes& data)
    {
        if (!UpdateResourceW(update, type, MAKEINTRESOURCEW(id), 0x0409, data.data(), static_cast<DWORD>(data.size())))
        {
            throw std::runtime_error("Cannot write game executable resource (Windows error " + std::to_string(GetLastError()) + ")");
        }
    };
    try
    {
        for (size_t i = 0; i < images.size(); ++i)
        {
            put(RT_ICON, static_cast<int>(i + 1), images[i]);
        }
        put(RT_GROUP_ICON, 1, group);
        put(RT_VERSION, 1, version);
        put(RT_RCDATA, FINALITY::PlayerRuntimeResource, configuration);
    }
    catch (...)
    {
        EndUpdateResourceW(update, TRUE);
        throw;
    }
    if (!EndUpdateResourceW(update, FALSE))
    {
        throw std::runtime_error("Cannot finalize game executable resources");
    }
#else
    throw std::runtime_error("Executable resource baking is currently supported for Windows exports");
#endif
}
