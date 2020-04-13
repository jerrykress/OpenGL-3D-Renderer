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
#define SCALING 20

std::vector<std::string> split(std::string str, char delimiter);
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle, Colour tri_color);
void display_obj(std::vector<std::string> filenames, float canvasDepth);
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

    std::map<std::string, Colour> mtl; //result
    std::vector<std::string> lines;

    std::ifstream file(filename.c_str());

    if (file.is_open())
    { //convert file to vector of strings
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }
    }

    for (int i = 0; i < lines.size(); i++)
    { //for each line in file
        if (lines[i].length() < 1)
        {
            continue; //if empty, skip
        }
        std::vector<std::string> splits = split(lines[i], ' '); //otherwise process current line
        std::string mode = splits[0];                           //extract line header

        if (mode == "newmtl")
        {
            std::string mtl_name = splits[1];
            std::string mtl_detail = lines[i + 1];
            std::vector<std::string> details = split(mtl_detail, ' ');
            Colour mtl_val = Colour(mtl_name, (int)(std::stof(details[1]) * 255), (int)(std::stof(details[2]) * 255), (int)(std::stof(details[3]) * 255), details[0]);

            mtl.insert(std::pair<std::string, Colour>(mtl_name, mtl_val));
            i++; //skip next line
        }

        if (mode == "map_Kd")
        {
            std::string mtl_name = splits[1];
            Colour mtl_val = Colour(mtl_name, "map_Kd");
        }
    }

    return mtl;
}

std::vector<ModelTriangle> load_files(std::vector<std::string> filenames)
{
    std::vector<ModelTriangle> loaded_triangles; //result

    for (std::string filename : filenames)
    {

        std::vector<std::string> lines;
        std::vector<glm::vec3> vs;
        std::vector<glm::vec2> vts;
        std::vector<glm::vec3> vns;
        std::vector<std::string> fs;
        std::map<std::string, Colour> cls;

        std::string current_object = "NULL";
        std::string current_colour = "NULL";

        std::ifstream file(filename.c_str());

        if (file.is_open())
        { //convert file to vector of strings
            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
            }
        }

        std::cout << "Done loading file..." << std::endl;

        for (int i = 0; i < lines.size(); i++)
        { //for each line in file
            if (lines[i].length() < 1)
            {
                current_colour = "NULL"; //reset current face colour
                current_object = "NULL"; //reset current object type
                continue;                //if empty, skip
            }
            std::vector<std::string> splits = split(lines[i], ' '); //otherwise process current line
            std::string mode = splits[0];                           //extract line header

            if (mode == "mtllib")
            {
                std::string mtlName = splits[1];
                cls = load_mtl(mtlName);
                std::cout << "Sucessfully loaded materials..." << std::endl;
                continue;
            }

            if (mode == "usemtl")
            {
                current_colour = splits[1];
                continue;
            }

            if (mode == "o")
            {
                current_object = splits[1];
                continue;
            }

            if (mode == "f")
            {
                if (splits.size() != 4)
                    throw "Incorrect face information in object file!";

                if (current_colour != "NULL")
                {
                    fs.push_back(lines[i] + " " + current_colour + "," + current_object);
                }
                else
                {
                    fs.push_back(lines[i]);
                }
                continue;
            }

            if (mode == "v")
            {
                vs.push_back(glm::vec3(std::stof(splits[1]), std::stof(splits[2]), std::stof(splits[3])));
                continue;
            }

            if (mode == "vt")
            {
                vts.push_back(glm::vec2(std::stof(splits[1]), std::stof(splits[2])));
                continue;
            }

            if (mode == "vn")
            {
                vns.push_back(glm::vec3(std::stof(splits[1]), std::stof(splits[2]), std::stof(splits[3])));
                continue;
            }
        }

        std::cout << "Done Parsing file..." << std::endl;

        for (std::string f : fs)
        {
            std::vector<int> f_vs;
            std::vector<int> f_vts;
            std::vector<int> f_vns;

            std::vector<std::string> face_info = split(f, ' '); //split each line of face information
            std::cout << "Started processing face: " << f << std::endl;

            for (int i = 1; i < 4; i++)
            { //process 3 vertex groups
                std::vector<std::string> vertex_info = split(face_info[i], '/');

                if (vertex_info.size() == 1)
                {
                    f_vs.push_back(std::stoi(vertex_info[0]));
                }
                if (vertex_info.size() == 2)
                {
                    f_vs.push_back(std::stoi(vertex_info[0]));
                    f_vts.push_back(std::stoi(vertex_info[1]));
                }
                if (vertex_info.size() == 3)
                {
                    f_vs.push_back(std::stoi(vertex_info[0]));
                    f_vts.push_back(std::stoi(vertex_info[1]));
                    f_vns.push_back(std::stoi(vertex_info[2]));
                }
            }

            std::cout << "Processed face vertices..." << std::endl;
            std::cout << "Retriving vertex:" << f_vs[0] << "," << f_vs[1] << "," << f_vs[2] << " of total " << vs.size() << std::endl;

            glm::vec3 v0 = vs[f_vs[0] - 1];
            glm::vec3 v1 = vs[f_vs[1] - 1];
            glm::vec3 v2 = vs[f_vs[2] - 1];

            std::cout << "Got face vertices..." << std::endl;

            ModelTriangle new_triangle = ModelTriangle(v0, v1, v2);

            std::cout << "Built face with vertices..." << std::endl;

            if (!f_vts.empty())
            {
                glm::vec2 t0 = vts[f_vts[0] - 1];
                glm::vec2 t1 = vts[f_vts[1] - 1];
                glm::vec2 t2 = vts[f_vts[2] - 1];

                new_triangle.setTexture(t0, t1, t2);

                std::cout << "Set face texture..." << std::endl;
            }

            if (!f_vns.empty())
            {
                glm::vec3 n0 = vns[f_vns[0] - 1];
                glm::vec3 n1 = vns[f_vns[1] - 1];
                glm::vec3 n2 = vns[f_vns[2] - 1];

                new_triangle.setNormal(n0, n1, n2);

                std::cout << "Set face normal..." << std::endl;
            }

            if (face_info.size() == 5)
            {
                //if face has additional information appended (colour and object type)
                std::vector<std::string> extras = split(face_info[4], ',');

                if (extras.size() == 2)
                {
                    if (cls.find(extras[0]) != cls.end())
                    {
                        Colour tri_colour = cls[extras[0]];
                        new_triangle.setColour(tri_colour);
                    }
                    std::cout << "Set face colour..." << std::endl;

                    new_triangle.setType(extras[1]);
                }
            }

            loaded_triangles.push_back(new_triangle);

            std::cout << "Added new face...\n"
                      << std::endl;
        }

        std::cout << "Successfully loaded: " << filename << std::endl;
    }

    return loaded_triangles;
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

glm::vec3 camera_rotation(int angle_x, int angle_y, glm::vec3 cameraPosition)
{
    // rotate by x-axis
    // glm::vec3 new_cam_position = cameraPosition;
    if (angle_x != 0)
    {
        glm::mat3 rotationMatrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, cos(angle_x), -sin(angle_x)),
                                 glm::vec3(0.0, sin(angle_x), cos(angle_x)));
        cameraPosition = cameraPosition * rotationMatrix;
    }

    if (angle_y != 0)
    {
        // rotate by y-axis
        glm::mat3 rotationMatrix(glm::vec3(cos(angle_y), 0.0, sin(angle_y)), glm::vec3(0.0, 1.0, 0.0),
                                 glm::vec3(-sin(angle_y), 0.0, cos(angle_y)));
        cameraPosition = cameraPosition * rotationMatrix;
    }
    return cameraPosition;
}
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth)
{
    std::vector<CanvasTriangle> projected;
    int focal = 5 * SCALING;
    glm::vec3 cameraPosition = glm::vec3(0, 1, 3);
    // cameraPosition = camera_rotation(0, 0, cameraPosition);
    // cameraPosition[2] = cameraPosition[2] + 10;

    for (ModelTriangle face : faces)
    {
        face.vertices[0] = face.vertices[0] - cameraPosition;
        face.vertices[1] = face.vertices[1] - cameraPosition;
        face.vertices[2] = face.vertices[2] - cameraPosition;

        projected.push_back(CanvasTriangle(CanvasPoint(face.vertices[0].x * focal / ((face.vertices[0].z * -1)), (face.vertices[0].y * -1) * focal / ((face.vertices[0].z * -1))),
                                           CanvasPoint(face.vertices[1].x * focal / ((face.vertices[1].z * -1)), (face.vertices[1].y * -1) * focal / ((face.vertices[1].z * -1))),
                                           CanvasPoint(face.vertices[2].x * focal / ((face.vertices[2].z * -1)), (face.vertices[2].y * -1) * focal / ((face.vertices[2].z * -1))),
                                           face.colour));
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
            window.setPixelColour(round((x) + (WIDTH / 2)), round((y) + (HEIGHT / 1.3)), colour);
        }
    }
    else
    {
        float red = line_colour.red;
        float green = line_colour.green;
        float blue = line_colour.blue;
        uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
        window.setPixelColour(round((start.x) + (WIDTH / 2)), round((start.y) + (HEIGHT / 1.3)), colour);
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

void display_obj(std::vector<std::string> filenames, float canvasDepth)
{
    std::vector<ModelTriangle> loaded = load_files(filenames);
    std::vector<CanvasTriangle> triangles = project(loaded, canvasDepth);   

    for (int i = 0; i < triangles.size(); i++)
    {
        // stroke_triangle(triangles[i]);
        filled_triangle(triangles[i], triangles[i].colour);
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

        std::vector<std::string> files{"cornell-box.obj"};
        display_obj(files, 10);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }

    return 0;
}