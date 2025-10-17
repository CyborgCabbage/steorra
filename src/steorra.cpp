#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <memory>
#include <iostream>
#include <vulkan/vulkan.h>

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

int main(int argc, char* args[]) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
		return 0;
	}
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> gWindow(SDL_CreateWindow("steorra", kScreenWidth, kScreenHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE), &SDL_DestroyWindow);
	if (!gWindow) {
		SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
		return 0;
	}
	SDL_ShowWindow(gWindow.get());
	bool quit{ false };
	while (!quit) {
		SDL_Event e{};
		while (SDL_PollEvent(&e) == true) {
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
	}

	SDL_Quit();
	return 0;
}
