#pragma once

#include <string_view>
#include <typeinfo>

namespace FINALITY::Helpers
{
    static std::string_view GetCleanClassName(const char* rawName)
    {
        std::string_view name(rawName);

        if (name.rfind("class ", 0) == 0)  name.remove_prefix(6);
        else if (name.rfind("struct ", 0) == 0) name.remove_prefix(7);
        else if (name.rfind("enum ", 0) == 0)   name.remove_prefix(5);

        size_t lastNamespace = name.find_last_of("::");
        if (lastNamespace != std::string_view::npos)
        {
            name.remove_prefix(lastNamespace + 1);
        }

        return name;
    }
}