/**
 * @file
 * @ingroup scene
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for SceneManager.h.
 */
#pragma once
#include <Core/Core.h>
#include <memory>
#include "Scene.h"

namespace FINALITY
{
	/** @todo TODO: Document SceneManager class semantics, ownership and usage. */
	class FAPI SceneManager
	{
	public:
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager(SceneManager&&) = delete;
		SceneManager& operator=(SceneManager&&) = delete;

		static SceneManager& GetInstance()
		{
			static SceneManager instance;
			return instance;
		}

		void Initialize() const;
        void Update(float ts);
		void Shutdown();

		void SetScene(std::unique_ptr<Scene> scene);
		void ChangeScene();
        bool EnterRuntime(std::unique_ptr<Scene> scene);
        void ExitRuntime();
        bool IsRuntimeSceneActive() const { return m_EditorScene != nullptr; }

		const Scene* GetActiveScene() const { return m_CurrentScene.get(); }
		Scene* GetActiveScene() { return m_CurrentScene.get(); }

		void SetSimulationEnabled(bool enabled) { m_SimulationEnabled = enabled; }
		bool IsSimulationEnabled() const { return m_SimulationEnabled; }

	private:
		SceneManager() {};
		~SceneManager() = default;

		std::unique_ptr<Scene> m_CurrentScene = std::make_unique<FINALITY::EmptyScene>();
		std::unique_ptr<Scene> m_NextScene = nullptr;
        std::unique_ptr<Scene> m_EditorScene;
		bool m_SimulationEnabled = true;
        bool m_Updating = false, m_Applying = false, m_ChangeRequested = false, m_ExitRequested = false;
	};
}
