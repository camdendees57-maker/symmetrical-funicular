#pragma once
struct Vec3 { float x,y,z; };
struct Quat { float x,y,z,w; };
struct Pose { Vec3 p; Quat q; bool valid; };
inline Vec3 add(Vec3 a, Vec3 b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 mul(Vec3 a, float s){ return {a.x*s,a.y*s,a.z*s}; }
inline Vec3 sub(Vec3 a, Vec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline float dot(Vec3 a, Vec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b){ return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
inline Vec3 qrot(Quat q, Vec3 v){
    Vec3 u{q.x,q.y,q.z};
    Vec3 t = mul(cross(u,v), 2.f);
    return add(v, add(mul(t,q.w), cross(u,t)));
}
