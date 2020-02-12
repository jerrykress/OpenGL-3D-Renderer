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
void draw(Colour line_colour, vector<vector<Colour>> rgb_values);
void texture_filled_triangle(CanvasTriangle triangle, vector<vector<Colour>> rgb_values);
vector<vector<Colour>> readfile(std::string filename, int width, int height, vector<vector<Colour>> rgb_values);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
void texture_colored_triangle(CanvasPoint intersection, CanvasPoint left, CanvasPoint right, vector<vector<Colour>> rgb_values);
void texture_fillBottomFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, vector<vector<Colour>> rgb_values);
void texture_fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, vector<vector<Colour>> rgb_values);
glm::vec3 *scaled_coordinates(CanvasPoint start, CanvasPoint end);
void fill_from_ppm(CanvasPoint start, CanvasPoint end, vector<vector<Colour>> rgb_values);
void update();
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
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

glm::vec3 *scaled_coordinates(CanvasPoint start, CanvasPoint end)
{
    glm::vec3 point_a = glm::vec3(start.texturePoint.x, start.texturePoint.y, 0);
    glm::vec3 point_b = glm::vec3(end.texturePoint.x, end.texturePoint.y, 0);

    float xDiff = end.x - start.x;
    float yDiff = end.y - start.y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;

    glm::vec3 *answer = new glm::vec3[int(numberOfSteps)];
    answer = interpolate(point_a, point_b, round(numberOfSteps));
    return answer;
}

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

    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
            handleEvent(event);
        update();
        Colour line_colour = Colour(255, 0, 0);
        draw(line_colour, rgb_values);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}

void draw(Colour line_colour, vector<vector<Colour>> rgb_values)
{
    // window.clearPixels();
    // draw_line(line_colour, start, end);
    CanvasPoint a = CanvasPoint(160.0, 10.0);
    CanvasPoint b = CanvasPoint(300.0, 230.0);
    CanvasPoint c = CanvasPoint(10.0, 150.0);

    a.texturePoint = TexturePoint(195.0, 5.0);
    b.texturePoint = TexturePoint(395.0, 380.0);
    c.texturePoint = TexturePoint(65.0, 330.0);

    CanvasTriangle triangle = CanvasTriangle(a, b, c, line_colour);
    texture_filled_triangle(triangle, rgb_values);
    stroke_triangle(triangle);
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

void texture_filled_triangle(CanvasTriangle triangle, vector<vector<Colour>> rgb_values)
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
    texture_colored_triangle(top, middlePoint, bottom, rgb_values);
}
// change all of this
void texture_fillBottomFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, vector<vector<Colour>> rgb_values)
{
    float invslope1 = (v2.x - v1.x) / (v2.y - v1.y);
    float invslope2 = (v3.x - v1.x) / (v3.y - v1.y);

    float curx1 = v1.x;
    float curx2 = v1.x;

    for (int scanlineY = v1.y; scanlineY <= v2.y; scanlineY++)
    {
        // CanvasPoint start = CanvasPoint(int(curx1), round(scanlineY));
        // CanvasPoint end = CanvasPoint(int(curx2), round(scanlineY));
        // fill_from_ppm(start, end, rgb_values);
        curx1 += invslope1;
        curx2 += invslope2;
    }
}
// change all of this

void texture_fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, vector<vector<Colour>> rgb_values)
{
    float invslope1 = (v3.x - v1.x) / (v3.y - v1.y);
    float invslope2 = (v3.x - v2.x) / (v3.y - v2.y);

    float curx1 = v3.x;
    float curx2 = v3.x;

    glm::vec3 *v1_to_v3 = scaled_coordinates(v1, v3);
    glm::vec3 *v2_to_v3 = scaled_coordinates(v2, v3);

    for (int scanlineY = v3.y; scanlineY > v1.y; scanlineY--)
    {
        CanvasPoint start = CanvasPoint(int(curx1), round(scanlineY));
        CanvasPoint end = CanvasPoint(int(curx2), round(scanlineY));
        // fill_from_ppm(start, end, rgb_values);
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}
void fill_from_ppm(CanvasPoint start, CanvasPoint end, vector<vector<Colour>> rgb_values)
{
    //3 interpolations
    glm::vec3 point_a = glm::vec3(start.texturePoint.x, start.texturePoint.y, 0);
    glm::vec3 point_b = glm::vec3(end.texturePoint.x, end.texturePoint.y, 0);

    float xDiff = end.x - start.x;
    float yDiff = end.y - start.y;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    float xStepSize = xDiff / numberOfSteps;
    float yStepSize = yDiff / numberOfSteps;

    glm::vec3 *answer = new glm::vec3[int(numberOfSteps)];
    answer = interpolate(point_a, point_b, round(numberOfSteps));
    int tx_index = 0;
    for (float i = 0.0; i < numberOfSteps; i++)
    {
        float x = start.x + (xStepSize * i);
        float y = start.y + (yStepSize * i);
        float red = rgb_values[round(answer[tx_index][0])][round(answer[tx_index][1])].red;
        float green = rgb_values[round(answer[tx_index][0])][round(answer[tx_index][1])].green;
        float blue = rgb_values[round(answer[tx_index][0])][round(answer[tx_index][1])].blue;
        tx_index++;
        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        window.setPixelColour(round(x), round(y), colour);
    }
}
void texture_colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, vector<vector<Colour>> rgb_values)
{
    if (vt2.y == vt3.y)
    {
        texture_fillBottomFlatTriangle(vt1, vt2, vt3, rgb_values);
    }
    else if (vt1.y == vt2.y)
    {
        texture_fillTopFlatTriangle(vt1, vt2, vt3, rgb_values);
    }
    else
    {
        CanvasPoint v4 = CanvasPoint(
            (vt1.x + (((vt2.y - vt1.y) / (vt3.y - vt1.y)) * (vt3.x - vt1.x))), vt2.y);
        TexturePoint t_v4 = TexturePoint(vt1.texturePoint.x + (((vt2.texturePoint.y - vt1.texturePoint.y) / (vt3.texturePoint.y - vt1.texturePoint.y)) * (vt3.texturePoint.x - vt1.texturePoint.x)), vt2.texturePoint.y);
        v4.texturePoint = t_v4;
        texture_fillBottomFlatTriangle(vt1, vt2, v4, rgb_values);
        texture_fillTopFlatTriangle(vt2, v4, vt3, rgb_values);
    }
    fill_from_ppm(vt1, vt3, rgb_values);
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
vector<vector<Colour>> readfile(std::string filename, int width, int height, vector<vector<Colour>> rgb_values)
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
