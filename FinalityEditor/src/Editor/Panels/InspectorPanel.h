#pragma once
#include <Finality.h>
#include <Editor/EditorCameraScene.h>
#include <entt.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

namespace FINALITY
{
	static glm::vec3 s_CopiedVector = glm::vec3(0.0f);

	static void DrawVec3Control(FINALITY::ImGUIPanel& panel, std::string_view label, glm::vec3& values, float resetValue = 0.0f)
	{
		panel.PushID(label);

		float totalWidth = panel.GetContentRegionAvail().x;
		float labelWidth = totalWidth * 0.3f;
		float itemWidth = (totalWidth * 0.7f) / 3.0f;

		panel.Text(label);

		if (panel.BeginPopup("VectorContextMenu"))
		{
			if (panel.MenuItem("Reset"))
			{
				values = glm::vec3(resetValue);
			}
			if (panel.MenuItem("Copy"))
			{
				s_CopiedVector = values;
			}
			if (panel.MenuItem("Paste"))
			{
				values = s_CopiedVector;
			}
			panel.EndPopup();
		}

		if (panel.IsItemClicked(1))
		{
			panel.OpenPopup("VectorContextMenu");
		}

		panel.SameLine(labelWidth);

		panel.PushItemWidth(itemWidth - 4.0f);
		panel.SliderFloat("X", &values.x, -50.0f, 50.0f);
		panel.PopItemWidth();
		panel.SameLine();

		panel.PushItemWidth(itemWidth - 4.0f);
		panel.SliderFloat("Y", &values.y, -50.0f, 50.0f);
		panel.PopItemWidth();
		panel.SameLine();

		panel.PushItemWidth(itemWidth - 4.0f);
		panel.SliderFloat("Z", &values.z, -50.0f, 50.0f);
		panel.PopItemWidth();

		panel.PopID();
	}

	static void DrawFloatControl(FINALITY::ImGUIPanel& panel, std::string_view label, float& value, float minVal, float maxVal)
	{
		panel.PushID(label);

		float totalWidth = panel.GetContentRegionAvail().x;
		float labelWidth = totalWidth * 0.3f;
		float itemWidth = totalWidth * 0.7f;

		panel.Text(label);
		panel.SameLine(labelWidth);

		panel.PushItemWidth(itemWidth - 4.0f);
		panel.SliderFloat("##Value", &value, minVal, maxVal);
		panel.PopItemWidth();

		panel.PopID();
	}

	static void DrawComponentHeader(FINALITY::ImGUIPanel& panel, std::string_view title, bool canRemove, bool& shouldRemove, bool& shouldReset)
	{
		panel.PushID(title);

		float availWidth = panel.GetContentRegionAvail().x;
		panel.TextColored({ 0.85f, 0.85f, 0.9f, 1.0f }, title);

		panel.SameLine(availWidth - 24.0f);
		if (panel.Button("...", glm::vec2(24.0f, 20.0f)))
		{
			panel.OpenPopup("ComponentContextMenu");
		}

		if (panel.IsItemClicked(1))
		{
			panel.OpenPopup("ComponentContextMenu");
		}

		if (panel.BeginPopup("ComponentContextMenu"))
		{
			if (panel.MenuItem("Reset"))
			{
				shouldReset = true;
			}
			if (canRemove && panel.MenuItem("Remove Component"))
			{
				shouldRemove = true;
			}
			panel.EndPopup();
		}

		panel.PopID();
	}

	static void DrawInspectorPanel(const Scene* scene, const entt::entity& selectedEntityHandle)
	{
		FINALITY::ImGUIPanel panel("Inspector");

		if (selectedEntityHandle == entt::null || !scene->GetRegistry().valid(selectedEntityHandle))
		{
			panel.Text("Select an entity in the Hierarchy to view properties.");
			return;
		}

		entt::registry& registry = const_cast<entt::registry&>(scene->GetRegistry());

		if (!registry.all_of<TagComponent>(selectedEntityHandle))
		{
			registry.emplace<TagComponent>(selectedEntityHandle, "Entity");
		}
		if (!registry.all_of<TransformComponent>(selectedEntityHandle))
		{
			registry.emplace<TransformComponent>(selectedEntityHandle);
		}

		{
			auto& tagComp = registry.get<TagComponent>(selectedEntityHandle);

			float totalWidth = panel.GetContentRegionAvail().x;
			panel.Text("Name");
			panel.SameLine(totalWidth * 0.2f);
			panel.PushItemWidth(totalWidth * 0.8f - 4.0f);
			panel.InputText("##TagInput", &tagComp.tag);
			panel.PopItemWidth();

			panel.TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "UUID: " + std::to_string(static_cast<uint64_t>(tagComp.uuid)));
			panel.Separator();
		}

		if (registry.all_of<TransformComponent>(selectedEntityHandle))
		{
			bool removeTransform = false;
			bool resetTransform = false;
			DrawComponentHeader(panel, "Transform", false, removeTransform, resetTransform);

			auto& transform = registry.get<TransformComponent>(selectedEntityHandle);
			if (resetTransform)
			{
				transform.Position = glm::vec3(0.0f);
				transform.Rotation = glm::vec3(0.0f);
				transform.Scale = glm::vec3(1.0f);
			}

			DrawVec3Control(panel, "Position", transform.Position, 0.0f);
			DrawVec3Control(panel, "Rotation", transform.Rotation, 0.0f);
			DrawVec3Control(panel, "Scale", transform.Scale, 1.0f);
			transform.Matrix = transform.GetTransformMatrix();

			panel.Separator();
		}

		if (registry.all_of<MeshComponent>(selectedEntityHandle))
		{
			bool removeMesh = false;
			bool resetMesh = false;
			DrawComponentHeader(panel, "Mesh Filter", true, removeMesh, resetMesh);

			if (removeMesh)
			{
				registry.remove<MeshComponent>(selectedEntityHandle);
			}
			else
			{
				auto& mesh = registry.get<MeshComponent>(selectedEntityHandle);
				if (resetMesh)
				{
					mesh.MeshData = nullptr;
					mesh.MeshIdentifier.clear();
				}

				float totalWidth = panel.GetContentRegionAvail().x;
				float labelWidth = totalWidth * 0.3f;
				float fieldWidth = totalWidth * 0.7f;

				panel.Text("Mesh");
				panel.SameLine(labelWidth);

				std::string displayLabel = "None (Mesh)";
				if (mesh.MeshData && !mesh.MeshIdentifier.empty())
				{
					displayLabel = mesh.MeshIdentifier;
				}
				else if (mesh.MeshData)
				{
					displayLabel = "Runtime Mesh";
				}

				if (panel.Button(displayLabel, glm::vec2(fieldWidth - 4.0f, 22.0f)))
				{
					panel.OpenPopup("SelectMeshPopup");
				}

				if (panel.BeginPopup("SelectMeshPopup", PanelFlags_NoTitleBar | PanelFlags_AlwaysAutoResize))
				{
					static std::string meshSearchFilter;
					if (meshSearchFilter.capacity() < 128)
						meshSearchFilter.reserve(128);

					panel.PushItemWidth(220.0f);
					panel.InputText("##MeshSearch", &meshSearchFilter);
					panel.PopItemWidth();
					panel.Separator();

					struct MeshOption
					{
						std::string Name;
						std::string Identifier;
					};

					static const std::vector<MeshOption> builtinMeshes = {
						{ "None", "" },
						{ "Cube", "builtin://cube" }
					};

					std::string lowerFilter = meshSearchFilter;
					std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

					for (const auto& option : builtinMeshes)
					{
						std::string lowerName = option.Name;
						std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

						if (!lowerFilter.empty() && lowerName.find(lowerFilter) == std::string::npos)
							continue;

						if (panel.Button(option.Name, glm::vec2(220.0f, 22.0f)))
						{
							mesh.MeshIdentifier = option.Identifier;
							if (option.Identifier.empty())
							{
								mesh.MeshData = nullptr;
							}
							else
							{
								mesh.MeshData = Mesh::Create(option.Identifier);
							}

							meshSearchFilter.clear();
							panel.CloseCurrentPopup();
						}
					}

					panel.EndPopup();
				}
			}
			panel.Separator();
		}

		if (registry.all_of<MaterialComponent>(selectedEntityHandle))
		{
			bool removeMaterial = false;
			bool resetMaterial = false;
			DrawComponentHeader(panel, "Mesh Renderer / Material", true, removeMaterial, resetMaterial);

			if (removeMaterial)
			{
				registry.remove<MaterialComponent>(selectedEntityHandle);
			}
			else
			{
				auto& material = registry.get<MaterialComponent>(selectedEntityHandle);
				if (resetMaterial)
				{
					material.MaterialInstance.reset();
					material.MaterialIdentifier.clear();
				}

				auto toProjectPath = [](const std::string& selected) -> std::string
				{
					if (selected.empty()) return {};
					try
					{
						namespace fs = std::filesystem;
						const fs::path abs = fs::absolute(selected).lexically_normal();
						const fs::path cwd = fs::current_path();
						const fs::path relative = fs::relative(abs, cwd);
						if (!relative.empty() && *relative.begin() != "..")
							return relative.generic_string();
						return abs.generic_string();
					}
					catch (const std::exception&)
					{
						return selected;
					}
				};

				panel.Text("Material Asset");
				panel.Text(material.MaterialIdentifier.empty()
					? (material.MaterialInstance ? "Runtime-only (not saved)" : "None")
					: material.MaterialIdentifier);

				if (panel.Button("Load .fmat"))
				{
					std::string selected = FileDialogs::OpenFile(
						"Finality Material (*.fmat)\0*.fmat\0All Files (*.*)\0*.*\0");
					if (!selected.empty())
					{
						const std::string path = toProjectPath(selected);
						auto asset = AssetManager::CreateAsset<MaterialAsset>(path, Renderer::GetDevice());
						if (asset)
						{
							auto instance = asset->CreateInstance();
							if (instance)
							{
								material.MaterialIdentifier = path;
								material.MaterialInstance = std::move(instance);
							}
						}
					}
				}

				if (!material.MaterialIdentifier.empty() && material.MaterialInstance)
				{
					auto asset = AssetManager::CreateAsset<MaterialAsset>(
						material.MaterialIdentifier, Renderer::GetDevice());
					if (asset)
					{
						panel.Text("Type: " + asset->GetDefinition().Type);
						panel.Text("Shader: " + asset->GetDefinition().FragmentShader);

						for (const auto& property : asset->GetDefinition().Properties)
						{
							if (property.Type == MaterialParameterType::Float)
							{
								float value = 0.0f;
								if (material.MaterialInstance->TryGetUniform1f(property.Name, value))
								{
									if (panel.SliderFloat(property.Name, &value, -10.0f, 10.0f))
										material.MaterialInstance->SetUniform1f(property.Name, value);
								}
							}
							else if (property.Type == MaterialParameterType::Vec4)
							{
								glm::vec4 value;
								if (material.MaterialInstance->TryGetUniformVec4(property.Name, value))
								{
									float rgba[4] = { value.r, value.g, value.b, value.a };
									if (panel.ColorEdit4(property.Name, rgba))
										material.MaterialInstance->SetUniformVec4(property.Name,
											glm::vec4(rgba[0], rgba[1], rgba[2], rgba[3]));
								}
							}
						}

						if (panel.Button("Save As .fmat"))
						{
							const std::string defaultName = asset->GetDefinition().Name + ".fmat";
							std::string selected = FileDialogs::SaveFile(
								"Finality Material (*.fmat)\0*.fmat\0All Files (*.*)\0*.*\0",
								defaultName.c_str());
							if (!selected.empty())
							{
								std::filesystem::path saved(selected);
								if (saved.extension() != ".fmat") saved.replace_extension(".fmat");
                                const auto previous = asset->GetDefinition();
                                asset->UpdateDefaultsFromInstance(*material.MaterialInstance);
                                const bool savedSuccessfully = MaterialSerializer::Serialize(asset->GetDefinition(), saved.string());
                                asset->GetDefinition() = previous;
                                if (savedSuccessfully)
                                {
                                    material.MaterialIdentifier = toProjectPath(saved.string());
                                    AssetManager::InvalidateAssetPath(material.MaterialIdentifier);
                                }
							}
						}
					}
				}
			}
			panel.Separator();
		}

		if (registry.all_of<CameraComponent>(selectedEntityHandle))
		{
			bool removeCamera = false;
			bool resetCamera = false;
			DrawComponentHeader(panel, "Camera", true, removeCamera, resetCamera);

			if (removeCamera)
			{
				registry.remove<CameraComponent>(selectedEntityHandle);
			}
			else
			{
				auto& cameraComp = registry.get<CameraComponent>(selectedEntityHandle);
				if (resetCamera)
				{
					cameraComp.FOV = 45.0f;
					cameraComp.NearClip = 0.1f;
					cameraComp.FarClip = 1000.0f;
					cameraComp.RebuildCamera();
				}

				bool wasPrimary = cameraComp.IsPrimary;
				panel.Checkbox("Main Camera (Primary)", &cameraComp.IsPrimary);

				if (cameraComp.IsPrimary && !wasPrimary)
				{
					auto camView = registry.view<CameraComponent>();
					for (auto entity : camView)
					{
						if (entity != selectedEntityHandle)
						{
							camView.get<CameraComponent>(entity).IsPrimary = false;
						}
					}
				}

				float prevFOV = cameraComp.FOV;
				float prevNear = cameraComp.NearClip;
				float prevFar = cameraComp.FarClip;

				DrawFloatControl(panel, "Field of View", cameraComp.FOV, 10.0f, 120.0f);
				DrawFloatControl(panel, "Clipping Near", cameraComp.NearClip, 0.01f, 10.0f);
				DrawFloatControl(panel, "Clipping Far", cameraComp.FarClip, 10.0f, 5000.0f);

				if (cameraComp.FOV != prevFOV || cameraComp.NearClip != prevNear || cameraComp.FarClip != prevFar)
				{
					cameraComp.RebuildCamera();
				}
			}
			panel.Separator();
		}

		panel.Spacing();

		float availWidth = panel.GetContentRegionAvail().x;
		float buttonWidth = 180.0f;
		panel.SetCursorPosX((availWidth - buttonWidth) * 0.5f);

		if (panel.Button("Add Component", glm::vec2(buttonWidth, 26.0f)))
		{
			panel.OpenPopup("AddComponentPopup");
		}

		if (panel.BeginPopup("AddComponentPopup", PanelFlags_NoTitleBar | PanelFlags_NoResize | PanelFlags_AlwaysAutoResize))
		{
			static std::string componentSearchFilter;
			if (componentSearchFilter.capacity() < 128)
				componentSearchFilter.reserve(128);

			float popupItemWidth = 220.0f;

			panel.PushItemWidth(popupItemWidth);
			panel.InputText("##CompSearch", &componentSearchFilter);
			panel.PopItemWidth();
			panel.Separator();

			struct ComponentEntry
			{
				std::string Name;
				bool AlreadyAdded;
			};

			std::vector<ComponentEntry> availableComponents = {
				{ "Mesh Filter", registry.all_of<MeshComponent>(selectedEntityHandle) },
				{ "Material", registry.all_of<MaterialComponent>(selectedEntityHandle) },
				{ "Camera", registry.all_of<CameraComponent>(selectedEntityHandle) }
			};

			std::string lowerFilter = componentSearchFilter;
			std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

			bool anyShown = false;

			for (const auto& entry : availableComponents)
			{
				if (entry.AlreadyAdded)
					continue;

				std::string lowerName = entry.Name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

				if (!lowerFilter.empty() && lowerName.find(lowerFilter) == std::string::npos)
					continue;

				anyShown = true;

				if (panel.Button(entry.Name, glm::vec2(popupItemWidth, 24.0f)))
				{
					if (entry.Name == "Mesh Filter")
					{
						registry.emplace<MeshComponent>(selectedEntityHandle);
					}
					else if (entry.Name == "Material")
					{
						registry.emplace<MaterialComponent>(selectedEntityHandle);
					}
					else if (entry.Name == "Camera")
					{
						auto camView = registry.view<CameraComponent>();
						bool hasPrimaryInScene = false;
						for (auto entity : camView)
						{
							if (camView.get<CameraComponent>(entity).IsPrimary)
							{
								hasPrimaryInScene = true;
								break;
							}
						}

						auto& newCam = registry.emplace<CameraComponent>(selectedEntityHandle, 45.0f, 1.778f, 0.1f, 1000.0f);
						newCam.IsPrimary = !hasPrimaryInScene;
					}

					componentSearchFilter.clear();
					panel.CloseCurrentPopup();
				}
			}

			if (!anyShown)
			{
				panel.TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "No matching components available.");
			}

			panel.EndPopup();
		}
	}
}