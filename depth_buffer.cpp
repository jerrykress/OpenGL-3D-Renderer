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
#define SCALING 60

std::vector<std::string> split(std::string str, char delimiter);
std::vector<ModelTriangle> load_obj(std::string filename);
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
glm::vec3 *interpolate(glm::vec3 a, glm::vec3 b, int gap);
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle, Colour tri_color);
void display_obj(std::string filename, float canvasDepth);
void setPixelColour_(int x, int y, Colour colour);
void clearDepthBuffer(std::string bufferName);
std::map<int, std::string> load_colour(std::string filename);
std::map<std::string, Colour> load_mtl(std::string filename);
std::vector<glm::vec3> interpolate_3d(glm::vec3 from, glm::vec3 to, int size);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
Colour white = Colour(255, 255, 255);

// std::vector<std::vector<float>> *currentDepth = new std::vector<std::vector<float>>(WIDTH);
// std::vector<std::vector<float>> *incomingDepth = new std::vector<std::vector<float>>(WIDTH);

float **currentDepth = new float *[WIDTH];
float **incomingDepth = new float *[WIDTH];

std::map<std::string, Colour>
load_mtl(std::string filename)
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

void setPixelColour_(int x, int y, Colour colour)
{
    if (incomingDepth[x][y] < currentDepth[x][y])
    {
        float red = colour.red;
        float green = colour.green;
        float blue = colour.blue;
        uint32_t uint_color = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);

        window.setPixelColour(x, y, uint_color);
        currentDepth[x][y] = incomingDepth[x][y];
    }
}

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

std::vector<glm::vec3> interpolate_3(glm::vec3 from, glm::vec3 to, int size)
{
    std::vector<glm::vec3> l;
    if (size > 1)
    {
        glm::vec3 distance = to - from;
        float gap = size - 1;
        glm::vec3 step = glm::vec3(distance.x / gap, distance.y / gap, distance.z / gap);

        for (int i = 0; i < (size - 1); i++)
        {
            glm::vec3 currentStep = glm::vec3(from.x + step.x * i,
                                              from.y + step.y * i,
                                              from.z + step.z * i);
            l.push_back(currentStep);
        }
    }

    if (size == 1)
    {
        l.push_back(from);
    }

    return l;
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
    // interpolate(glm::vec3 a, glm::vec3 b, int gap)
    int gap = calculate_gap(v2, v3, v1);
    if (gap > 0)
    {
        glm::vec3 from = glm::vec3(v1.x, v1.y, v1.depth);
        glm::vec3 to = glm::vec3(v2.x, v2.y, v2.depth);

        glm::vec3 *answer_left = new glm::vec3[gap];
        answer_left = interpolate(from, to, gap);

        to = glm::vec3(v3.x, v3.y, v3.depth);
        glm::vec3 *answer_right = new glm::vec3[gap];
        answer_right = interpolate(from, to, gap);

        for (int i = 0; i < gap; i++)
        {
            CanvasPoint start = CanvasPoint(int(answer_left[i][0]), round(answer_left[i][1]), round(answer_left[i][2]));
            CanvasPoint end = CanvasPoint(int(answer_right[i][0]), round(answer_right[i][1]), round(answer_right[i][2]));
            draw_line(color, start, end);
        }
    }
}
void fillTopFlatTriangle(CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour color)
{
    // interpolate(glm::vec3 a, glm::vec3 b, int gap)
    int gap = calculate_gap(v2, v3, v1);
    if (gap > 0)
    {
        glm::vec3 from = glm::vec3(v3.x, v3.y, v3.depth);
        glm::vec3 to = glm::vec3(v1.x, v1.y, v1.depth);

        glm::vec3 *answer_left = new glm::vec3[gap];
        answer_left = interpolate(from, to, gap);

        to = glm::vec3(v2.x, v2.y, v2.depth);
        glm::vec3 *answer_right = new glm::vec3[gap];
        answer_right = interpolate(from, to, gap);

        for (int i = 0; i < gap; i++)
        {
            CanvasPoint start = CanvasPoint(int(answer_left[i][0]), round(answer_left[i][1]), round(answer_left[i][2]));
            CanvasPoint end = CanvasPoint(int(answer_right[i][0]), round(answer_right[i][1]), round(answer_right[i][2]));
            draw_line(color, start, end);
        }
    }
}
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour colour)
{
    if (vt2.y == vt3.y)
    {
        fillBottomFlatTriangle(vt1, vt2, vt3, colour);
    }
    else if (vt1.y == vt2.y)
    {
        fillTopFlatTriangle(vt1, vt2, vt3, colour);
    }
    else
    {
        CanvasPoint v4 = CanvasPoint(
            (vt1.x + (((vt2.y - vt1.y) / (vt3.y - vt1.y)) * (vt3.x - vt1.x))),
            vt2.y,
            (vt1.depth + (((vt2.y - vt1.y) / (vt3.y - vt1.y)) * (vt3.depth - vt1.depth))));
        fillBottomFlatTriangle(vt1, vt2, v4, colour);
        fillTopFlatTriangle(vt2, v4, vt3, colour);
        incomingDepth[int(round(v4.x + (WIDTH / 2)))][int(round(v4.y + (HEIGHT / 1.3)))] = v4.depth;
        setPixelColour_(round(v4.x + (WIDTH / 2)), round(v4.y + (HEIGHT / 1.3)), colour);
    }
    draw_line(colour, vt1, vt3);
    draw_line(colour, vt2, vt3);
    draw_line(colour, vt1, vt2);

    incomingDepth[int(round(vt1.x + (WIDTH / 2)))][int(round(vt1.y + (HEIGHT / 1.3)))] = vt1.depth;
    incomingDepth[int(round(vt2.x + (WIDTH / 2)))][int(round(vt2.y + (HEIGHT / 1.3)))] = vt2.depth;
    incomingDepth[int(round(vt3.x + (WIDTH / 2)))][int(round(vt3.y + (HEIGHT / 1.3)))] = vt3.depth;

    setPixelColour_(round(vt1.x + (WIDTH / 2)), round(vt1.y + (HEIGHT / 1.3)), colour);
    setPixelColour_(round(vt2.x + (WIDTH / 2)), round(vt2.y + (HEIGHT / 1.3)), colour);
    setPixelColour_(round(vt3.x + (WIDTH / 2)), round(vt3.y + (HEIGHT / 1.3)), colour);
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

glm::vec3 camera_rotation(int angle_x, int angle_y, glm::vec3 cameraPosition)
{
    // rotate by x-axis
    glm::vec3 new_cam_position = cameraPosition;
    if (angle_x != 0)
    {
        glm::mat3 rotationMatrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, cos(angle_x), -sin(angle_x)),
                                 glm::vec3(0.0, sin(angle_x), cos(angle_x)));
        new_cam_position = cameraPosition * rotationMatrix;
    }

    if (angle_y != 0)
    {
        // rotate by y-axis
        glm::mat3 rotationMatrix(glm::vec3(cos(angle_y), 0.0, sin(angle_y)), glm::vec3(0.0, 1.0, 0.0),
                                 glm::vec3(-sin(angle_y), 0.0, cos(angle_y)));
        new_cam_position = cameraPosition * rotationMatrix;
    }
    return new_cam_position;
}

std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth)
{
    std::vector<CanvasTriangle> projected;
    int focal = 5 * SCALING;
    glm::vec3 cameraPosition = glm::vec3(0, 1, 3);
    cameraPosition = camera_rotation(0, 45, cameraPosition);
    cameraPosition[2] = cameraPosition[2] + 3;

    for (ModelTriangle face : faces)
    {
        face.vertices[0] = (face.vertices[0] - cameraPosition);
        face.vertices[1] = (face.vertices[1] - cameraPosition);
        face.vertices[2] = (face.vertices[2] - cameraPosition);

        projected.push_back(CanvasTriangle(CanvasPoint(face.vertices[0].x * focal / ((face.vertices[0].z * -1)), (face.vertices[0].y * -1) * focal / ((face.vertices[0].z * -1)), face.vertices[0].z * -1),
                                           CanvasPoint(face.vertices[1].x * focal / ((face.vertices[1].z * -1)), (face.vertices[1].y * -1) * focal / ((face.vertices[1].z * -1)), face.vertices[1].z * -1),
                                           CanvasPoint(face.vertices[2].x * focal / ((face.vertices[2].z * -1)), (face.vertices[2].y * -1) * focal / ((face.vertices[2].z * -1)), face.vertices[2].z * -1)));
    }

    return projected;
}

void draw_line(Colour colour, CanvasPoint start, CanvasPoint end)
{
    float xDiff = end.x - start.x;
    float yDiff = end.y - start.y;
    float zDiff = end.depth - start.depth;
    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
    if (numberOfSteps > 0)
    {
        float xStepSize = xDiff / numberOfSteps;
        float yStepSize = yDiff / numberOfSteps;
        float zStepSize = zDiff / numberOfSteps;

        for (float i = 0.0; i < numberOfSteps; i++)
        {
            float x = start.x + (xStepSize * i);
            float y = start.y + (yStepSize * i);
            float z = start.depth + (zStepSize * i);
            int x_coord = round(x + (WIDTH / 2));
            int y_coord = round(y + (HEIGHT / 1.3));
            incomingDepth[x_coord][y_coord] = z;
            setPixelColour_(round(x + (WIDTH / 2)), round(y + (HEIGHT / 1.3)), colour);
        }
    }
    else
    {
        int x_coord = round(start.x + (WIDTH / 2));
        int y_coord = round(start.y + (HEIGHT / 1.3));
        incomingDepth[x_coord][y_coord] = start.depth;
        setPixelColour_(round(start.x + (WIDTH / 2)), round(start.y + (HEIGHT / 1.3)), colour);
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

void display_obj(std::string filename, float canvasDepth)
{
    std::vector<CanvasTriangle> triangles = project(load_obj(filename), canvasDepth);
    std::map<int, std::string> face_mtl = load_colour("cornell.obj");
    std::map<std::string, Colour> mtls = load_mtl("cornell-box.mtl");

    for (int i = 0; i < triangles.size(); i++)
    {
        // stroke_triangle(triangles[i]);
        filled_triangle(triangles[i], mtls[face_mtl[i]]);
        clearDepthBuffer("incoming");
    }
    //done painting current object, clear incoming depth array
}

void clearDepthBuffer(std::string bufferName)
{
    if (bufferName == "all")
    {
        for (int i = 0; i < WIDTH; i++)
        {
            for (int j = 0; j < HEIGHT; j++)
            {
                currentDepth[i][j] = std::numeric_limits<float>::max();
                incomingDepth[i][j] = std::numeric_limits<float>::max();
            }
        }
    }

    if (bufferName == "incoming")
    {
        for (int i = 0; i < WIDTH; i++)
        {
            for (int j = 0; j < HEIGHT; j++)
            {
                incomingDepth[i][j] = std::numeric_limits<float>::max();
            }
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_Event event;

    //initialise the buffers
    for (int i = 0; i < WIDTH; i++)
    {
        currentDepth[i] = new float[HEIGHT];
        incomingDepth[i] = new float[HEIGHT];
    }
    //initialise depth arrays to infinity
    clearDepthBuffer("all");

    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
        {
            handleEvent(event);
        }
        display_obj("cornell.obj", 10);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }

    return 0;
}