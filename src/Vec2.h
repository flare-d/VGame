#pragma once
#include <cmath>
#include <string>
#include <cstdio>

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return Vec2(x / s, y / s); }

    // Compound assignments let gameplay code scale or accumulate vectors in
    // place (dash exit momentum, contact normal accumulation) without a
    // temporary on every step.
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    float len() const { return std::sqrt(x*x + y*y); }
    float lenSq() const { return x*x + y*y; }
    Vec2  norm() const { float l = len(); return l > 1e-6f ? *this / l : Vec2(); }
};

inline float sign(float v) { return (v > 0.0f) - (v < 0.0f); }

inline std::string fmt(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return std::string(buf);
}
