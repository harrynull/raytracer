#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <string>
#include <GL/glew.h>

class Window
{
public:
    Window(size_t width, size_t height, std::string_view title) : width(width), height(height)
    {
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        ctx = SDL_GL_CreateContext(window);
        glewExperimental = 1;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        glewInit();   
    }

    Window(const Window&) = delete;

    ~Window()
    {
        SDL_GL_DeleteContext(ctx);
        SDL_DestroyWindow(window);
    }

    void mainLoop(std::function<void()> render)
    {
        bool shouldQuit = false;
        while(!shouldQuit)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                switch (e.type) {
                case SDL_QUIT:
                    shouldQuit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (e.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        width = e.window.data1;
                        height = e.window.data2;
                        break;
                    }
                    break;
                }
            }

            render();

            SDL_GL_SwapWindow(window);
        }
    }
private:
    SDL_Window* window;
    SDL_GLContext ctx;
    size_t width, height;
};
