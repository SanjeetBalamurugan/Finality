#pragma once
#include <Finality.h>

namespace FINALITY
{
	static void DrawConsole()
	{
		FINALITY::ImGUIPanel panel("Console");

		if (panel.Button("Clear"))
		{
			Logger::GetConsoleSink()->Clear();
		}
		panel.SameLine();

		static bool collapse = false;
		panel.Checkbox("Collapse", &collapse);
		panel.SameLine();

		auto rawEntries = Logger::GetConsoleSink()->CopyEntries();

		int infoCount = 0;
		int warnCount = 0;
		int errorCount = 0;

		for (const auto& entry : rawEntries)
		{
			if (entry.Level == spdlog::level::warn)
				warnCount++;
			else if (entry.Level >= spdlog::level::err)
				errorCount++;
			else
				infoCount++;
		}

		static bool showInfo = true;
		static bool showWarn = true;
		static bool showErr = true;
		static std::string searchFilter = "";

		if (searchFilter.capacity() < 256)
			searchFilter.reserve(256);

		std::string infoLabel = fmt::format("Info ({})", infoCount);
		std::string warnLabel = fmt::format("Warn ({})", warnCount);
		std::string errorLabel = fmt::format("Error ({})", errorCount);

		float spacing = panel.GetStyleItemSpacing().x;
		float checkboxPadding = panel.GetFontSize() * 2.5f;

		float infoWidth = panel.CalcTextWidth(infoLabel) + checkboxPadding;
		float warnWidth = panel.CalcTextWidth(warnLabel) + checkboxPadding;
		float errorWidth = panel.CalcTextWidth(errorLabel) + checkboxPadding;

		float filtersWidth = infoWidth + warnWidth + errorWidth + spacing * 2.0f;

		float windowWidth = panel.GetWindowWidth();
		float windowPadding = panel.GetWindowPadding().x;

		float searchWidth = 350.0f;
		float rightControlsWidth = searchWidth + spacing + filtersWidth;
		float rightControlsX = windowWidth - windowPadding - rightControlsWidth;

		float currentX = panel.GetCursorPosX();

		if (currentX < rightControlsX)
		{
			panel.SetCursorPosX(rightControlsX);
		}

		panel.PushItemWidth(searchWidth);
		panel.InputText("##ConsoleSearch", &searchFilter);
		panel.PopItemWidth();
		panel.SameLine();

		panel.Checkbox(infoLabel, &showInfo);
		panel.SameLine();
		panel.Checkbox(warnLabel, &showWarn);
		panel.SameLine();
		panel.Checkbox(errorLabel, &showErr);

		panel.Separator();

		std::vector<LogEntry> displayEntries;
		static size_t lastRawSize = 0;

		bool hasNewEntries = rawEntries.size() > lastRawSize;
		lastRawSize = rawEntries.size();

		std::vector<LogEntry> filteredEntries;

		std::string lowerSearch = searchFilter;

		std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
			});

		for (const auto& entry : rawEntries)
		{
			if (entry.Level == spdlog::level::warn && !showWarn)
				continue;

			if (entry.Level >= spdlog::level::err && !showErr)
				continue;

			if (entry.Level < spdlog::level::warn && !showInfo)
				continue;

			if (!lowerSearch.empty())
			{
				std::string lowerMsg = entry.Message;

				std::transform(lowerMsg.begin(), lowerMsg.end(), lowerMsg.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
					});

				if (lowerMsg.find(lowerSearch) == std::string::npos)
					continue;
			}

			filteredEntries.push_back(entry);
		}

		if (collapse)
		{
			for (const auto& entry : filteredEntries)
			{
				bool found = false;

				for (auto& disp : displayEntries)
				{
					if (disp.RawPayload == entry.RawPayload && disp.Level == entry.Level && disp.LoggerName == entry.LoggerName)
					{
						disp.Count++;
						disp.Message = entry.Message;
						found = true;
						break;
					}
				}

				if (!found)
					displayEntries.push_back(entry);
			}
		}
		else
		{
			displayEntries = filteredEntries;
		}

		static int selectedIndex = -1;

		float footerHeight = panel.GetTextLineHeightWithSpacing() * 5.0f;

		if (panel.BeginChild("ScrollingRegion", glm::vec2(0.0f, -footerHeight), true, PanelFlags_HorizontalScrollbar))
		{
			for (int i = 0; i < static_cast<int>(displayEntries.size()); i++)
			{
				auto& entry = displayEntries[i];

				glm::vec4 color = glm::vec4(1.0f);
				const char* icon = "[i]";

				if (entry.Level == spdlog::level::warn)
				{
					color = glm::vec4(1.0f, 0.82f, 0.21f, 1.0f);
					icon = "[!]";
				}
				else if (entry.Level >= spdlog::level::err)
				{
					color = glm::vec4(0.95f, 0.32f, 0.22f, 1.0f);
					icon = "[X]";
				}

				std::string label;

				if (collapse && entry.Count > 1)
				{
					label = fmt::format("{} ({}) {}", icon, entry.Count, entry.Message);
				}
				else
				{
					label = fmt::format("{} {}", icon, entry.Message);
				}

				bool isSelected = selectedIndex == i;

				panel.PushID(i);
				panel.PushStyleColor(0, color);

				if (panel.Selectable(label, isSelected))
				{
					selectedIndex = i;
				}

				panel.PopStyleColor();
				panel.PopID();
			}

			if (hasNewEntries)
			{
				panel.SetScrollHereY(1.0f);
			}
		}

		panel.EndChild();

		panel.Separator();

		if (panel.BeginChild("DetailsRegion", glm::vec2(0.0f, 0.0f), true, PanelFlags_None))
		{
			if (selectedIndex >= 0 && selectedIndex < static_cast<int>(displayEntries.size()))
			{
				auto& selectedEntry = displayEntries[selectedIndex];

				panel.Text(selectedEntry.Message);

				panel.PushStyleColor(0, glm::vec4(0.55f, 0.55f, 0.55f, 1.0f));

				panel.Text(selectedEntry.stackTrace);

				panel.PopStyleColor();
			}
		}

		panel.EndChild();
	}
}