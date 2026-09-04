#pragma once
#include "vec.h"
struct Hands {
    Pose left{}, right{};
    bool lGrip=false, rGrip=false, lTrig=false, rTrig=false;
    float lStickX=0, rStickX=0;
};
Hands ReadHands();
