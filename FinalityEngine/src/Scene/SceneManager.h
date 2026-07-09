#pragma once
#include <Core/Core.h>
#include <memory>
#include "Scene.h"

namespace FINALITY
{
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
		void Update(float ts) const;
		void Shutdown() const;

		void SetScene(std::unique_ptr<Scene> scene);
		void ChangeScene();
	private:
		SceneManager() {};
		~SceneManager() = default;

		std::unique_ptr<Scene> m_CurrentScene = std::make_unique<FINALITY::EmptyScene>();
		std::unique_ptr<Scene> m_NextScene = nullptr;
	};
}