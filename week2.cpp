#include <SDL2/SDL.h>
#include <ModelTriangle.h>
#include <CanvasTriangle.h>
// #include <CanvasPoint.h>
#include <DrawingWindow.h>
// #include <Colour.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void stroke_triangle(CanvasTriangle triangle);
void draw(Colour line_colour, CanvasPoint start, CanvasPoint end);
void filled_triangle(CanvasTriangle triangle); 
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
void update();
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size); 
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

int main(int argc, char *argv[])
{
    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
        // if (event.type == SDL_KEYUP)
        // {
        //     Colour line_colour = Colour(255, 255, 255); 
        //     CanvasPoint a = CanvasPoint(float(rand()%320), float(rand()%240));
        //     CanvasPoint b = CanvasPoint(float(rand()%320), float(rand()%240));
        //     CanvasPoint c = CanvasPoint(float(rand()%320), float(rand()%240));

        //     CanvasTriangle triangle = CanvasTriangle(a, b, c, line_colour);
        //     stroke_triangle(triangle);
        // }
        update();
        Colour line_colour = Colour(255, 255, 255);
        CanvasPoint end = CanvasPoint(150.0, 150.0);
        CanvasPoint start = CanvasPoint(20.0, 20.0);
        draw(line_colour, start, end);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}

void draw(Colour line_colour, CanvasPoint start, CanvasPoint end)
{
    // window.clearPixels();
    // draw_line(line_colour, start, end);
    CanvasPoint a = CanvasPoint(150.0, 100.0);
    CanvasPoint b = CanvasPoint(20.0, 20.0);
    CanvasPoint c = CanvasPoint(60.0, 80.0);

    CanvasTriangle triangle = CanvasTriangle(a, b, c, line_colour);
    stroke_triangle(triangle);
    //   for (int y = 0; y < window.height; y++)
    //   {
    //     for (int x = 0; x < window.width; x++)
    //     {
    //       float red = rand() % 255;
    //       float green = 0.0;
    //       float blue = 0.0;
    //       uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
    //       window.setPixelColour(x, y, colour);
    //     }
    //   }
}
void stroke_triangle(CanvasTriangle triangle)
{
    draw_line(triangle.colour, triangle.vertices[0], triangle.vertices[1]);
    draw_line(triangle.colour, triangle.vertices[1], triangle.vertices[2]);
    draw_line(triangle.colour, triangle.vertices[2], triangle.vertices[0]);
}

int indexofSmallestElement(float array[], int size)
{
    int index = 0;

    for(int i = 1; i < size; i++)
    {
        if(array[i] < array[index])
            index = i;              
    }

    return index;
}

int indexofLargestElement(float array[], int size)
{
    int index = 0;

    for(int i = 1; i < size; i++)
    {
        if(array[i] > array[index])
            index = i;              
    }

    return index;
}

void filled_triangle(CanvasTriangle triangle)
{   
    //sort the vertices first 
    float y_vertices[] = { triangle.vertices[0].y, triangle.vertices[1].y, triangle.vertices[2].y }; 
    int index_smallest = indexofSmallestElement(y_vertices, 3); 
    int index_largest = indexofLargestElement(y_vertices, 3); 
    CanvasPoint top = triangle.vertices[index_smallest]; 
    CanvasPoint bottom = triangle.vertices[index_largest]; 
    int index_middle = 0; 
    // find middle value 
    for(int i = 0; i < 3; i++)
    {
        if ((index_smallest != i) && (index_largest != i))
        { 
            index_middle = i; 
        }
    }

    CanvasPoint middlePoint = CanvasPoint()

}
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end)
{
    float xDiff = end.x - start.x;
    float yDiff = end.y - start.y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;
    for (float i = 0.0; i < numberOfSteps; i++)
    {
        float x = start.x + (xStepSize * i);
        float y = start.y + (yStepSize * i);
        float red = line_colour.red;
        float green = line_colour.green;
        float blue = line_colour.blue;
        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        window.setPixelColour(round(x), round(y), colour);
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
