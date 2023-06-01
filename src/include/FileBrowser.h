#include <functional>
#include <filesystem>
#include <imgui.h>

#pragma once

namespace ImGui
{
	static char dirBuffer[256];
	static char selectedFileBuffer[256];
	static int selectedIndex = -1;

	static void strcpy(std::string &src, char *dst)
	{
		int len = src.length();
		src.copy(dst, len);
		dst[len] = '\0';
	}

	void InitFileBrowser(std::string startDir = "")
	{
		std::string currentDir = startDir != "" ? startDir : std::filesystem::current_path().u8string();
		strcpy(currentDir, dirBuffer);

		ImGui::OpenPopup("LoadRom");
	}

	void FileBrowser(std::function<void(const char *)> callback)
	{
		ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("LoadRom", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::InputText("##file", dirBuffer, 255);

			if (std::filesystem::exists(dirBuffer))
			{
				if (ImGui::ListBoxHeader("##entry_list"))
				{
					int index = 0;
					for (const auto &entry : std::filesystem::directory_iterator(dirBuffer))
					{
						ImGui::Text(std::filesystem::is_directory(entry) ? "[D]" : "[F]");
						ImGui::SameLine(30);

						std::string current = entry.path().u8string();
						std::string fileNameString = entry.path().filename().u8string();

						ImGui::PushID(index);
						if (ImGui::Selectable(fileNameString.c_str(), selectedIndex == index))
						{
							if (std::filesystem::is_directory(entry))
							{
								strcpy(current, dirBuffer);
								selectedIndex = -1;
							}
							else
							{
								strcpy(current, selectedFileBuffer);
								selectedIndex = index;
							}
						}
						ImGui::PopID();

						index++;
					}

					ImGui::ListBoxFooter();
				}
			}

			if (ImGui::ArrowButton("##dir_up", ImGuiDir_Up))
			{
				std::string parentString = std::filesystem::path(dirBuffer).parent_path().u8string();
				strcpy(parentString, dirBuffer);
			}

			ImGui::SameLine();

			if (ImGui::Button("Select") && selectedIndex != -1)
			{
				callback(selectedFileBuffer);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}