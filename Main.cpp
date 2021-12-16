#define SDL_MAIN_HANDLED
#include "SDL.h"

int main(int argc, char* argv[])
{
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_EVERYTHING);
}