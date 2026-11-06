#pragma once
#include <Finality.h>
#include <Editor/EditorCameraScene.h>
#include <entt.hpp>

namespace FINALITY
{
	static void DrawSceneHierarchyPanel(Scene* scene, entt::entity& selectedEntityHandle)
	{
		FINALITY::ImGUIPanel panel("Hierarchy");

		static std::string searchFilter;
		if (searchFilter.capacity() < 256)
			searchFilter.reserve(256);

		float searchBarWidth = panel.GetContentRegionAvail().x - panel.GetWindowPadding().x;

		panel.PushItemWidth(searchBarWidth);
		panel.InputText("##HierarchySearch", &searchFilter);
		panel.PopItemWidth();

		panel.Spacing();

		entt::registry& registry = scene->GetRegistry();

		auto GetEntityName = [&](entt::entity entity) -> std::string
			{
				if (registry.all_of<TagComponent>(entity))
					return registry.get<TagComponent>(entity).tag;

				return "GameObject";
			};

		auto MatchesSearch = [&](entt::entity entity) -> bool
			{
				if (searchFilter.empty())
					return true;

				return GetEntityName(entity).find(searchFilter) != std::string::npos;
			};

		static entt::entity entityContextTriggered = entt::null;
		static bool openEntityContext = false;
		static bool openWorkspaceContext = false;
		bool anyItemHoveredThisFrame = false;

		auto DrawEntityNode = [&](auto& self, entt::entity entity) -> void
			{
				if (entity == entt::null || !registry.valid(entity))
					return;

				const auto* hierarchy = registry.try_get<RelationshipComponent>(entity);

				entt::entity firstChild = hierarchy
					? hierarchy->FirstChild
					: entt::null;

				bool hasChildren = firstChild != entt::null;

				if (!searchFilter.empty() && !MatchesSearch(entity))
				{
					bool childMatches = false;
					entt::entity child = firstChild;

					while (child != entt::null)
					{
						if (registry.valid(child) && MatchesSearch(child))
						{
							childMatches = true;
							break;
						}

						const auto* childRel = registry.try_get<RelationshipComponent>(child);
						if (!childRel)
							break;

						child = childRel->NextSibling;
					}

					if (!childMatches)
						return;
				}

				panel.PushID(static_cast<int>(entity));

				bool selected = selectedEntityHandle == entity;
				bool opened = false;

				if (hasChildren)
				{
					opened = panel.TreeNode(GetEntityName(entity), selected);

					if (panel.IsItemClicked(0))
						selectedEntityHandle = entity;
				}
				else
				{
					panel.Indent(14.0f);
					if (panel.Selectable(GetEntityName(entity), selected))
						selectedEntityHandle = entity;
					panel.Unindent(14.0f);
				}

				if (panel.IsItemHovered())
					anyItemHoveredThisFrame = true;

				if (panel.IsItemClicked(1))
				{
					selectedEntityHandle = entity;
					entityContextTriggered = entity;
					openEntityContext = true;
				}

				panel.PopID();

				if (!opened)
					return;

				entt::entity child = firstChild;
				while (child != entt::null)
				{
					self(self, child);

					const auto* childRel = registry.try_get<RelationshipComponent>(child);
					if (!childRel)
						break;

					child = childRel->NextSibling;
				}

				panel.TreePop();
			};

		for (auto entity : registry.view<RelationshipComponent>())
		{
			const auto& hierarchy = registry.get<RelationshipComponent>(entity);
			if (hierarchy.Parent == entt::null)
				DrawEntityNode(DrawEntityNode, entity);
		}

		for (auto entity : registry.view<TagComponent>())
		{
			if (!registry.all_of<RelationshipComponent>(entity))
				DrawEntityNode(DrawEntityNode, entity);
		}

		if (panel.IsHovered() && panel.IsMouseClicked(1) && !anyItemHoveredThisFrame)
		{
			openWorkspaceContext = true;
		}

		if (openEntityContext)
		{
			panel.OpenPopup("HierarchyEntityContext");
			openEntityContext = false;
		}

		if (openWorkspaceContext)
		{
			panel.OpenPopup("HierarchyWorkspaceContext");
			openWorkspaceContext = false;
		}

		bool entityDestroyed = false;
		entt::entity entityToDestroy = entt::null;

		if (panel.BeginPopup("HierarchyEntityContext"))
		{
			if (panel.MenuItem("Create Child Entity"))
			{
                FINALITY::Entity childEntity = scene->CreateEntity("Child GameObject");
                childEntity.SetParent(scene->GetEntity(entityContextTriggered));
			}
			if (panel.MenuItem("Delete Entity"))
			{
				entityToDestroy = entityContextTriggered;
				entityDestroyed = true;
			}
			panel.EndPopup();
		}

		if (panel.BeginPopup("HierarchyWorkspaceContext"))
		{
			if (panel.MenuItem("Create Empty Entity"))
			{
				scene->CreateEntity("Empty");
			}
			panel.EndPopup();
		}

        if (entityDestroyed && entityToDestroy != entt::null)
        {
            scene->DestroyEntity(entityToDestroy);
            selectedEntityHandle = entt::null;
            entityContextTriggered = entt::null;
        }

		if (panel.IsHovered() && panel.IsItemClicked(0) && !anyItemHoveredThisFrame)
		{
			selectedEntityHandle = entt::null;
		}
	}
}
