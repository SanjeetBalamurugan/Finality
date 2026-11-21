#include "ReflectionCache.h"
#include <algorithm>
#include <clang-c/Index.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace fs = std::filesystem;
std::string str(CXString s)
{
    std::string r = clang_getCString(s) ? clang_getCString(s) : "";
    clang_disposeString(s);
    return r;
}
std::vector<CXCursor> children(CXCursor c)
{
    std::vector<CXCursor> r;
    clang_visitChildren(
        c,
        [](CXCursor c, CXCursor, CXClientData d) {
            static_cast<std::vector<CXCursor> *>(d)->push_back(c);
            return CXChildVisit_Continue;
        },
        &r);
    return r;
}
std::string name(CXCursor c)
{
    return str(clang_getCursorSpelling(c));
}
std::string usr(CXCursor c)
{
    return str(clang_getCursorUSR(c));
}
std::string qualified(CXCursor c)
{
    std::string r;
    while (!clang_Cursor_isNull(c) && clang_getCursorKind(c) != CXCursor_TranslationUnit)
    {
        auto n = name(c);
        if (!n.empty())
            r = "::" + n + r;
        c = clang_getCursorSemanticParent(c);
    }
    return r;
}
fs::path file(CXCursor c)
{
    CXFile f{};
    clang_getSpellingLocation(clang_getCursorLocation(c), &f, nullptr, nullptr, nullptr);
    return f ? fs::weakly_canonical(str(clang_getFileName(f))) : fs::path();
}
bool record(CXCursor c)
{
    auto k = clang_getCursorKind(c);
    return k == CXCursor_ClassDecl || k == CXCursor_StructDecl;
}
std::string quote(const std::string &s)
{
    std::ostringstream o;
    o << std::quoted(s);
    return o.str();
}
struct Parser
{
    fs::path source, engine, output;
    std::vector<std::string> flags;
    std::set<std::string> included, supported;
    CXIndex index = clang_createIndex(0, 0);
    ReflectionCache cache;
    size_t reused = 0;
    struct Unit
    {
        fs::path path;
        CXTranslationUnit tu{};
        std::vector<CXCursor> definitions, scripts;
        bool legacy = false;
        std::optional<CachedReflection> cached;
        std::vector<fs::path> dependencies;
        fs::file_time_type started;
    };
    std::vector<Unit> units;
    ~Parser()
    {
        for (auto &u : units)
            if (u.tu)
                clang_disposeTranslationUnit(u.tu);
        clang_disposeIndex(index);
    }
    bool project(CXCursor c)
    {
        auto p = file(c);
        if (p.empty())
            return false;
        auto r = p.lexically_relative(source);
        return !r.empty() && *r.begin() != "..";
    }
    std::vector<CXCursor> bases(CXCursor c)
    {
        std::vector<CXCursor> r;
        for (auto b : children(c))
            if (clang_getCursorKind(b) == CXCursor_CXXBaseSpecifier &&
                clang_getCXXAccessSpecifier(b) == CX_CXXPublic)
            {
                auto d = clang_getTypeDeclaration(clang_getCursorType(b));
                if (project(d))
                    r.push_back(d);
            }
        return r;
    }
    bool script(CXCursor c, std::set<std::string> seen = {})
    {
        if (qualified(c) == "::FINALITY::ScriptableComponent")
            return true;
        if (!seen.insert(usr(c)).second)
            return false;
        for (auto b : children(c))
            if (clang_getCursorKind(b) == CXCursor_CXXBaseSpecifier &&
                script(clang_getTypeDeclaration(clang_getCursorType(b)), seen))
                return true;
        return false;
    }
    std::vector<CXCursor> fields(CXCursor c)
    {
        std::vector<CXCursor> r;
        for (auto f : children(c))
        {
            if (clang_getCursorKind(f) != CXCursor_FieldDecl)
                continue;
            bool annotated = false;
            for (auto a : children(f))
                if (clang_getCursorKind(a) == CXCursor_AnnotateAttr && name(a) == "finality.serialize")
                    annotated = true;
            auto t = clang_getCursorType(f);
            if ((clang_getCXXAccessSpecifier(f) != CX_CXXPublic && !annotated) ||
                clang_isConstQualifiedType(t))
                continue;
            if (clang_Cursor_isBitField(f) || t.kind == CXType_LValueReference ||
                t.kind == CXType_RValueReference)
                throw std::runtime_error("Unsupported reference or bitfield: " + qualified(f));
            r.push_back(f);
        }
        return r;
    }
    void collect(CXCursor c, std::map<std::string, CXCursor> &needed, std::set<std::string> stack = {})
    {
        auto key = usr(c);
        if (stack.count(key))
            throw std::runtime_error("Recursive Inspector struct: " + qualified(c));
        if (needed.count(key))
            return;
        stack.insert(key);
        for (auto b : bases(c))
            collect(b, needed, stack);
        for (auto f : fields(c))
        {
            auto t = clang_getCanonicalType(clang_getCursorType(f));
            auto s = str(clang_getTypeSpelling(t));
            if (s == "bool" || s == "int" || s == "float" || supported.count(s))
                continue;
            auto d = clang_getTypeDeclaration(t);
            if (record(d) && project(d))
                collect(d, needed, stack);
            else
                throw std::runtime_error("Unsupported Inspector field " + qualified(f) + ": " + s);
        }
        needed[key] = c;
    }
    void walk(CXCursor c, std::vector<CXCursor> &result)
    {
        for (auto d : children(c))
            if (project(d))
            {
                result.push_back(d);
                auto k = clang_getCursorKind(d);
                if (record(d) || k == CXCursor_Namespace || k == CXCursor_LinkageSpec ||
                    k == CXCursor_UnexposedDecl)
                    walk(d, result);
            }
    }
    void parse(fs::path path)
    {
        if (auto cached = cache.Read(path))
        {
            units.push_back({path});
            units.back().legacy = cached->Legacy;
            for (const auto &dependency : cached->Dependencies)
                included.insert(dependency.generic_string());
            units.back().cached = std::move(cached);
            ++reused;
            return;
        }
        const auto started = fs::file_time_type::clock::now();
        std::ifstream in(path, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), {});
        content += "\nstruct FinalityReflectionBuiltinTypes { std::string String; glm::vec3 Vector; "
                   "FINALITY::ScriptAssetReference Asset; };\n";
        auto filename = path.string();
        CXUnsavedFile unsaved{filename.c_str(), content.c_str(), static_cast<unsigned long>(content.size())};
        std::vector<const char *> args;
        for (auto &a : flags)
            args.push_back(a.c_str());
        CXTranslationUnit tu{};
        if (clang_parseTranslationUnit2(index, filename.c_str(), args.data(), int(args.size()), &unsaved, 1,
                                        CXTranslationUnit_SkipFunctionBodies, &tu) != CXError_Success)
            throw std::runtime_error("Clang could not parse " + filename);
        units.push_back({path, tu});
        auto &unit = units.back();
        unit.started = started;
        bool errors = false;
        for (unsigned i = 0; i < clang_getNumDiagnostics(tu); ++i)
        {
            auto d = clang_getDiagnostic(tu, i);
            if (clang_getDiagnosticSeverity(d) >= CXDiagnostic_Error)
            {
                std::cerr << str(clang_formatDiagnostic(d, clang_defaultDiagnosticDisplayOptions())) << '\n';
                errors = true;
            }
            clang_disposeDiagnostic(d);
        }
        if (errors)
            throw std::runtime_error("Reflection failed; generated files were not changed");
        clang_getInclusions(
            tu,
            [](CXFile f, CXSourceLocation *, unsigned, CXClientData data) {
                auto *parser = static_cast<Parser *>(data);
                const auto dependency = fs::weakly_canonical(str(clang_getFileName(f)));
                parser->included.insert(dependency.generic_string());
                parser->units.back().dependencies.push_back(dependency);
            },
            this);
        std::vector<CXCursor> cursors;
        walk(clang_getTranslationUnitCursor(tu), cursors);
        for (auto c : cursors)
            if (name(c) == "FinalityReflectionBuiltinTypes")
                for (auto f : children(c))
                    if (clang_getCursorKind(f) == CXCursor_FieldDecl)
                        supported.insert(
                            str(clang_getTypeSpelling(clang_getCanonicalType(clang_getCursorType(f)))));
        std::map<std::string, CXCursor> needed;
        for (auto c : cursors)
        {
            if (record(c) && clang_isCursorDefinition(c) && script(c) && !clang_CXXRecord_isAbstract(c))
            {
                unit.scripts.push_back(c);
                collect(c, needed);
            }
            if (clang_getCursorKind(c) == CXCursor_FunctionDecl && name(c) == "FinalityRegisterScripts")
                unit.legacy = true;
        }
        for (auto &[key, c] : needed)
            unit.definitions.push_back(c);
    }
    void generate()
    {
        std::map<std::string, std::string> outputs;
        std::vector<std::string> legacy;
        size_t total = 0;
        for (auto &u : units)
        {
            uint64_t hash = 14695981039346656037ull;
            for (unsigned char byte : u.path.lexically_relative(source).generic_string())
                hash = (hash ^ byte) * 1099511628211ull;
            std::ostringstream identifier;
            identifier << std::hex << hash;
            auto key = identifier.str();
            if (u.cached)
            {
                outputs["Unit_" + key + ".cpp"] = u.cached->Text;
                total += u.cached->Scripts;
                if (u.legacy)
                    legacy.push_back("FinalityLegacyRegister_" + key);
                continue;
            }
            std::ostringstream o;
            o << "#include <Scene/ScriptModule.h>\n#define FinalityScriptABI FinalityLegacyABI_" << key
              << "\n#define FinalityRegisterScripts FinalityLegacyRegister_" << key << "\n#include "
              << quote(u.path.generic_string())
              << "\n#undef FinalityScriptABI\n#undef FinalityRegisterScripts\nnamespace FINALITY {\n";
            std::set<std::string> visited;
            std::function<void(CXCursor)> emit = [&](CXCursor c) {
                if (!visited.insert(usr(c)).second)
                    return;
                for (auto b : bases(c))
                    emit(b);
                o << "template<> inline auto ScriptFieldAccess::Members<" << qualified(c)
                  << ">() { return std::tuple_cat(";
                for (auto b : bases(c))
                    o << "ScriptFieldAccess::Members<" << qualified(b) << ">(), ";
                o << "std::make_tuple(";
                bool first = true;
                for (auto f : fields(c))
                {
                    if (!first)
                        o << ", ";
                    first = false;
                    o << "MakeScriptField(" << quote(name(f)) << ", &" << qualified(c) << "::" << name(f)
                      << ")";
                }
                o << ")); }\n";
            };
            for (auto c : u.definitions)
                emit(c);
            o << "}\nnamespace {\n";
            size_t i = 0;
            for (auto c : u.scripts)
            {
                o << "const bool FinalityReflected_" << key << "_" << i++
                  << " = FINALITY::ScriptReflection::Register<" << qualified(c) << ">("
                  << quote(qualified(c).substr(2)) << ", " << quote(file(c).generic_string()) << ");\n";
                ++total;
            }
            o << "}\n";
            if (u.legacy)
                legacy.push_back("FinalityLegacyRegister_" + key);
            outputs["Unit_" + key + ".cpp"] = o.str();
            u.cached = CachedReflection{o.str(), u.scripts.size(), u.legacy, u.dependencies};
            u.cached->Dependencies.push_back(u.path);
            for (const auto &dependency : u.dependencies)
                u.cached->Dependencies.push_back(dependency.parent_path());
            for (const auto &flag : flags)
                if (flag.starts_with("-I") && fs::is_directory(flag.substr(2)))
                    u.cached->Dependencies.emplace_back(flag.substr(2));
            auto &dependencies = u.cached->Dependencies;
            std::sort(dependencies.begin(), dependencies.end());
            dependencies.erase(std::unique(dependencies.begin(), dependencies.end()), dependencies.end());
            for (const auto &dependency : dependencies)
            {
                if (fs::last_write_time(dependency) > u.started)
                    throw std::runtime_error("Reflection inputs changed during parsing; retry the build");
                u.cached->Stamps.push_back(ReflectionCache::Stamp(dependency));
            }
        }
        std::ostringstream r;
        r << "#include <Scene/ScriptModule.h>\n";
        for (auto &f : legacy)
            r << "extern \"C\" bool " << f << "(FINALITY::ScriptRegistrationContext*);\n";
        r << "extern \"C\" __declspec(dllexport) uint64_t FinalityScriptABI() { return "
             "FINALITY::ScriptModuleABI(); }\nextern \"C\" __declspec(dllexport) bool "
             "FinalityRegisterScripts(FINALITY::ScriptRegistrationContext* context) { if (!context) return "
             "false; return FINALITY::ScriptReflection::RegisterModule([](const auto& merge) { using "
             "Callback = std::function<bool(const FINALITY::ScriptDefinition&)>; Callback callback=merge; "
             "FINALITY::ScriptRegistrationContext legacyContext{&callback, [](void* user,const "
             "FINALITY::ScriptDefinition& definition){return "
             "(*static_cast<Callback*>(user))(definition);}};\n";
        for (auto &f : legacy)
            r << "if (!" << f << "(&legacyContext)) return false;\n";
        r << "return true; }, [context](const auto& definition){return context->Add(definition);}); }\n";
        outputs["Registration.cpp"] = r.str();
        fs::create_directories(output);
        for (auto &[filename, text] : outputs)
        {
            auto path = output / filename;
            std::ifstream in(path);
            std::string previous((std::istreambuf_iterator<char>(in)), {});
            if (previous != text)
            {
                std::ofstream out(path, std::ios::binary);
                out << text;
                if (!out)
                    throw std::runtime_error("Cannot write " + path.string());
            }
        }
        for (auto &entry : fs::directory_iterator(output))
            if (entry.path().extension() == ".cpp" && !outputs.count(entry.path().filename().string()))
                fs::remove(entry.path());
        for (const auto &unit : units)
            if (unit.tu)
                cache.Write(unit.path, *unit.cached);
        std::cout << "Reflected " << total << " scripts in " << units.size() << " translation units ("
                  << reused << " cached, " << units.size() - reused << " parsed).\n";
    }
};
int main(int argc, char **argv)
{
    try
    {
        if (argc == 2 && std::string(argv[1]) == "--version")
        {
            std::cout << "FIScriptReflect 1 / " << str(clang_getClangVersion()) << '\n';
            return 0;
        }
        std::map<std::string, std::string> args;
        for (int i = 1; i < argc; i += 2)
        {
            if (i + 1 >= argc)
                throw std::runtime_error("Expected option value");
            args[argv[i]] = argv[i + 1];
        }
        for (auto key : {"--engine", "--source", "--output", "--msvc", "--sdk", "--configuration"})
            if (!args.count(key))
                throw std::runtime_error(std::string("Missing ") + key);
        Parser p;
        if (args["--configuration"] != "Debug" && args["--configuration"] != "Release")
            throw std::runtime_error("Configuration must be Debug or Release");
        p.engine = fs::canonical(args["--engine"]);
        p.source = fs::canonical(args["--source"]);
        p.output = fs::absolute(args["--output"]);
        auto msvc = fs::path(args["--msvc"]), sdk = fs::path(args["--sdk"]);
        std::istringstream version(msvc.filename().string());
        int major, minor;
        char dot;
        version >> major >> dot >> minor;
        p.flags = {"-x",
                   "c++",
                   "-std=c++20",
                   "--target=x86_64-pc-windows-msvc",
                   "-fms-extensions",
                   "-fms-compatibility",
                   "-fms-compatibility-version=" + std::to_string(major + 5) + "." + std::to_string(minor),
                   "-Wno-ignored-attributes",
                   "-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
                   "-DFINALITY_REFLECTION_SCAN",
                   "-DGLM_FORCE_PURE",
                   "-DGLM_FORCE_UNRESTRICTED_FLOAT",
                   "-DIMGUI_API=__declspec(dllimport)",
                   "-D_MT",
                   "-D_DLL",
                   args["--configuration"] == "Release" ? "-DNDEBUG" : "-D_DEBUG",
                   "-include",
                   "Scene/ScriptModule.h"};
        auto resource = fs::absolute(argv[0]).parent_path() / "ClangResource";
        if (!fs::is_regular_file(resource / "include/stddef.h"))
            throw std::runtime_error("Missing bundled ClangResource/include/stddef.h");
        p.flags.push_back("-resource-dir=" + resource.string());
        p.flags.push_back("-I" + p.source.string());
        if (args["--configuration"] == "Debug")
            p.flags.push_back("-DDEBUG");
        for (auto path : {p.engine / "FinalityEngine/src", p.engine / "vendor/glm",
                          p.engine / "vendor/entt/include", p.engine / "vendor/spdlog/include",
                          p.engine / "vendor/yaml-cpp/include", p.engine / "vendor/imgui", msvc / "include",
                          sdk / "ucrt", sdk / "shared", sdk / "um", sdk / "winrt"})
            p.flags.push_back("-I" + path.string());
        std::vector<fs::path> files;
        for (auto &f : fs::recursive_directory_iterator(p.source))
        {
            auto e = f.path().extension().string();
            std::transform(e.begin(), e.end(), e.begin(), ::tolower);
            if (f.is_regular_file() && (e == ".cpp" || e == ".h" || e == ".hpp"))
                files.push_back(fs::canonical(f.path()));
        }
        std::sort(files.begin(), files.end());
        p.cache.Directory = p.output / ".cache";
        p.cache.Context = "FinalityReflectionCache-v1\n" + str(clang_getClangVersion()) + "\n" +
                          ReflectionCache::Stamp(fs::absolute(argv[0])) + "\n";
        for (const auto &flag : p.flags)
            p.cache.Context += flag + "\n";
        for (const auto &f : files)
            if (f.extension() != ".cpp")
                p.cache.Context += f.generic_string() + "\n";
        for (auto &f : files)
            if (f.extension() == ".cpp")
                p.parse(f);
        for (auto &f : files)
            if (f.extension() != ".cpp" && !p.included.count(f.generic_string()))
                p.parse(f);
        p.generate();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Native script reflection failed: " << e.what() << '\n';
        return 1;
    }
}
