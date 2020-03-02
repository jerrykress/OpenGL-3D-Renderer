#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <math.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#define WIDTH 1024
#define HEIGHT 768

std::vector<std::string> split(std::string str, char delimiter);
std::vector<ModelTriangle> load_obj(std::string filename);
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth, std::vector<std::vector<int>> camera_movement);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end, std::vector<std::vector<int>> camera_movement);
glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap);
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle);
void display_obj(std::string filename, float canvasDepth, std::vector<std::vector<int>> camera_movement);
std::vector<std::vector<int>> camera_movement(float horizontal, float vertical, float depth);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
Colour white = Colour(255, 255, 255);

std::vector<glm::vec3> interpolate_3d(glm::vec3 from, glm::vec3 to, int size)
{
    std::vector<glm::vec3> l;
    float t = (1.0 / float(size));
    float t_iter = 0;
    for (int i = 0; i < size + 1; i++)
    {
        l.push_back((from * glm::vec3(1 - t_iter,
                                      1 - t_iter,
                                      1 - t_iter)) +
                    to * glm::vec3(t_iter, t_iter, t_iter));
        t_iter = t * i;
    }
    return l;
}
std::vector<std::string> split(std::string str, char delimiter)
{
    std::vector<std::string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    std::string tok;

    while (getline(ss, tok, delimiter))
    {
        internal.push_back(tok);
    }

    return internal;
}

// MLT = value from 0 to 1 so RGB = 255 * MLT value
std::vector<ModelTriangle> load_obj(std::string filename)
{
    //lines from obj file
    std::vector<glm::vec3> vs;
    std::vector<glm::vec3> fs;
    //constructed elements
    std::vector<ModelTriangle> faces;
    //metadata
    std::ifstream file(filename.c_str());

    //read in entire file line by line
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.length() < 1)
                continue;
            line.erase(std::remove(line.begin(), line.end(), '/'), line.end());

            std::vector<std::string> chunks = split(line, ' ');
            //find if it's a vertex or a face
            if (chunks[0] == "v")
            {
                vs.push_back(glm::vec3(std::stof(chunks[1]), std::stof(chunks[2]), std::stof(chunks[3])));
            }

            if (chunks[0] == "f")
            {
                fs.push_back(glm::vec3(std::stoi(chunks[1]), std::stoi(chunks[2]), std::stoi(chunks[3])));
            }
        }
    }
    //build faces from vertices
    for (glm::vec3 f : fs)
    {

        faces.push_back(ModelTriangle(vs[f.x - 1], vs[f.y - 1], vs[f.z - 1], white));
    }

    return faces;
}

std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth, std::vector<std::vector<int>> camera_movement)
{
    std::vector<CanvasTriangle> projected;
    int focal = (HEIGHT / 1.75);
    //rotate
    for (ModelTriangle face : faces)
    {
        //each triangle coordinates
        for (int i = 0; i < 3; i++)
        {
            int angel_x = camera_movement[1][0];
            int angel_y = camera_movement[1][1];
            int angel_z = camera_movement[1][2];
            // rotate by x-axis
            glm::vec3 vertices = glm::vec3(face.vertices[i].x, face.vertices[i].y, face.vertices[i].z);

            if (angel_x > 0)
            {
                glm::mat3 rotationMatrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, cos(angel_x), -sin(angel_x)),
                                         glm::vec3(0.0, sin(angel_x), cos(angel_x)));
                glm::vec3 rotated = vertices * rotationMatrix;

                face.vertices[i].x = rotated[0];
                face.vertices[i].y = rotated[1];
                face.vertices[i].z = rotated[2];
            }

            if (angel_y > 0)
            {
                // rotate by y-axis
                glm::mat3 rotationMatrix(glm::vec3(cos(angel_y), 0.0, sin(angel_y)), glm::vec3(0.0, 1.0, 0.0),
                                         glm::vec3(-sin(angel_y), 0.0, cos(angel_y)));
                glm::vec3 rotated = vertices * rotationMatrix;
                face.vertices[i].x = rotated[0];
                face.vertices[i].y = rotated[1];
                face.vertices[i].z = rotated[2];
            }

            if (angel_z > 0)
            {
                // rotate by y-axis
                glm::mat3 rotationMatrix(glm::vec3(cos(angel_z), -sin(angel_z), 0.0), glm::vec3(sin(angel_z), cos(angel_z), 0.0),
                                         glm::vec3(0.0, 0.0, 1.0));
                glm::vec3 rotated = vertices * rotationMatrix;
                face.vertices[i].x = rotated[0];
                face.vertices[i].y = rotated[1];
                face.vertices[i].z = rotated[2];
            }
        }
    }
    for (ModelTriangle face : faces)
    {

        projected.push_back(CanvasTriangle(CanvasPoint(face.vertices[0].x * focal / (face.vertices[0].z + depth), (face.vertices[0].y * -1) * focal / (face.vertices[0].z + depth)),
                                           CanvasPoint(face.vertices[1].x * focal / (face.vertices[1].z + depth), (face.vertices[1].y * -1) * focal / (face.vertices[1].z + depth)),
                                           CanvasPoint(face.vertices[2].x * focal / (face.vertices[2].z + depth), (face.vertices[2].y * -1) * focal / (face.vertices[2].z + depth))));
    }

    return projected;
}

void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end, std::vector<std::vector<int>> camera_movement)
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
        window.setPixelColour(round(x + (WIDTH / 2) + camera_movement[0][0]), round(y + (HEIGHT / 1.25) + camera_movement[0][1]), colour);
    }
}

void stroke_triangle(CanvasTriangle triangle, std::vector<std::vector<int>> camera_movement)
{
    draw_line(triangle.colour, triangle.vertices[0], triangle.vertices[1], camera_movement);
    draw_line(triangle.colour, triangle.vertices[1], triangle.vertices[2], camera_movement);
    draw_line(triangle.colour, triangle.vertices[2], triangle.vertices[0], camera_movement);
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

void display_obj(std::string filename, float canvasDepth, std::vector<std::vector<int>> camera_movement)
{
    std::vector<CanvasTriangle> triangles = project(load_obj(filename), canvasDepth, camera_movement);

    for (CanvasTriangle triangle : triangles)
    {
        stroke_triangle(triangle, camera_movement);
    }
}

std::vector<std::vector<int>> camera_movement(float horizontal, float vertical, float depth)
{
    std::vector<std::vector<int>> camera_movement(3);
    for (int i = 0; i < 3; i++)
    {
        camera_movement[i].resize(3);
    }
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            camera_movement[i][j] = 0;
        }
    }
    camera_movement[0][0] = horizontal;
    camera_movement[0][1] = vertical;
    camera_movement[0][2] = depth;

    // camera_movement[1][0] = 200;
    camera_movement[1][1] = 180;

    return camera_movement;
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
        {
            handleEvent(event);
        }
        std::vector<std::vector<int>> cam_position = camera_movement(30, 0, 10);
        display_obj("cornell.obj", 10, cam_position);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }

    return 0;
}