/**
 * @file
 * @ingroup core
 * @brief Provides native file, save and folder selection.
 *
 * Windows uses IFileDialog, including the Explorer-style folder picker. Cancellation returns an empty selection and dialogs preserve the process working directory.
 *
 * @par Usage
 * Treat an empty result as cancellation. Validate the selected destination for the operation; choosing a folder alone does not mean it is suitable for overwriting.
 */
#pragma once
#include <Core/Core.h>
#include <string>

namespace FINALITY
{
    /**
	 * @ingroup core
	 * @brief Provides native file, save and folder selection.
	 *
	 * Windows uses IFileDialog, including the Explorer-style folder picker. Cancellation returns an empty selection and dialogs preserve the process working directory.
	 *
	 * @par Usage
	 * Treat an empty result as cancellation. Validate the selected destination for the operation; choosing a folder alone does not mean it is suitable for overwriting.
	 */
    class FAPI FileDialogs
    {
    public:
        static std::string OpenFile(const char* filter);
        static std::string OpenFolder();
        static std::string SaveFile(const char* filter, const char* defaultName);
    };
} // namespace FINALITY
