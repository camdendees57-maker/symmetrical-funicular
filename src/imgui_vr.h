#pragma once
#include "vec.h"

enum ImGuiVR_FollowMode {
    IMGUIVR_FOLLOW_LOOK = 0,
    IMGUIVR_FOLLOW_LEFT_HAND = 1
};

void ImGuiVR_Init(float panel_w_m, float panel_h_m, float look_distance_m);
void ImGuiVR_SetHead(const Pose& head);
void ImGuiVR_SetLeftHand(const Pose& hand);
void ImGuiVR_SetLookDistance(float meters);
void ImGuiVR_SetHandOffset(Vec3 local_offset);
void ImGuiVR_SetFollowMode(int mode);
int  ImGuiVR_GetFollowMode();

/* writes column-major 4x4 world matrix for the panel quad */
void ImGuiVR_BuildWorld(float out_world_mtx16[16]);

/* poke-hit test in panel local space. returns true if the SNAP button was hit */
bool ImGuiVR_TryToggleFromPoke(const Pose& board, const Pose& right_hand);
