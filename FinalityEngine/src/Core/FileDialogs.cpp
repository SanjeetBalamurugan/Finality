#include "FileDialogs.h"
#include <cstdio>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <cstdlib>
#endif

std::string FINALITY::FileDialogs::OpenFile(const char* filter)
{
#ifdef _WIN32
	// WINDOWS
	char fileName[MAX_PATH] = {};

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
		return fileName;

	return {};
#else
	// LINUX
	std::array<char, 4096> buffer{};

	FILE* pipe = popen(
		"zenity --file-selection --file-filter='Scene files | *.scene'",
		"r"
	);

	if (!pipe)
		return {};

	if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
	{
		pclose(pipe);

		std::string path = buffer.data();

		if (!path.empty() && path.back() == '\n')
			path.pop_back();

		return path;
	}

	pclose(pipe);
	return {};
#endif
}

std::string FINALITY::FileDialogs::SaveFile(const char* filter, const char* defaultName)
{
#ifdef _WIN32
	// WINDOWS
	char fileName[MAX_PATH] = {};

	strcpy_s(fileName, defaultName);

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetSaveFileNameA(&ofn))
		return fileName;

	return {};
#else
	// LINUX
	std::string command =
		"zenity --file-selection --save "
		"--confirm-overwrite "
		"--filename=\"" + std::string(defaultName) + "\" "
		"--file-filter='Scene files | *.scene'";

	std::array<char, 4096> buffer{};

	FILE* pipe = popen(command.c_str(), "r");

	if (!pipe)
		return {};

	if (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
	{
		pclose(pipe);

		std::string path = buffer.data();

		if (!path.empty() && path.back() == '\n')
			path.pop_back();

		return path;
	}

	pclose(pipe);
	return {};
#endif
}