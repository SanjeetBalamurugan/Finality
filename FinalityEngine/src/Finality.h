#pragma once

// CORE
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Game.h"
#include "Core/RenderCommand.h"
#include "Core/Log.h"
#include "Core/Mesh.h"
#include "Core/Pipeline.h"

// SCENE
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/ScriptableComponent.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

// RENDERER
#include "Renderer/Shader.h"
#include "Renderer/Renderer.h"

// EVENTS
#include "Events/Input.h"
#include "Events/KeyCodes.h"

#define FI_CLIENT_ERROR(...)       ::FINALITY::Logger::GetClientLogger()->error(__VA_ARGS__)
#define FI_CLIENT_WARN(...)        ::FINALITY::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define FI_CLIENT_INFO(...)        ::FINALITY::Logger::GetClientLogger()->info(__VA_ARGS__)