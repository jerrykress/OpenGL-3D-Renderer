#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <math.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <fstream>
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
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
int indexofSmallestElement(float array[], int size);
int indexofLargestElement(float array[], int size);
void colored_triangle(CanvasPoint vt1, CanvasPoint vt2, CanvasPoint vt3, Colour color);
void filled_triangle(CanvasTriangle triangle, Colour tri_color);
void display_obj(std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition);
std::map<int, std::string> load_colour(std::string filename);
std::map<std::string, Colour> load_mtl(std::string filename);
std::vector<glm::vec3> interpolate_3d(glm::vec3 from, glm::vec3 to, int size);
// Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction);
Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
Colour white = Colour(255, 255, 255);
std::vector<ModelTriangle> global_triangles;

void savePPM(DrawingWindow w, std::string filename)
{
    //open a new file
    std::ofstream file;
    file.open(filename);

    //write ppm header
    file << "P3\n";
    file << WIDTH << " " << HEIGHT << "\n";
    file << "255\n";

    //write ppm body
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            //get each pixel and deconstruct
            uint32_t pixel = w.getPixelColour(j, i);

            int b =  pixel & 255;
            int g = (pixel >> 8) & 25;
            int r = (pixel >> 16) & 25;

            file << r << " " << g << " " << b << "  ";
        }
        file << "\n";
    }

    file.close();
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

            if (mode == "o"){
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

                if(extras.size() == 2)
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

void display_obj(std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition)
{
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
Colour proximity_lighting(ModelTriangle triangle, glm::vec3 intersection_point, std::vector<glm::vec3> light_source, std::vector<bool> is_shadow)
{
    glm::vec3 A = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 B = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 cross = glm::cross(B, A);
    glm::vec3 triangle_normal = glm::normalize(cross);
    glm::vec3 average_vertices = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]);
    average_vertices = glm::vec3(float(average_vertices.x / 3), float(average_vertices.y / 3), float(average_vertices.z / 3));
    glm::vec3 surface_to_lightsource;
    float dot_product = 0.0;
    float coefficient = 0.0;
    bool inside_color = false;
    for (int i = 0; i < light_source.size(); i++)
    {
        surface_to_lightsource = intersection_point - light_source[i];
        dot_product = glm::dot(triangle_normal, surface_to_lightsource);
        if ((dot_product > 0) && (dot_product < 90))
        {
            inside_color = true;
            float r = distance_of_vectors(intersection_point, light_source[i]);
            float power = pow(r, 2);

            coefficient = coefficient + ((40.0 * dot_product) / (4 * M_PI * power));
        }
    }

    if (inside_color == true)
    {
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
        for (int i = 0; i < is_shadow.size(); i++)
        {
            if (is_shadow[i] == true)
            {
                coefficient = coefficient * 0.5;
            }
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

Colour specular_lighting(ModelTriangle triangle, glm::vec3 intersection_point, glm::vec3 camera_position, glm::vec3 light_source, bool is_shadow)
{
    glm::vec3 entry_ray = glm::normalize(intersection_point - light_source);
    glm::vec3 triangle_normal = (glm::dot(triangle.normal, entry_ray) <= 0) ? (triangle.normal) : (triangle.normal * -1.0f);
    glm::vec3 observation_ray = glm::normalize(camera_position - intersection_point);
    glm::vec3 reflection_ray = glm::normalize(entry_ray - float(2) * glm::dot(triangle_normal, entry_ray) * triangle_normal);

    float specular_coefficient = glm::dot(observation_ray, reflection_ray);

    specular_coefficient = (specular_coefficient > 1) ? (1) : (specular_coefficient);
    specular_coefficient = (specular_coefficient < 0) ? (0) : (specular_coefficient);

    float r = (is_shadow) ? (float(triangle.colour.red) * 0.2) : (float(triangle.colour.red) * specular_coefficient);
    float g = (is_shadow) ? (float(triangle.colour.green) * 0.2) : (float(triangle.colour.green) * specular_coefficient);
    float b = (is_shadow) ? (float(triangle.colour.blue) * 0.2) : (float(triangle.colour.blue) * specular_coefficient);

    return Colour(int(r), int(g), int(b));
}
std::vector<bool> get_all_shadows(std::vector<glm::vec3> light_sources, std::vector<ModelTriangle> triangles, glm::vec3 closest_point, ModelTriangle closest_triangle)
{
    std::vector<bool> shadows;
    for (int i = 0; i < light_sources.size(); i++)
    {
        bool shadow_detection = shadow_detector(light_sources[i], triangles, closest_point, closest_triangle);
        shadows.push_back(shadow_detection);
    }

    return shadows;
}
std::vector<glm::vec3> get_light_points(glm::vec3 middle_light_point)
{
    // currently turned soft shadows off
    std::vector<glm::vec3> light_points;
    float height = middle_light_point.y;
    glm::vec3 left_top = glm::vec3(middle_light_point.x - 0.1, height, middle_light_point.z - 0.1);
    // glm::vec3 right_top = glm::vec3(middle_light_point.x + 0.1, height, middle_light_point.z - 0.1);
    // glm::vec3 left_bottom = glm::vec3(middle_light_point.x - 0.1, height, middle_light_point.z + 0.1);
    // glm::vec3 right_bottom = glm::vec3(middle_light_point.x + 0.1, height, middle_light_point.z + 0.1);
    // glm::vec3 mid_top = glm::vec3(middle_light_point.x - 0.05, height, middle_light_point.z - 0.05);
    // glm::vec3 mid_bottom = glm::vec3(middle_light_point.x + 0.05, height, middle_light_point.z + 0.05);

    light_points.push_back(left_top);
    // light_points.push_back(right_top);
    // light_points.push_back(left_bottom);
    // light_points.push_back(right_bottom);
    // light_points.push_back(mid_top);
    // light_points.push_back(mid_bottom);
    return light_points;
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
        glm::vec3 light_source = glm::vec3(1.234, 4.5, 0.04);
        std::vector<glm::vec3> light_sources = get_light_points(light_source);
        std::vector<bool> shadows = get_all_shadows(light_sources, triangles, closest_point, closest_triangle);
        Colour output_colour = proximity_lighting(closest_triangle, closest_point, light_sources, shadows);
        // Colour output_colour = specular_lighting(closest_triangle, closest_point, cameraPosition, light_source, shadow_detection);
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

            // anti-aliasing
            // // pixel as a world coordiantes
            // float x = (2 * ((i + 0.25) / WIDTH) - 1) * aspect_ratio * scale;
            // float y = (1 - (2 * ((j + 0.5) / HEIGHT))) * scale;
            // //calculate ray from camera to the pixel and normalize it
            // glm::vec3 pixel_pos_local = glm::vec3(x, y, focal);
            // //transform the vectors with the translation vector
            // glm::vec3 cam_position_transformed = (ts_vector + cameraPosition) * rotation_matrix;
            // glm::vec3 pixel_pos_world = (ts_vector + pixel_pos_local) * rotation_matrix;
            // //generate and normalize ray direction
            // glm::vec3 ray_direction = glm::normalize(pixel_pos_world - cam_position_transformed);
            // // get the colour from nearest triangle the ray intersects, if none then we draw black
            // Colour line_colour = getClosestIntersection(cam_position_transformed, triangles, ray_direction);
            // // float red = line_colour.red;
            // // float green = line_colour.green;
            // // float blue = line_colour.blue;

            // x = (2 * ((i + 0.5) / WIDTH) - 1) * aspect_ratio * scale;
            // y = (1 - (2 * ((j + 0.75) / HEIGHT))) * scale;
            // //calculate ray from camera to the pixel and normalize it
            // pixel_pos_local = glm::vec3(x, y, focal);
            // //transform the vectors with the translation vector
            // cam_position_transformed = (ts_vector + cameraPosition) * rotation_matrix;
            // pixel_pos_world = (ts_vector + pixel_pos_local) * rotation_matrix;
            // //generate and normalize ray direction
            // ray_direction = glm::normalize(pixel_pos_world - cam_position_transformed);
            // // get the colour from nearest triangle the ray intersects, if none then we draw black
            // Colour line_colour2 = getClosestIntersection(cam_position_transformed, triangles, ray_direction);
            // // tracing shadows

            // x = (2 * ((i + 0.75) / WIDTH) - 1) * aspect_ratio * scale;
            // y = (1 - (2 * ((j + 0.5) / HEIGHT))) * scale;
            // //calculate ray from camera to the pixel and normalize it
            // pixel_pos_local = glm::vec3(x, y, focal);
            // //transform the vectors with the translation vector
            // cam_position_transformed = (ts_vector + cameraPosition) * rotation_matrix;
            // pixel_pos_world = (ts_vector + pixel_pos_local) * rotation_matrix;
            // //generate and normalize ray direction
            // ray_direction = glm::normalize(pixel_pos_world - cam_position_transformed);
            // // get the colour from nearest triangle the ray intersects, if none then we draw black
            // Colour line_colour3 = getClosestIntersection(cam_position_transformed, triangles, ray_direction);
            // // tracing shadows

            // x = (2 * ((i + 0.5) / WIDTH) - 1) * aspect_ratio * scale;
            // y = (1 - (2 * ((j + 0.25) / HEIGHT))) * scale;
            // //calculate ray from camera to the pixel and normalize it
            // pixel_pos_local = glm::vec3(x, y, focal);
            // //transform the vectors with the translation vector
            // cam_position_transformed = (ts_vector + cameraPosition) * rotation_matrix;
            // pixel_pos_world = (ts_vector + pixel_pos_local) * rotation_matrix;
            // //generate and normalize ray direction
            // ray_direction = glm::normalize(pixel_pos_world - cam_position_transformed);
            // // get the colour from nearest triangle the ray intersects, if none then we draw black
            // Colour line_colour4 = getClosestIntersection(cam_position_transformed, triangles, ray_direction);
            // // tracing shadows

            // // output to screen
            // float red = (line_colour.red + line_colour2.red + line_colour3.red + line_colour4.red) / 4;
            // float green = (line_colour.green + line_colour2.green + line_colour3.green + line_colour4.green) / 4;
            // float blue = (line_colour.green + line_colour2.blue + line_colour3.blue + line_colour4.blue) / 4;
            // uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
            // window.setPixelColour(i, j, colour);
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    glm::vec3 cameraPosition = glm::vec3(0, 0, -1);

    //load multiple files, give list as input
    std::vector<std::string> files{"cornell-box.obj", "sphere8.obj"};
    global_triangles = load_files(files);

    while (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
        {
            handleEvent(event);
        }

        //this is the function that does ray tracing
        display_obj(global_triangles, cameraPosition);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();

    }

    return 0;
}