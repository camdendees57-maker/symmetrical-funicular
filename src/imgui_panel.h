#pragma once
#include "vec.h"
void ImGuiPanel_Init();
void ImGuiPanel_Frame(const Pose& head, const Pose& left, const Pose& right, bool poke);
