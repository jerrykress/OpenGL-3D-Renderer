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
void colored_triangle(CanvasPoint intersection, CanvasPoint left, CanvasPoint right);
void update();
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void handleEvent(SDL_Event event);
glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap);

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
        Colour line_colour = Colour(255, 0, 0);
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
    filled_triangle(triangle);

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

    for (int i = 1; i < size; i++)
    {
        if (array[i] < array[index])
            index = i;
    }

    return index;
}

int indexofLargestElement(float array[], int size)
{
    int index = 0;

    for (int i = 1; i < size; i++)
    {
        if (array[i] > array[index])
            index = i;
    }

    return index;
}

void filled_triangle(CanvasTriangle triangle)
{
    //sort the vertices first
    float y_vertices[] = {triangle.vertices[0].y, triangle.vertices[1].y, triangle.vertices[2].y};
    int index_smallest = indexofSmallestElement(y_vertices, 3);
    int index_largest = indexofLargestElement(y_vertices, 3);
    CanvasPoint top = triangle.vertices[index_smallest];
    CanvasPoint bottom = triangle.vertices[index_largest];
    int index_middle = 0;
    // find middle value
    for (int i = 0; i < 3; i++)
    {
        if ((index_smallest != i) && (index_largest != i))
        {
            index_middle = i;
        }
    }
    CanvasPoint middlePoint = triangle.vertices[index_middle];
    //interpolation
    // http://www.hugi.scene.org/online/coding/hugi%2017%20-%20cotriang.htm
    float t = (middlePoint.y - top.y) / (bottom.y - top.y);
    float d_x = top.x + (t * (bottom.x - top.x));
    CanvasPoint intersectedPoint = CanvasPoint(d_x, middlePoint.y);

    //test to draw triangles
    CanvasTriangle top_tri = CanvasTriangle(top, intersectedPoint, middlePoint);
    CanvasTriangle bottom_tri = CanvasTriangle(bottom, middlePoint, intersectedPoint);

    // stroke_triangle(top_tri);
    // stroke_triangle(bottom_tri);

    //draw the top one
    colored_triangle(top, middlePoint, intersectedPoint);
    // //draw the bottom one
    colored_triangle(bottom, middlePoint, intersectedPoint);
}
void colored_triangle(CanvasPoint intersection, CanvasPoint left, CanvasPoint right)
{
    if (left.x > right.x)
    {
        CanvasPoint temp = left;
        left = right;
        right = temp;
    }

    //draw left line first
    float start_x = intersection.x;
    float start_y = intersection.y;
    float end_x = left.x;
    float end_y = left.y;
    if (intersection.y > left.y)
    {
        start_x = left.x;
        start_y = left.y;
        end_x = intersection.x;
        end_y = intersection.y;
    }

    float xDiff = end_x - start_x;
    float yDiff = end_y - start_y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;

    float start_right_x = intersection.x;
    float start_right_y = intersection.y;
    float end_right_x = right.x;
    float end_right_y = right.y;

    if (intersection.y > right.y)
    {
        start_right_x = right.x;
        start_right_y = right.y;
        end_right_x = intersection.x;
        end_right_y = intersection.y;
    }

    //calculate point for right side
    float right_xDiff = end_right_x - start_right_x;
    float right_yDiff = end_right_y - start_right_y;
    float right_numberOfSteps = std::max(abs(right_xDiff), abs(right_yDiff));
    float right_yStepSize = right_yDiff / right_numberOfSteps;
    float right_xStepSize = right_xDiff / right_numberOfSteps;
    //in each iteration it must draw the left line, then draw until the right
    for (float i = 0.0; i < numberOfSteps; i++)
    {
        float x = start_x + (xStepSize * i);
        float y = start_y + (yStepSize * i);
        float right_y = start_right_y + (right_yStepSize * i);
        float right_x = start_right_x + (right_xStepSize * i);

        float red = 255;
        float green = 255;
        float blue = 255;
        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        // for (float j = x; j < right_x + 1; j++)
        // {
        //     window.setPixelColour(round(j), round(y), colour);
        // }
        window.setPixelColour(round(right_x), round(right_y), colour);
        window.setPixelColour(round(x), round(y), colour);
    }
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
