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

#define WIDTH 1024
#define HEIGHT 768

void stroke_triangle(CanvasTriangle triangle);
void draw(Colour line_colour, CanvasPoint start, CanvasPoint end);
void filled_triangle(CanvasTriangle triangle);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
void colored_triangle(CanvasPoint intersection, CanvasPoint left, CanvasPoint right, Colour color);
void fillBottomFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color);
void fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color);
int calculate_gap(CanvasPoint endpoint1, CanvasPoint endpoint2, CanvasPoint intersection);
void update();
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap)
{
    glm::vec3 *answer = new glm::vec3[gap];
    double step1 = (b[0] - a[0]) / (gap);
    double step2 = (b[1] - a[1]) / (gap);
    double step3 = (b[2] - a[2]) / (gap);

    // first value and last value
    answer[0] = a;
    answer[gap - 1] = b;
    for (int i = 1; i < gap; i++)
    {
        double val1 = a[0] + (i * step1);
        double val2 = a[1] + (i * step2);
        double val3 = a[2] + (i * step3);
        glm::vec3 temp = glm::vec3(val1, val2, val3);
        answer[i] = temp;
    }
    return answer;
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
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
    CanvasPoint a = CanvasPoint(50.0, 50.0);
    CanvasPoint b = CanvasPoint(50.0, 350.0);
    CanvasPoint c = CanvasPoint(500.0, 50.0);

    CanvasTriangle triangle = CanvasTriangle(a, b, c, line_colour);
    stroke_triangle(triangle);
    filled_triangle(triangle);
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
    Colour tri_color = Colour(255, 255, 255);
    colored_triangle(top, middlePoint, bottom, tri_color);
}
int calculate_gap(CanvasPoint endpoint1, CanvasPoint endpoint2, CanvasPoint intersection)
{
    float xDiff = endpoint1.x - intersection.x;
    float yDiff = endpoint1.y - intersection.y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));

    float xDiff2 = endpoint2.x - intersection.x;
    float yDiff2 = endpoint2.y - intersection.y;
    float numberOfSteps2 = std::max(abs(xDiff2), abs(yDiff2));

    if (numberOfSteps2 > numberOfSteps)
    {
        return round(numberOfSteps2);
    }
    else
    {
        return round(numberOfSteps);
    }
}
// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html source
void fillBottomFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color)
{
    // interpolate(glm::vec3 a, glm::vec3 b, int gap)
    int gap = calculate_gap(v2, v3, v1);
    glm::vec3 from = glm::vec3(v1.x, v1.y, 1);
    glm::vec3 to = glm::vec3(v2.x, v2.y, 1);

    glm::vec3 *answer_left = new glm::vec3[gap];
    answer_left = interpolate(from, to, gap);

    to = glm::vec3(v3.x, v3.y, 1);
    glm::vec3 *answer_right = new glm::vec3[gap];
    answer_right = interpolate(from, to, gap);

    for (int i = 0; i < gap; i++)
    {
        CanvasPoint start = CanvasPoint(int(answer_left[i][0]), round(answer_left[i][1]));
        CanvasPoint end = CanvasPoint(int(answer_right[i][0]), round(answer_right[i][1]));
        draw_line(color, start, end);
    }
}
void fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color)
{
    // interpolate(glm::vec3 a, glm::vec3 b, int gap)
    int gap = calculate_gap(v2, v3, v1);
    glm::vec3 from = glm::vec3(v3.x, v3.y, 1);
    glm::vec3 to = glm::vec3(v1.x, v1.y, 1);

    glm::vec3 *answer_left = new glm::vec3[gap];
    answer_left = interpolate(from, to, gap);

    to = glm::vec3(v2.x, v2.y, 1);
    glm::vec3 *answer_right = new glm::vec3[gap];
    answer_right = interpolate(from, to, gap);

    for (int i = 0; i < gap; i++)
    {
        CanvasPoint start = CanvasPoint(int(answer_left[i][0]), round(answer_left[i][1]));
        CanvasPoint end = CanvasPoint(int(answer_right[i][0]), round(answer_right[i][1]));
        draw_line(color, start, end);
    }
}
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color)
{

    float red = color.red;
    float green = color.green;
    float blue = color.blue;
    uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
    if (vt2.y == vt3.y)
    {
        fillBottomFlatTriangle(vt1, vt2, vt3, color);
    }
    else if (vt1.y == vt2.y)
    {
        fillTopFlatTriangle(vt1, vt2, vt3, color);
    }
    else
    {
        CanvasPoint v4 = CanvasPoint(
            (vt1.x + (((vt2.y - vt1.y) / (vt3.y - vt1.y)) * (vt3.x - vt1.x))), vt2.y);
        fillBottomFlatTriangle(vt1, vt2, v4, color);
        draw_line(color, vt2, v4);
        window.setPixelColour(round(v4.x), round(v4.y), colour);
        fillTopFlatTriangle(vt2, v4, vt3, color);
    }

    draw_line(color, vt1, vt3);
    draw_line(color, vt2, vt3);
    draw_line(color, vt1, vt2);
    window.setPixelColour(round(vt1.x), round(vt1.y), colour);
    window.setPixelColour(round(vt2.x), round(vt2.y), colour);
    window.setPixelColour(round(vt3.x), round(vt3.y), colour);
}
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end)
{
    float xDiff = end.x - start.x;
    float yDiff = end.y - start.y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    if (numberOfSteps > 0)
    {
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
    else
    {
        float red = line_colour.red;
        float green = line_colour.green;
        float blue = line_colour.blue;
        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        window.setPixelColour(round(start.x + (WIDTH / 2)), round(start.y + (HEIGHT / 1.3)), colour);
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
