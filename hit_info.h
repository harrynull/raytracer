#pragma once

class Material;
struct HitInfo
{
    float t;
    Vec3 p;
    Vec3 normal;
    std::shared_ptr<Material> material;
};
