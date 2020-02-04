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
#define HEIGHT 255

glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap);
void draw();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap)
{
    glm::vec3 *answer = new glm::vec3[gap];
    double step1 = (b[0] - a[0]) / (gap - 1);
    double step2 = (b[1] - a[1]) / (gap - 1);
    double step3 = (b[2] - a[2]) / (gap - 1);

    // first value and last value
    answer[0] = a;
    answer[gap - 1] = b;
    for (int i = 1; i < gap - 1; i++)
    {
        double val1 = a[0] + (i * step1);
        double val2 = a[1] + (i * step2);
        double val3 = a[2] + (i * step3);
        glm::vec3 temp = glm::vec3(val1, val2, val3);
        answer[i] = temp;
    }
    return answer;
}

int main(int argc, const char *argv[])
{
    // glm::vec3 from(1, 4, 9.2);
    // glm::vec3 to(4, 1, 9.8);
    // // std::cout << " (" << from[0] << "," << from[1] << "," << from[2] << " ), ";
    // glm::vec3 *answer = interpolate(from, to, 4);
    // for (int i = 0; i < 4; i++)
    // {
    //     std::cout << " (" << answer[i][0] << "," << answer[i][1] << "," << answer[i][2] << " ), ";
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
    for (int y = 0; y < window.height; y++)
    {
        //( R, G, B)
        glm::vec3 from(255, y, 0);
        glm::vec3 to(0, y, 255 - y);
        glm::vec3 *answer = interpolate(from, to, window.width);
        for (int x = 0; x < window.width; x++)
        {
            float red = (int)answer[x][0];
            float green = (int)answer[x][1];
            float blue = (int)answer[x][2];

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
