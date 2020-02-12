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

// extern vector<vector<Colour>> rgb_values;
void draw(int width, int height, DrawingWindow window, vector<vector<Colour>> rgb_values);
void update();
void handleEvent(SDL_Event event);
// void readfile(std::string filename, int width, int height, DrawingWindow window);
vector<vector<Colour>> readfile(std::string filename, int width, int height, vector<vector<Colour>> rgb_values);
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
    file.close();
    vector<vector<Colour>> rgb_values(width);
    for (int i = 0; i < width; i++)
    {
        rgb_values[i].resize(height);
    }
    rgb_values = readfile(filename, width, height, rgb_values);
    // std::cout << width << " x " << height;
    // get width and height first
    DrawingWindow window = DrawingWindow(width, height, false);

    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
        update();
        draw(width, height, window, rgb_values);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}

void draw(int width, int height, DrawingWindow window, vector<vector<Colour>> rgb_values)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // std::cout << "RGB" << rgb_values[x][y].red << " , " << rgb_values[x][y].green << " , " << rgb_values[x][y].blue << "\n";
            uint32_t colour = (255 << 24) + (int(rgb_values[x][y].red) << 16) + (int(rgb_values[x][y].green) << 8) + int(rgb_values[x][y].blue);
            window.setPixelColour(x, y, colour);
        }
    }
    // readfile("texture.ppm");
}

vector<vector<Colour>> readfile(std::string filename, int width, int height, vector<vector<Colour>> rgb_values)
{
    std::ifstream file(filename.c_str());
    if (file.is_open())
    {
        // negate first four lines
        std::string line;
        for (int i = 0; i < 4; i++)
        {
            std::cout << line << "\n";
            std::getline(file, line);
        }
        // exit(EXIT_FAILURE);
        int counter = 0;
        Colour pixel = Colour(0, 0, 0);
        int x = 0;
        int y = 0;
        char byte;
        while (file.get(byte))
        {
            if (counter == 0)
            {
                pixel.red = (unsigned char)byte;
                counter++;
            }
            else if (counter == 1)
            {
                pixel.green = (unsigned char)byte;
                counter++;
            }
            else
            {
                pixel.blue = (unsigned char)byte;

                rgb_values[x][y] = pixel;
                if (x > width - 2)
                {
                    
                    x = 0;
                    if (y > height - 1)
                    {
                        return rgb_values;
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
    return rgb_values;
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
