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
        DISPPARAMS args{ values, nullptr, count, 0 };
        return object && SUCCEEDED(object->GetIDsOfNames(IID_NULL, &text, 1, LOCALE_USER_DEFAULT, &id)) &&
               SUCCEEDED(object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &args, nullptr,
                                        nullptr, nullptr));
    }

    void ShowPhysicalFolders(IDispatch* solution)
    {
        Value projects, count;
        if (!Get(solution, L"Projects", projects) || projects.Data.vt != VT_DISPATCH || !Get(projects.Data.pdispVal, L"Count", count) || count.Data.vt != VT_I4)
        {
            return;
        }
        for (long index = 1; index <= count.Data.lVal; ++index)
        {
            LPOLESTR itemName = const_cast<LPOLESTR>(L"Item");
            DISPID itemID;
            if (FAILED(projects.Data.pdispVal->GetIDsOfNames(IID_NULL, &itemName, 1, LOCALE_USER_DEFAULT, &itemID)))
            {
                continue;
            }
            VARIANT argument{};
            argument.vt = VT_I4;
            argument.lVal = index;
            DISPPARAMS args{ &argument, nullptr, 1, 0 };
            Value project, fullName, object;
            if (FAILED(projects.Data.pdispVal->Invoke(itemID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD | DISPATCH_PROPERTYGET, &args, &project.Data, nullptr, nullptr)) || project.Data.vt != VT_DISPATCH)
            {
                continue;
            }
            if (!Get(project.Data.pdispVal, L"FullName", fullName) || fullName.Data.vt != VT_BSTR || std::filesystem::path(fullName.Data.bstrVal).filename() != L"GameScripts.vcxproj")
            {
                continue;
            }
            if (!Get(project.Data.pdispVal, L"Object", object) || object.Data.vt != VT_DISPATCH)
            {
                continue;
            }
            LPOLESTR property = const_cast<LPOLESTR>(L"ShowAllFiles");
            DISPID id, named = DISPID_PROPERTYPUT;
            if (FAILED(object.Data.pdispVal->GetIDsOfNames(IID_NULL, &property, 1, LOCALE_USER_DEFAULT, &id)))
            {
                continue;
            }
            VARIANT enabled{};
            enabled.vt = VT_BOOL;
            enabled.boolVal = VARIANT_TRUE;
            DISPPARAMS put{ &enabled, &named, 1, 1 };
            object.Data.pdispVal->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &put, nullptr, nullptr, nullptr);
        }
    }
} // namespace

bool FINALITY::OpenInRunningSolution(const std::filesystem::path& solution,
                                     const std::filesystem::path& source)
{
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized))
    {
        return false;
    }

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
    if (FAILED(GetRunningObjectTable(0, &table)) || FAILED(CreateBindCtx(0, &context)) || FAILED(table->EnumRunning(&entries)))
    {
        return false;
    }
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
            if (Get(dte.Get(), L"Solution", openSolution) && openSolution.Data.vt == VT_DISPATCH && Get(openSolution.Data.pdispVal, L"FullName", path) && path.Data.vt == VT_BSTR)
            {
                std::error_code error;
                if (std::filesystem::equivalent(std::filesystem::path(path.Data.bstrVal), solution, error) && !error)
                {
                    ShowPhysicalFolders(openSolution.Data.pdispVal);
                    Value operations, argument;
                    if (!Get(dte.Get(), L"ItemOperations", operations) || operations.Data.vt != VT_DISPATCH)
                    {
                        return false;
                    }
                    argument.Data.vt = VT_BSTR;
                    argument.Data.bstrVal = SysAllocString(std::filesystem::absolute(source).c_str());
                    VARIANT values[2]{};
                    values[0].vt = VT_ERROR;
                    values[0].scode = DISP_E_PARAMNOTFOUND;
                    values[1] = argument.Data;
                    if (!Invoke(operations.Data.pdispVal, L"OpenFile", values, 2))
                    {
                        return false;
                    }
                    Value window;
                    if (Get(dte.Get(), L"MainWindow", window) && window.Data.vt == VT_DISPATCH)
                    {
                        Invoke(window.Data.pdispVal, L"Activate", nullptr, 0);
                    }
                    return true;
                }
            }
        }
        entry.Reset();
    }
    return false;
}
