#include <stdio.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

#define WIDTH 320
#define HEIGHT 240

double *interpolate(double a, double b, int gap);
void draw();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

double *interpolate(double a, double b, int gap)
{
    double *answer = new double[gap];
    double step = (b - a) / (gap - 1);
    answer[0] = a;
    answer[gap - 1] = b;
    for (int i = 1; i < gap - 1; i++)
    {
        answer[i] = a + (i * step);
    }
    return answer;
}

int main(int argc, const char *argv[])
{
    // double *answer;
    // answer = interpolate(2.2, 8.5, 7);
    // for (int i = 0; i < 7; i++)
    // {
    //     std::cout << " : " << answer[i];
    // }

    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
        update();
        draw();
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}

void draw()
{
    window.clearPixels();
    double *answer;
    answer = interpolate(0, 255, window.width);
    for (int y = 0; y < window.height; y++)
    {
        for (int x = 0; x < window.width; x++)
        {
            float red = (int)answer[x];
            float green = (int)answer[x];
            float blue = (int)answer[x];

            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(x, y, colour);
        }
    }
}

void handleEvent(SDL_Event event)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_LEFT)
            std::cout << "LEFT" << std::endl;
        else if (event.key.keysym.sym == SDLK_RIGHT)
            std::cout << "RIGHT" << std::endl;
        else if (event.key.keysym.sym == SDLK_UP)
            std::cout << "UP" << std::endl;
        else if (event.key.keysym.sym == SDLK_DOWN)
            std::cout << "DOWN" << std::endl;
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
        std::cout << "MOUSE CLICKED" << std::endl;
}

void update()
{
    // Function for performing animation (shifting artifacts or moving the camera)
}
