#pragma once
#include "vec3.h"
#include "ray.h"
#include "random.h"
#include <cmath>

constexpr double PI = 3.141592653589793238463;
class Camera {
public:
    Camera(Vec3 lookfrom, Vec3 lookat, Vec3 vup, float vfov, float aspect,
        float aperture, float focus_dist) :  lookfrom(lookfrom), lookat(lookat), vup(vup), vfov(vfov), aspect(aspect), aperture(aperture), focus_dist(focus_dist)
    {
        calculate();
    }

    void calculate() {
        lens_radius = aperture / 2;
        float theta = vfov * PI / 180;
        float half_height = tan(theta / 2);
        float half_width = aspect * half_height;
        origin = lookfrom;
        w = unit_vector(lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);
        lower_left_corner = origin
            - half_width * focus_dist * u
            - half_height * focus_dist * v
            - focus_dist * w;
        horizontal = 2 * half_width * focus_dist * u;
        vertical = 2 * half_height * focus_dist * v;
    }

    Ray getRay(float s, float t) const noexcept {
        Vec3 rd = lens_radius * random_in_unit_disk();
        Vec3 offset = u * rd.x() + v * rd.y();
        return Ray(origin + offset,
            lower_left_corner + s * horizontal + t * vertical
            - origin - offset);
    }

    Vec3 lookfrom, lookat, vup;
    float vfov, aspect, aperture;
    float focus_dist;
private:
    Vec3 origin;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w;
    float lens_radius;
};
