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
#include <map>

#define WIDTH 1024
#define HEIGHT 768

// triangle functions
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle, Colour tri_color);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);

//camera movement functions
void display_obj(std::string filename, float canvasDepth, std::vector<std::vector<int>> camera_movement);
std::vector<std::vector<int>> camera_movement(float horizontal, float vertical, float depth);
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth, std::vector<std::vector<int>> camera_movement);

//load color, obj and interpolate
std::map<int, std::string> load_colour(std::string filename);
std::map<std::string, Colour> load_mtl(std::string filename);
std::vector<std::string> split(std::string str, char delimiter);
std::vector<ModelTriangle> load_obj(std::string filename);
std::vector<glm::vec3> interpolate_3d(glm::vec3 from, glm::vec3 to, int size);

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

std::map<std::string, Colour> load_mtl(std::string filename)
{
    //metadata
    std::ifstream file(filename.c_str());
    //mtl lookup table
    std::map<std::string, Colour> mtl;
    //parsing info
    std::vector<std::string> mtlName;
    //flag for checking parsing
    bool parsingMtl = false;

    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (parsingMtl)
            {
                if (line.length() < 1)
                {
                    throw "Empty material found!";
                }

                std::vector<std::string> mtlVal = split(line, ' ');

                if (mtlVal.size() != 4 || mtlVal[0] != "Kd")
                {
                    throw "Material definition is not in correct format!";
                }

                Colour mtlColour = Colour(mtlName[1], (int)(std::stof(mtlVal[1]) * 255), (int)(std::stof(mtlVal[2]) * 255), (int)(std::stof(mtlVal[3]) * 255));

                mtl.insert(std::pair<std::string, Colour>(mtlName[1], mtlColour));

                parsingMtl = false;
            }

            if (line.length() < 1)
                continue;

            mtlName = split(line, ' ');

            if (mtlName[0] == "newmtl")
            {
                parsingMtl = true;
                continue;
            }
        }
    }

    return mtl;
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

void filled_triangle(CanvasTriangle triangle, Colour tri_color)
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
    // Colour tri_color = Colour(255, 255, 255);
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

    int gap = calculate_gap(v2, v3, v1);
    glm::vec3 from = glm::vec3(v1.x, v1.y, 1);
    glm::vec3 to = glm::vec3(v2.x, v2.y, 1);

    std::vector<glm::vec3> answer_left = interpolate_3d(from, to, gap);

    to = glm::vec3(v3.x, v3.y, 1);
    std::vector<glm::vec3> answer_right = interpolate_3d(from, to, gap);

    for (int i = 0; i < gap + 1; i++)
    {
        CanvasPoint start = CanvasPoint(answer_left[i][0], answer_left[i][1]);
        CanvasPoint end = CanvasPoint(answer_right[i][0], answer_right[i][1]);
        draw_line(color, start, end);
    }
}
void fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color)
{

    int gap = calculate_gap(v2, v3, v1);
    glm::vec3 to = glm::vec3(v3.x, v3.y, 1);
    glm::vec3 from = glm::vec3(v1.x, v1.y, 1);

    std::vector<glm::vec3> answer_left = interpolate_3d(from, to, gap);

    from = glm::vec3(v2.x, v2.y, 1);
    std::vector<glm::vec3> answer_right = interpolate_3d(from, to, gap);

    for (int i = 0; i < gap + 1; i++)
    {
        CanvasPoint start = CanvasPoint(answer_left[i][0], answer_left[i][1]);
        CanvasPoint end = CanvasPoint(answer_right[i][0], answer_right[i][1]);
        draw_line(color, start, end);
    }
}
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color)
{

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
        fillTopFlatTriangle(vt2, v4, vt3, color);
    }
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

    camera_movement[1][2] = 45;
    // camera_movement[1][1] = 180;

    return camera_movement;
}

std::map<int, std::string> load_colour(std::string filename)
{
    std::map<int, std::string> face_mtl;

    //metadata
    std::ifstream file(filename.c_str());
    std::string currentColour;
    int faceIndex = 0;

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

            if (chunks[0] == "usemtl")
            {
                currentColour = chunks[1];
                continue;
            }

            if (chunks[0] == "f")
            {
                face_mtl.insert(std::pair<int, std::string>(faceIndex, currentColour));
                faceIndex++;
            }
        }
    }

    return face_mtl;
}

std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth, std::vector<std::vector<int>> camera_movement)
{
    std::vector<CanvasTriangle> projected;
    // int focal = (HEIGHT / 1.75);
    int focal = 5;

    //calculate changes of rotation in the camera
    // int angel_x = camera_movement[1][0];
    // int angel_y = camera_movement[1][1];
    // int angel_z = camera_movement[1][2];
    // glm::vec3 vertices = glm::vec3(0.0, 0.0, 0.0);
    // if (angel_x != 0)
    // {
    //     glm::mat3 rotationMatrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, cos(angel_x), -sin(angel_x)),
    //                              glm::vec3(0.0, sin(angel_x), cos(angel_x)));
    //     glm::vec3 rotation = vertices * rotationMatrix;
    //     std::cout << "Angle : " << angel_x << "\n";
    //     std::cout << "COS : " << cos(angel_x) << " SIN : " << sin(angel_x) << "\n";
    // }

    // if (angel_y != 0)
    // {
    //     // rotate by y-axis
    //     glm::mat3 rotationMatrix(glm::vec3(cos(angel_y), 0.0, sin(angel_y)), glm::vec3(0.0, 1.0, 0.0),
    //                              glm::vec3(-sin(angel_y), 0.0, cos(angel_y)));
    //     vertices = vertices * rotationMatrix;
    // }

    // if (angel_z != 0)
    // {
    //     // rotate by y-axis
    //     glm::mat3 rotationMatrix(glm::vec3(cos(angel_z), -sin(angel_z), 0.0), glm::vec3(sin(angel_z), cos(angel_z), 0.0),
    //                              glm::vec3(0.0, 0.0, 1.0));
    //     vertices = vertices * rotationMatrix;
    // }

    // // we map those changes to the vertices
    // for (ModelTriangle face : faces)
    // {
    //     //each triangle coordinates
    //     for (int i = 0; i < 3; i++)
    //     {
    //         if (vertices[0] < 0)
    //         {
    //             face.vertices[i].x = face.vertices[i].x + abs(vertices[0]);
    //         }
    //         else
    //         {
    //             face.vertices[i].x = face.vertices[i].x - vertices[0];
    //         }
    //         if (vertices[1] < 0)
    //         {
    //             face.vertices[i].y = face.vertices[i].y + abs(vertices[1]);
    //         }
    //         else
    //         {
    //             face.vertices[i].y = face.vertices[i].y - vertices[1];
    //         }
    //         if (vertices[0] < 0)
    //         {
    //             face.vertices[i].z = face.vertices[i].z + abs(vertices[2]);
    //         }
    //         else
    //         {
    //             face.vertices[i].z = face.vertices[i].z - vertices[2];
    //         }
    //     }
    // }

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
                // std::cout << "BEFORE -> x : " << face.vertices[i].x << " y : " << face.vertices[i].x << "z : " << face.vertices[i].z << "\n";
                face.vertices[i].x = rotated[0];
                face.vertices[i].y = rotated[1];
                face.vertices[i].z = rotated[2];
                // std::cout << " AFTER -> x : " << face.vertices[i].x << " y : " << face.vertices[i].x << "z : " << face.vertices[i].z << "\n";
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
        projected.push_back(CanvasTriangle(CanvasPoint(face.vertices[0].x * focal / ((face.vertices[0].z * -1) + depth), (face.vertices[0].y * -1) * focal / ((face.vertices[0].z * -1) + depth)),
                                           CanvasPoint(face.vertices[1].x * focal / ((face.vertices[1].z * -1) + depth), (face.vertices[1].y * -1) * focal / ((face.vertices[1].z * -1) + depth)),
                                           CanvasPoint(face.vertices[2].x * focal / ((face.vertices[2].z * -1) + depth), (face.vertices[2].y * -1) * focal / ((face.vertices[2].z * -1) + depth))));
    }

    return projected;
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
            window.setPixelColour(round(x + (WIDTH / 2)), round(y + (HEIGHT / 1.3)), colour);
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

void stroke_triangle(CanvasTriangle triangle)
{
    draw_line(triangle.colour, triangle.vertices[0], triangle.vertices[1]);
    draw_line(triangle.colour, triangle.vertices[1], triangle.vertices[2]);
    draw_line(triangle.colour, triangle.vertices[2], triangle.vertices[0]);
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
    std::map<int, std::string> face_mtl = load_colour("cornell.obj");
    std::map<std::string, Colour> mtls = load_mtl("cornell-box.mtl");

    for (int i = 0; i < triangles.size(); i++)
    {
        // stroke_triangle(triangles[i]);
        filled_triangle(triangles[i], mtls[face_mtl[i]]);
    }
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