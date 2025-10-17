/*This source code copyrighted by Lazy Foo' Productions 2004-2025
and may not be redistributed without written permission.*/

/* Headers */
//Using SDL and STL string
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <memory>

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

int main(int argc, char* args[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> gWindow(SDL_CreateWindow("steorra", kScreenWidth, kScreenHeight, 0), &SDL_DestroyWindow);
    if (!gWindow) {
        SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 0;
    }
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> gScreenSurface(SDL_GetWindowSurface(gWindow.get()), &SDL_DestroySurface);
    if (!gScreenSurface) {
        SDL_Log("SDL_GetWindowSurface failed: %s\n", SDL_GetError());
        return 0;
    }
    bool quit{ false };
    while (!quit) {
        SDL_Event e{};
        while (SDL_PollEvent(&e) == true) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }
        SDL_FillSurfaceRect(gScreenSurface.get(), nullptr, SDL_MapSurfaceRGB(gScreenSurface.get(), 0xFF, 0xFF, 0xFF));
        SDL_UpdateWindowSurface(gWindow.get());
    }

    SDL_Quit();
    return 0;
}
