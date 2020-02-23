#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "vec3.h"
#include "ray.h"
#include "materials.h"
#include "hit_info.h"

class Object
{
public:
    [[nodiscard]] virtual std::optional<HitInfo> hit(const Ray& r, float t_min, float t_max) const noexcept = 0;
};

class Sphere : public Object
{
public:
    Sphere(Vec3 center, float radius, std::shared_ptr<Material> material)
        :center(center), radius(radius), material(std::move(material)) {}

    [[nodiscard]] std::optional<HitInfo> hit(const Ray& r, float t_min, float t_max) const noexcept override
    {
        Vec3 oc = r.origin() - center;
        float a = dot(r.direction(), r.direction());
        float b = dot(oc, r.direction());
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - a * c;
        if (discriminant < 0)
            return {};
        HitInfo rec;
        float temp = (-b - sqrt(discriminant)) / a; // root 1
        if (temp < t_max && temp > t_min)
            return createHitInfo(r, temp);
        temp = (-b + sqrt(discriminant)) / a; // root 2
        if (temp < t_max && temp > t_min)
            return createHitInfo(r, temp);
        return {};
    }

    Vec3 center;
    float radius;
    std::shared_ptr<Material> material;
private:
    HitInfo createHitInfo(const Ray& r, float t) const noexcept
    {
        auto p = r.point_at_parameter(t);
        return HitInfo{ t, p ,(p - center) / radius, material };
    }
};


class ObjectGroup : public Object
{
public:
    template<class T, class... Args >
    void addObject(Args... args)
    {
        objects.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    [[nodiscard]] std::optional<HitInfo> hit(const Ray& r, float t_min, float t_max) const noexcept override
    {
        std::optional<HitInfo> ret = {};
        float closest_so_far = t_max;
        for (auto& obj : objects) {
            if (auto info = obj->hit(r, t_min, closest_so_far); info.has_value()) {
                closest_so_far = info.value().t;
                ret = info.value();
            }
        }
        return ret;
    }
private:
    std::vector<std::unique_ptr<Object>> objects;
};
