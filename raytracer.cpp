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
#include "window.h"
#include "shader.h"

using namespace std;

ObjectGroup scene;
constexpr int MaxDepth = 50;
constexpr int Width = 1900;
constexpr int Height = 1000;
int SampleNumber = 500;
constexpr int TaskBlockSize = 1;

class MainProgram
{
public:
    MainProgram() :w(Width, Height, "ray tracer")
    {
    }

    void run() {
        init();
        w.mainLoop([this]() {render(); });
    }

private:
    static ObjectGroup generateRandomScene() {
        int n = 500;
        ObjectGroup list;

        /*
        group.addObject<Sphere>(Vec3{ 0, 0, -1 }, .5, lambertian1);
        group.addObject<Sphere>(Vec3{ 0, -100.5, -1 }, 100, lambertian2);
        group.addObject<Sphere>(Vec3{ 1, 0, -1 }, .5, metal1);
        group.addObject<Sphere>(Vec3{ -1, 0, -1 }, .5, dielectric);*/

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
        list.addObject<Sphere>(Vec3{ 4, 1, 0 }, 1.0, make_shared<Metal>(Vec3{ 0.7, 0.6, 0.5 }, 0.0));

        return list;
    }
    static Vec3 color(const Ray& r, Object& world, int depth) {
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

    void renderTask(int i_low, int i_high, int j_low, int j_high)
    {
        for(int i=i_low;i<i_high;i++)
        {
            for (int j = j_low; j < j_high; j++)
            {
                Vec3 col = { 0,0,0 };

                for (int s = 0; s < SampleNumber; s++) {
                    float u = float(i + random_double()) / float(img.width);
                    float v = float(j + random_double()) / float(img.height);
                    col += color(camera.getRay(u, v), group, 0);
                }
                col /= SampleNumber;
                col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

                img.getPixel(i, j).setPixel(col * 255.99);
            }
        }
    }

    void startRaytracing()
    {
        cout << "Starting new ray tracing... from "<<camera.lookfrom << " to " <<camera.lookat << " with SampleNumber = " << SampleNumber << endl;
        tp.stop();
        img.clear();

        for (int j = img.height; j >= 0; j -= TaskBlockSize) {
            for (int i = 0; i < img.width; i += TaskBlockSize) {
                auto task = [this, i, j]() {
                    renderTask(i, min(i + TaskBlockSize, int(img.width)),
                        max(j - TaskBlockSize, 0), j);
                };
                tp.addTask(task);
            }
        }

        tp.start(std::thread::hardware_concurrency() - 1);
    }

    void init()
    {
        auto lambertian1 = make_shared<Lambertian>(Vec3(0.8, 0.3, 0.3));
        auto lambertian2 = make_shared<Lambertian>(Vec3(0.8, 0.8, 0.0));
        auto metal1 = make_shared<Metal>(Vec3(0.8, 0.6, 0.2), 0.0);
        auto dielectric = make_shared<Dielectric>(1.5);
        group = generateRandomScene();
        startRaytracing();
    }

    void render()
    {
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //glRasterPos2i(0, 0);
        glDrawPixels(Width, Height, GL_RGB, GL_UNSIGNED_BYTE, img.getBuffer());

        glFlush();

        auto keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_S]) {
            writeImage(ofstream("img.ppm"), img);
            cout << "Image saved" << endl;;
        }
        auto speed = 0.05f;
        if (keys[SDL_SCANCODE_LEFT]) {
            camera.lookat += Vec3(speed,0,0);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            camera.lookat += Vec3(-speed, 0, 0);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_UP]) {
            camera.lookat += Vec3(0, speed, 0);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            camera.lookat += Vec3(0, -speed, 0);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_W]) {
            camera.lookfrom += speed * unit_vector(camera.lookat - camera.lookfrom);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_S]) {
            camera.lookfrom -= speed * unit_vector(camera.lookat - camera.lookfrom);
            camera.calculate();
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_EQUALS]) {
            SampleNumber += 10;
            startRaytracing();
        }
        if (keys[SDL_SCANCODE_MINUS]) {
            SampleNumber -= 10;
            SampleNumber = max(1, SampleNumber);
            startRaytracing();
        }
        pb.flush(tp.getCounter());
    }

    Window w;
    ObjectGroup group;
    ThreadPool tp;
    Image img{ Width, Height };
    Camera camera{ {2,0.9,-2.5}, {1.6,0.9,-1}, {0, 1, 0}, 90, float(img.width) / float(img.height), 0.1 };
    ProgressBar pb{ Width * Height, 80 };
};


int main()
{
    MainProgram prog;
    prog.run();
}

