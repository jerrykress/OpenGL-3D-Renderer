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

#define WIDTH 320
#define HEIGHT 240
#define SCALING 20
void intersection_on_pixel(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, int focal, glm::mat3 rotation_matrix);
std::vector<std::string> split(std::string str, char delimiter);

std::vector<ModelTriangle> load_obj(std::string filename);
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle, Colour tri_color);
void display_obj(std::string filename, float canvasDepth);
std::map<int, std::string> load_colour(std::string filename);
std::map<std::string, Colour> load_mtl(std::string filename);
std::vector<glm::vec3> interpolate_3d(glm::vec3 from, glm::vec3 to, int size);
// Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction);
Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
Colour white = Colour(255, 255, 255);

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

glm::mat3 camera_rotation(int angle_x, int angle_y, glm::vec3 cameraPosition)
{
    // rotate by x-axis
    double angle_x_convert = angle_x * M_PI / 180;
    double angle_y_convert = angle_y * (M_PI / 180);

    glm::mat3 final_rotation_matrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));

    if (angle_x_convert != 0)
    {
        glm::mat3 rotationMatrix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, cos(angle_x), -sin(angle_x)),
                                 glm::vec3(0.0, sin(angle_x), cos(angle_x)));
        final_rotation_matrix = final_rotation_matrix * rotationMatrix;
    }

    if (angle_y_convert != 0)
    {
        // rotate by y-axis
        glm::mat3 rotationMatrix(glm::vec3(cos(angle_y), 0.0, sin(angle_y)), glm::vec3(0.0, 1.0, 0.0),
                                 glm::vec3(-sin(angle_y), 0.0, cos(angle_y)));
        final_rotation_matrix = final_rotation_matrix * rotationMatrix;
    }
    return final_rotation_matrix;
}
std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, float depth)
{
    std::vector<CanvasTriangle> projected;
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

void display_obj(std::vector<ModelTriangle> triangles, std::map<int, std::string> face_mtl, std::map<std::string, Colour> mtls, glm::vec3 cameraPosition)
{

    for (int i = 0; i < triangles.size(); i++)
    {
        triangles[i].colour = mtls[face_mtl[i]];
    }
    int focal = -2;
    //input as degress -> converter inside function
    glm::mat3 final_rotation_matrix = camera_rotation(0, 0, cameraPosition);
    // cameraPosition[2] = cameraPosition[2] + 4;

    intersection_on_pixel(cameraPosition, triangles, focal, final_rotation_matrix);
}
float distance_of_vectors(glm::vec3 start, glm::vec3 end)
{
    return sqrt(pow(end.x - start.x, 2.0) + pow(end.y - start.y, 2.0) + pow(end.z - start.z, 2.0));
}
bool shadow_detector(glm::vec3 light_source, std::vector<ModelTriangle> triangles, glm::vec3 intersection_point, ModelTriangle current_triangle)
{
    float closest_t = FLT_MAX;
    bool is_intersection = false;
    glm::vec3 closest_point = glm::vec3(0, 0, 0);
    glm::vec3 ray_direction = glm::normalize(light_source - intersection_point);
    float inter_to_light = distance_of_vectors(light_source, intersection_point);
    //loop through every triangle
    for (ModelTriangle triangle : triangles)
    {
        if ((triangle.vertices[0] != current_triangle.vertices[0]) && (triangle.vertices[1] != current_triangle.vertices[1]) && (triangle.vertices[2] != current_triangle.vertices[2]))
        {
            glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
            glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
            glm::vec3 SPVector = intersection_point - triangle.vertices[0];
            glm::mat3 DEMatrix(-ray_direction, e0, e1);
            glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
            float t = possibleSolution[0];
            float u = possibleSolution[1];
            float v = possibleSolution[2];
            // find the one that is closest
            if (t > 0)
            {
                if ((0.0 <= u) && (u <= 1.0) && (0.0 <= v) && (v <= 1.0) && (u + v <= 1))
                {
                    if (t < closest_t)
                    {
                        is_intersection = true;
                        closest_t = t;
                        closest_point = triangle.vertices[0] + (u * e0) + (v * e1);
                    }
                }
            }
        }
    }
    if (is_intersection == true)
    {
        float closest_to_inter = distance_of_vectors(intersection_point, closest_point);
        if ((closest_to_inter < inter_to_light) && (inter_to_light > 4))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return is_intersection;
}
Colour proximity_lighting(ModelTriangle triangle, glm::vec3 intersection_point, glm::vec3 light_source, bool is_shadow)
{

    glm::vec3 A = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 B = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 cross = glm::cross(B, A);
    glm::vec3 triangle_normal = glm::normalize(cross);
    // glm::vec3 average_vertices = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]);
    // average_vertices = glm::vec3(float(average_vertices.x / 3), float(average_vertices.y / 3), float(average_vertices.z / 3));
    glm::vec3 surface_to_lightsource = intersection_point - light_source;
    float dot_product = glm::dot(triangle_normal, surface_to_lightsource);
    if ((dot_product > 0) && (dot_product < 90))
    {
        // distance r
        float r = distance_of_vectors(intersection_point, light_source);
        float power = pow(r, 2);
        // includes angle of incidence
        float coefficient = (100.0 * dot_product) / (4 * M_PI * power);

        //does not include angle of incidence
        // float coefficient = 10.0 / (power);
        if (coefficient > 1)
        {
            coefficient = 1;
        }
        else if (coefficient < 0)
        {
            //ambieng lighting - Turned off for now
            coefficient = 0;
        }
        if (is_shadow == true)
        {
            coefficient = 0.2;
        }
        float red = float(triangle.colour.red) * coefficient;
        float green = float(triangle.colour.green) * coefficient;
        float blue = float(triangle.colour.blue) * coefficient;
        // return triangle.colour;
        return Colour(int(red), int(green), int(blue));
    }
    else
    {
        return Colour(0, 0, 0);
    };
}


Colour specular_lighting (ModelTriangle triangle, glm::vec3 intersection_point, glm::vec3 camera_position, glm::vec3 light_source, bool is_shadow){
    glm::vec3 entry_ray        = glm::normalize(intersection_point - light_source);
    glm::vec3 triangle_normal  = (glm::dot(triangle.normal, entry_ray) <= 0) ? (triangle.normal) : (triangle.normal * -1.0f);
    glm::vec3 observation_ray  = glm::normalize(camera_position - intersection_point);
    glm::vec3 reflection_ray   = glm::normalize(entry_ray - float(2) * glm::dot(triangle_normal, entry_ray) * triangle_normal);

    float specular_coefficient = glm::dot(observation_ray, reflection_ray);

    specular_coefficient = (specular_coefficient > 1) ? (1) : (specular_coefficient);
    specular_coefficient = (specular_coefficient < 0) ? (0) : (specular_coefficient);

    float r = (is_shadow) ? (float(triangle.colour.red)   * 0.2) : (float(triangle.colour.red)   * specular_coefficient);
    float g = (is_shadow) ? (float(triangle.colour.green) * 0.2) : (float(triangle.colour.green) * specular_coefficient);
    float b = (is_shadow) ? (float(triangle.colour.blue)  * 0.2) : (float(triangle.colour.blue)  * specular_coefficient);

    return Colour(int(r), int(g), int(b));
}


Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction)
{

    glm::vec3 closest_point = glm::vec3(0, 0, 0);
    ModelTriangle closest_triangle = triangles[0];

    float closest_t = FLT_MAX;
    bool is_intersection = false;
    Colour black = Colour(0, 0, 0);
    //loop through every triangle
    for (ModelTriangle triangle : triangles)
    {

        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
        glm::mat3 DEMatrix(-ray_direction, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
        float t = possibleSolution[0];
        float u = possibleSolution[1];
        float v = possibleSolution[2];
        // find the one that is closest
        if (t > 0)
        {
            if ((0.0 <= u) && (u <= 1.0) && (0.0 <= v) && (v <= 1.0) && (u + v <= 1))
            {
                if (t < closest_t)
                {
                    is_intersection = true;
                    closest_t = t;
                    closest_triangle = triangle;
                    closest_point = triangle.vertices[0] + (u * e0) + (v * e1);
                }
            }
        }
    }

    if (is_intersection)
    {
        glm::vec3 light_source = glm::vec3(-0.234, 4.5, -2.04);
        bool shadow_detection = shadow_detector(light_source, triangles, closest_point, closest_triangle);
        // if (shadow_detection)
        // {
        //     return black;
        // }
        // else
        // {
        // Colour output_colour = proximity_lighting(closest_triangle, closest_point, light_source, shadow_detection);
        Colour output_colour = specular_lighting(closest_triangle, closest_point, cameraPosition, light_source, shadow_detection);
        return output_colour;
        // }
    }
    else
    {
        return black;
    }
}
void intersection_on_pixel(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, int focal, glm::mat3 rotation_matrix)
{
    // transformation vector by translation only
    glm::vec3 ts_vector = glm::vec3(0, 2, 3.5);
    float aspect_ratio = WIDTH / HEIGHT;
    float fov = 90;
    float scale = tan((fov * 0.5) * (M_PI / 180));

    //loop through each pixel in image plane coordiantes
    for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            // pixel as a world coordiantes
            float x = (2 * ((i + 0.5) / WIDTH) - 1) * aspect_ratio * scale;
            float y = (1 - (2 * ((j + 0.5) / HEIGHT))) * scale;
            //calculate ray from camera to the pixel and normalize it
            glm::vec3 pixel_pos_local = glm::vec3(x, y, focal);
            //transform the vectors with the translation vector
            glm::vec3 cam_position_transformed = (ts_vector + cameraPosition) * rotation_matrix;
            glm::vec3 pixel_pos_world = (ts_vector + pixel_pos_local) * rotation_matrix;
            //generate and normalize ray direction
            glm::vec3 ray_direction = glm::normalize(pixel_pos_world - cam_position_transformed);
            // get the colour from nearest triangle the ray intersects, if none then we draw black
            Colour line_colour = getClosestIntersection(cam_position_transformed, triangles, ray_direction);
            // tracing shadows

            // output to screen
            float red = line_colour.red;
            float green = line_colour.green;
            float blue = line_colour.blue;
            uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            window.setPixelColour(i, j, colour);
        }
    }
}
int main(int argc, char *argv[])
{
    SDL_Event event;
    glm::vec3 cameraPosition = glm::vec3(0, 0, -1);
    std::vector<ModelTriangle> triangles = load_obj("cornell.obj");
    std::map<int, std::string> face_mtl = load_colour("cornell.obj");
    std::map<std::string, Colour> mtls = load_mtl("cornell-box.mtl");

    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
        {
            handleEvent(event);
        }

        //this is the function that does ray tracing
        display_obj(triangles, face_mtl, mtls, cameraPosition);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }

    return 0;
}