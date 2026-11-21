/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ImGuizmoApi.h.
 */
#pragma once

#include <imgui.h>
#pragma push_macro("IMGUI_API")
#undef IMGUI_API
#define IMGUI_API
#include <ImGuizmo.h>
#pragma pop_macro("IMGUI_API")
