#pragma once
#include "vec.h"
struct Hands {
    Pose left{}, right{}, head{};
    bool lGrip=false, rGrip=false, lTrig=false, rTrig=false;
    float lTrigVal=0, rTrigVal=0;
    float lGripVal=0, rGripVal=0;
    float lStickX=0, rStickX=0, lStickY=0, rStickY=0;
    const char* backend;
};
Hands ReadHands();
void ResolveXrHooks();
