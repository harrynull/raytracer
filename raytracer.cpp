#include <iostream>
#include <fstream>
#include <optional>
#include "vec3.h"
#include "ray.h"
#include "image.h"
#include "camera.h"
#include "random.h"
#include <vector>
#include "progress_bar.h"
#include "materials.h"
#include "objects.h"
#include "threadpool.h"

using namespace std;

ObjectGroup scene;
constexpr int MaxDepth = 50;
constexpr int Width = 2000;
constexpr int Height = 1000;
constexpr int SampleNumber = 50;


Vec3 color(const Ray& r, Object& world, int depth) {
    if (auto info = world.hit(r, 0.001, std::numeric_limits<float>::max());
        info) {

        Ray scattered;
        Vec3 attenuation;
        if (depth < MaxDepth && info->material->scatter(r, *info, attenuation, scattered)) {
            return attenuation * color(scattered, world, depth + 1);
        }
        return { 0,0,0 };

    }
    float t = 0.5 * (unit_vector(r.direction()).y() + 1.0);
    return (1.0 - t) * Vec3(1.0, 1.0, 1.0) + t * Vec3(0.5, 0.7, 1.0);
}

ObjectGroup randomScene() {
    int n = 500;
    ObjectGroup list;

    list.addObject<Sphere>(Vec3(0, -1000, 0), 1000, make_shared<Lambertian>(Vec3(0.5, 0.5, 0.5)));
    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = random_double();
            Vec3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - Vec3(4, 0.2, 0)).length() > 0.9) {
                if (choose_mat < 0.8) {  // diffuse
                    list.addObject<Sphere>(center, 0.2, make_shared<Lambertian>(Vec3(random_double() * random_double(),
                        random_double() * random_double(),
                        random_double() * random_double())));
                }
                else if (choose_mat < 0.95) { // metal
                    list.addObject<Sphere>(center, 0.2, make_shared<Metal>(Vec3(0.5 * (1 + random_double()),
                        0.5 * (1 + random_double()),
                        0.5 * (1 + random_double())),
                        0.5 * random_double()));
                }
                else {  // glass
                    list.addObject<Sphere>(center, 0.2, make_shared<Dielectric>(1.5));
                }
            }
        }
    }

    list.addObject<Sphere>(Vec3{ 0, 1, 0 }, 1.0, make_shared<Dielectric>(1.5));
    list.addObject<Sphere>(Vec3{ -4, 1, 0 }, 1.0, make_shared<Lambertian>(Vec3{ 0.4, 0.2, 0.1 }));
    list.addObject<Sphere>(Vec3{ 4, 1, 0 }, 1.0, make_shared<Metal>(Vec3{ 0.7, 0.6, 0.5 },0.0));

    return list;
}
int main()
{
    auto lambertian1 = make_shared<Lambertian>(Vec3(0.8, 0.3, 0.3));
    auto lambertian2 = make_shared<Lambertian>(Vec3(0.8, 0.8, 0.0));
    auto metal1 = make_shared<Metal>(Vec3(0.8, 0.6, 0.2), 0.0);
    auto dielectric = make_shared<Dielectric>(1.5);

    /*ObjectGroup group;
    group.addObject<Sphere>(Vec3{ 0, 0, -1 }, .5, lambertian1);
    group.addObject<Sphere>(Vec3{ 0, -100.5, -1 }, 100, lambertian2);
    group.addObject<Sphere>(Vec3{ 1, 0, -1 }, .5, metal1);
    group.addObject<Sphere>(Vec3{ -1, 0, -1 }, .5, dielectric);*/
    auto group = randomScene();

    int sample_number = SampleNumber;
    Image img{ Width, Height };
    Camera camera{ {-2,2,1}, {0,0,-1}, {0,1,0}, 90, float(img.width) / float(img.height),
    0.1, (Vec3{-2,2,1} -Vec3{0,0,-1}).length() };

    ThreadPool tp;
    for (int j = img.height - 1; j >= 0; j--) {
        for (int i = 0; i < img.width; i++) {
            auto task = [i, j, sample_number, &camera, &group, &img]() {
                Vec3 col = { 0,0,0 };

                for (int s = 0; s < sample_number; s++) {
                    float u = float(i + random_double()) / float(img.width);
                    float v = float(j + random_double()) / float(img.height);
                    col += color(camera.getRay(u, v), group, 0);
                }
                col /= sample_number;
                col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

                img.getPixel(i, j).setPixel(col * 255.99);
            };
            tp.addTask(task);
        }
    }
    tp.start(std::thread::hardware_concurrency());

    ProgressBar pb(img.height * img.width, 80);
    while(!tp.finished())
    {
        pb.flush(tp.getCounter());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    writeImage(ofstream("test.ppm"), img);
}

