#include "app.hpp"

#include <SDL.h>

#include <cstdio>
#include <exception>

int main(int /*argc*/, char* /*argv*/[])
{
    try {
        App app;
        return app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "blockfall: %s\n", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Blockfall", e.what(), nullptr);
        return 1;
    }
}
