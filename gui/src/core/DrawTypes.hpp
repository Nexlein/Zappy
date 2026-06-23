#pragma once

#include "raylib.h"

struct Particle {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float size;
    float alpha;  // 1.0 → 0.0
    float delay;  // seconds before particle activates
    bool active = false;
};

struct LineSegment {
    Vector3 a;
    Vector3 b;
    Color color;
    float alpha;
};
