#pragma once
#include <memory>
#include <optional>
#include <vector>
#include "vec3.h"
#include "ray.h"
#include "materials.h"
#include "hit_info.h"
#include "aabb.h"

class Object
{
public:
    [[nodiscard]] virtual std::optional<HitInfo> hit(const Ray& r, float t_min, float t_max) const noexcept = 0;
    [[nodiscard]] virtual bool bounding_box(AABB& box) const noexcept = 0;
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
    [[nodiscard]] bool bounding_box(AABB& box) const noexcept override {
        box = AABB(center - Vec3(radius, radius, radius),
            center + Vec3(radius, radius, radius));
        return true;
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

class bvh_node;
class ObjectGroup : public Object
{
public:
    template<class T, class... Args >
    void addObject(Args... args)
    {
        objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
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
    [[nodiscard]] bool bounding_box(AABB& box) const noexcept override {
        if (objects.empty()) return false;
        AABB temp_box;
        bool first_true = objects[0]->bounding_box(temp_box);
        if (!first_true)
            return false;
        
        box = temp_box;
        for (int i = 1; i < objects.size(); i++) {
            if (objects[i]->bounding_box(temp_box)) {
                box = surrounding_box(box, temp_box);
            }
            else
                return false;
        }
        return true;
    }

    std::shared_ptr<bvh_node> to_bvh_node();
private:
    std::vector<std::shared_ptr<Object>> objects;
};

class bvh_node : public Object {
public:
    bvh_node() {}
    bvh_node(std::vector<std::shared_ptr<Object>> l) {
        int axis = int(3 * random_double());

        if (axis == 0) std::sort(l.begin(), l.end(), [](std::shared_ptr<Object> a, std::shared_ptr<Object> b) {
            AABB box_left, box_right;
            if (!a->bounding_box(box_left) || !b->bounding_box(box_right))
                std::cerr << "no bounding box in bvh_node constructor\n";
            return box_left.min().x() - box_right.min().x() < 0.0?-1:1;
        });
        else if (axis == 1) std::sort(l.begin(), l.end(), [](std::shared_ptr<Object> a, std::shared_ptr<Object> b) {
            AABB box_left, box_right;
            if (!a->bounding_box(box_left) || !b->bounding_box(box_right))
                std::cerr << "no bounding box in bvh_node constructor\n";
            return box_left.min().y() - box_right.min().y() < 0.0 ? -1 : 1;
            });
        else std::sort(l.begin(), l.end(), [](std::shared_ptr<Object> a, std::shared_ptr<Object> b) {
            AABB box_left, box_right;
            if (!a->bounding_box(box_left) || !b->bounding_box(box_right))
                std::cerr << "no bounding box in bvh_node constructor\n";
            return box_left.min().z() - box_right.min().z() < 0.0 ? -1 : 1;
            });

        if (l.size() == 1) {
            left = right = l[0];
        }
        else if (l.size() == 2) {
            left = l[0];
            right = l[1];
        }
        else {
            left = std::make_shared<bvh_node>(std::vector<std::shared_ptr<Object>>{ l.begin(), l.begin()+ l.size() / 2 });
            right = std::make_shared<bvh_node>(std::vector<std::shared_ptr<Object>>{ l.begin() + l.size() / 2, l.end() });
        }

        AABB box_left, box_right;

        if (!left->bounding_box(box_left) ||
            !right->bounding_box(box_right)) {

            std::cerr << "no bounding box in bvh_node constructor\n";
        }

        box = surrounding_box(box_left, box_right);
    }

    [[nodiscard]] std::optional<HitInfo> hit(const Ray& r, float t_min, float t_max) const noexcept override
    {
        if (box.hit(r, t_min, t_max)) {
            auto left_rec = left->hit(r, t_min, t_max), right_rec = right->hit(r, t_min, t_max);
            if (left_rec && right_rec) {
                if (left_rec->t < right_rec->t)
                    return left_rec;
                return right_rec;
            }
            if (left_rec)
                return left_rec;
            if (right_rec)
                return right_rec;
        }
        return {};
    }
    [[nodiscard]] bool bounding_box(AABB& box) const noexcept override {
        box = this->box;
        return true;
    }
private:
    std::shared_ptr<Object> left;
    std::shared_ptr<Object> right;
    AABB box;
};

std::shared_ptr<bvh_node> ObjectGroup::to_bvh_node()
{
    return std::make_shared<bvh_node>(objects);
}
class XYRect : public Object {
public:
    XYRect() {}
    XYRect(float _x0, float _x1, float _y0, float _y1, float _k, std::shared_ptr<Material> mat)
        : x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {};
    [[nodiscard]] std::optional<HitInfo> hit(const Ray& r, float t0, float t1) const noexcept override
    {
        float t = (k - r.origin().z()) / r.direction().z();
        if (t < t0 || t > t1)
            return {};
        float x = r.origin().x() + t * r.direction().x();
        float y = r.origin().y() + t * r.direction().y();
        if (x < x0 || x > x1 || y < y0 || y > y1)
            return {};
        return HitInfo{ t,r.point_at_parameter(t),{0, 0, 1},mp };
    }
    [[nodiscard]] bool bounding_box(AABB& box) const noexcept override {
        box = AABB(Vec3(x0, y0, k - 0.0001), Vec3(x1, y1, k + 0.0001));
        return true;
    }
    std::shared_ptr<Material> mp;
    float x0, x1, y0, y1, k;
};
