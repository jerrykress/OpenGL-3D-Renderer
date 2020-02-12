#include <SDL2/SDL.h>
#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void draw();
void update();
void handleEvent(SDL_Event event);
void readfile(std::string filename, int width, int height, DrawingWindow window);

// DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

int main(int argc, char *argv[])
{
    std::string filename = "texture.ppm";
    std::ifstream file(filename.c_str());
    int width = 0;
    int height = 0;
    if (file.is_open())
    {
        std::string line;
        for (int i = 0; i < 3; i++)
        {
            std::getline(file, line);
            if (i == 2)
            {
                width = std::stoi(line.substr(0, 3));
                height = std::stoi(line.substr(4, 6));
            }
        }
    }

    // get width and height first
    DrawingWindow window = DrawingWindow(width, height, false);

    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
        readfile(filename, width, height, window);
        update();
        // draw();
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}

void draw()
{
    // readfile("texture.ppm");
}

void readfile(std::string filename, int width, int height, DrawingWindow window)
{
    std::ifstream file(filename.c_str());

    if (file.is_open())
    {
        // negate first four lines
        std::string line;
        for (int i = 0; i < 4; i++)
        {
            std::getline(file, line);
        }
        int counter = 0;
        Colour pixel = Colour(0, 0, 0);
        int x = 0;
        int y = 0;
        char byte;
        while (file.get(byte))
        {
            if (isspace((int)byte))
            {
                std::cout << "SPACE";
                continue;
            }
            else if (counter == 0)
            {
                pixel.red = (unsigned int)byte;
                counter++;
            }
            else if (counter == 1)
            {
                pixel.green = (unsigned int)byte;
                counter++;
            }
            else
            {
                pixel.blue = (unsigned int)byte;
                uint32_t colour = (255 << 24) + (int(pixel.red) << 16) + (int(pixel.green) << 8) + int(pixel.blue);

                window.setPixelColour(x, y, colour);
                if (x > width - 1)
                {
                    x = 0;
                    if (y > height - 1)
                    {
                        y = 0;
                    }
                    else
                    {
                        y++;
                    }
                }
                else
                {
                    x++;
                }
                counter = 0;
            }
            // process pair (a,b)
        }
    }
}
void update()
{
    // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_LEFT)
            cout << "LEFT" << endl;
        else if (event.key.keysym.sym == SDLK_RIGHT)
            cout << "RIGHT" << endl;
        else if (event.key.keysym.sym == SDLK_UP)
            cout << "UP" << endl;
        else if (event.key.keysym.sym == SDLK_DOWN)
            cout << "DOWN" << endl;
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
        cout << "MOUSE CLICKED" << endl;
}
