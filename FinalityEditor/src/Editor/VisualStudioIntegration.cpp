#include "VisualStudioIntegration.h"
#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <string>
#include <wrl/client.h>
namespace
{
    struct Value
    {
        VARIANT Data;
        Value()
        {
            VariantInit(&Data);
        }
        ~Value()
        {
            VariantClear(&Data);
        }
    };
    bool Get(IDispatch* object, const wchar_t* name, Value& result)
    {
        LPOLESTR text = const_cast<LPOLESTR>(name);
        DISPID id;
        DISPPARAMS args{};
        return object && SUCCEEDED(object->GetIDsOfNames(IID_NULL, &text, 1, LOCALE_USER_DEFAULT, &id)) &&
               SUCCEEDED(object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &args,
                                        &result.Data, nullptr, nullptr));
    }
    bool Invoke(IDispatch* object, const wchar_t* name, VARIANT* values, UINT count)
    {
        LPOLESTR text = const_cast<LPOLESTR>(name);
        DISPID id;
        DISPPARAMS args{values, nullptr, count, 0};
        return object && SUCCEEDED(object->GetIDsOfNames(IID_NULL, &text, 1, LOCALE_USER_DEFAULT, &id)) &&
               SUCCEEDED(object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &args, nullptr,
                                        nullptr, nullptr));
    }
}
bool FINALITY::OpenInRunningSolution(const std::filesystem::path& solution,
                                     const std::filesystem::path& source)
{
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized))
        return false;
    struct Apartment
    {
        ~Apartment()
        {
            CoUninitialize();
        }
    } apartment;
    Microsoft::WRL::ComPtr<IRunningObjectTable> table;
    Microsoft::WRL::ComPtr<IBindCtx> context;
    Microsoft::WRL::ComPtr<IEnumMoniker> entries;
    if (FAILED(GetRunningObjectTable(0, &table)) || FAILED(CreateBindCtx(0, &context)) ||
        FAILED(table->EnumRunning(&entries)))
        return false;
    Microsoft::WRL::ComPtr<IMoniker> entry;
    while (entries->Next(1, &entry, nullptr) == S_OK)
    {
        LPOLESTR display = nullptr;
        bool visualStudio = false;
        if (SUCCEEDED(entry->GetDisplayName(context.Get(), nullptr, &display)))
        {
            visualStudio = std::wstring(display).find(L"VisualStudio.DTE.") != std::wstring::npos;
            CoTaskMemFree(display);
        }
        Microsoft::WRL::ComPtr<IUnknown> object;
        Microsoft::WRL::ComPtr<IDispatch> dte;
        if (visualStudio && SUCCEEDED(table->GetObject(entry.Get(), &object)) && SUCCEEDED(object.As(&dte)))
        {
            Value openSolution, path;
            if (Get(dte.Get(), L"Solution", openSolution) && openSolution.Data.vt == VT_DISPATCH &&
                Get(openSolution.Data.pdispVal, L"FullName", path) && path.Data.vt == VT_BSTR)
            {
                std::error_code error;
                if (std::filesystem::equivalent(std::filesystem::path(path.Data.bstrVal), solution, error) &&
                    !error)
                {
                    Value operations, argument;
                    if (!Get(dte.Get(), L"ItemOperations", operations) || operations.Data.vt != VT_DISPATCH)
                        return false;
                    argument.Data.vt = VT_BSTR;
                    argument.Data.bstrVal = SysAllocString(std::filesystem::absolute(source).c_str());
                    VARIANT values[2]{};
                    values[0].vt = VT_ERROR;
                    values[0].scode = DISP_E_PARAMNOTFOUND;
                    values[1] = argument.Data;
                    if (!Invoke(operations.Data.pdispVal, L"OpenFile", values, 2))
                        return false;
                    Value window;
                    if (Get(dte.Get(), L"MainWindow", window) && window.Data.vt == VT_DISPATCH)
                        Invoke(window.Data.pdispVal, L"Activate", nullptr, 0);
                    return true;
                }
            }
        }
        entry.Reset();
    }
    return false;
}
