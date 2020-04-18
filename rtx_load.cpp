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
#include <list>
#include <set>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>

#define WIDTH 320
#define HEIGHT 240
#define SCALING 20
void intersection_on_pixel(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, int focal, glm::mat3 rotation_matrix, std::vector<std::vector<Colour>> rgb_values);
std::vector<std::string> split(std::string str, char delimiter);
std::vector<bool> get_all_shadows(std::vector<glm::vec3> light_sources, std::vector<ModelTriangle> triangles, glm::vec3 closest_point, ModelTriangle closest_triangle);
std::vector<ModelTriangle> load_obj(std::string filename);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
void display_obj(std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition);
std::map<int, std::string> load_colour(std::string filename);
std::map<std::string, Colour> load_mtl(std::string filename);
Colour proximity_lighting(ModelTriangle triangle, glm::vec3 intersection_point, std::vector<glm::vec3> light_source, std::vector<bool> is_shadow, bool is_glass, float u, float v);
// Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction);
Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction, std::vector<std::vector<Colour>> rgb_values);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
Colour white = Colour(255, 255, 255);
std::vector<ModelTriangle> global_triangles;
std::map<std::string, std::string> global_animation; //object name -> animation info
std::map<std::string, glm::vec3> global_centroids;   //object name -> object centroid

std::vector<std::vector<Colour>> readfile(std::string filename, int width, int height, std::vector<std::vector<Colour>> rgb_values)
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
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            //get each pixel and deconstruct
            uint32_t pixel = w.getPixelColour(j, i);

            int b = pixel & 255;
            int g = (pixel >> 8) & 255;
            int r = (pixel >> 16) & 255;

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

void remove(std::vector<glm::vec3> &v)
{
    auto end = v.end();
    for (auto it = v.begin(); it != end; ++it)
    {
        end = std::remove(it + 1, end, *it);
    }

    v.erase(end, v.end());
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
                continue; //if empty, skip
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

                std::string next_line = lines[i + 1];
                std::vector<std::string> next_split = split(next_line, ' ');
                std::string next_mode = next_split[0];

                if (next_mode == "a") //if animation info is defined in the next line, parse that information
                {
                    std::string animation_info = next_split[1];
                    global_animation.insert(std::pair<std::string, std::string>(current_object, animation_info));
                }

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

        std::cout << "Successfully loaded: " << filename << "\n"
                  << std::endl;
    }

    return loaded_triangles;
}

std::vector<ModelTriangle> calculate_vertex_normals(std::vector<ModelTriangle> triangles)
{
    for (int i = 0; i < triangles.size(); i++)
    {
        if (triangles[i].type == "Icosphere")
        {
            for (int j = 0; j < 3; j++)
            {
                int count = 1;
                glm::vec3 current_vertex = triangles[i].vertices[j];
                glm::vec3 current_vertex_normal = triangles[i].normal;
                for (int inside = 0; inside < triangles.size(); inside++)
                {
                    if ((inside != i) && (triangles[inside].type == "Icosphere"))
                    {
                        for (int inner_j = 0; inner_j < 3; inner_j++)
                        {
                            glm::vec3 search_vertex = triangles[inside].vertices[inner_j];
                            if ((search_vertex.x == current_vertex.x) && (search_vertex.y == current_vertex.y) && (search_vertex.z == current_vertex.z))
                            {
                                count++;
                                current_vertex_normal = current_vertex_normal + triangles[inside].normal;
                            }
                        }
                    }
                }
                current_vertex_normal = current_vertex_normal / glm::vec3(count, count, count);
                triangles[i].setVertex_Normals(j, current_vertex_normal);
            }
        }
    }
    return triangles;
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

    // scale logo
    for (int i = 0; i < triangles.size(); i++)
    {
        if (triangles[i].type == "logo")
        {

            for (int j = 0; j < 3; j++)
            {
                triangles[i].vertices[j].x = (triangles[i].vertices[j].x / 120) - 2.5;
                triangles[i].vertices[j].y = (triangles[i].vertices[j].y / 120);
                triangles[i].vertices[j].z = (triangles[i].vertices[j].z / 11) - 3.0;
            }
        }
    }
    // load texture
    std::string filename = "logo_texture.ppm";
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
    std::vector<std::vector<Colour>> rgb_values(width);
    for (int i = 0; i < width; i++)
    {
        rgb_values[i].resize(height);
    }
    triangles = calculate_vertex_normals(triangles);
    rgb_values = readfile(filename, width, height, rgb_values);
    // std::cout << "END READING RGB VAL" << std::endl;
    intersection_on_pixel(cameraPosition, triangles, focal, final_rotation_matrix, rgb_values);
}

/*Calculate the centroid given an object name*/
glm::vec3 centroid(std::string object)
{
    if (global_centroids.find(object) != global_centroids.end()) //if already exist, return record
    {
        // std::cout << "Found centroid for object: " << object << std::endl;
        return global_centroids[object];
    }

    std::vector<glm::vec3> object_vertices; //if not, continue to calculate
    glm::vec3 vertex_sum(0, 0, 0);

    for (ModelTriangle traingle : global_triangles)
    {
        if (traingle.type == object)
        {
            for (int i = 0; i < 3; i++)
            {
                object_vertices.push_back(traingle.vertices[i]);
            }
        }
    }

    // remove(object_vertices);

    for (glm::vec3 vertex : object_vertices)
    {
        vertex_sum = vertex_sum + vertex;
    }

    glm::vec3 centre = glm::vec3(vertex_sum.x / object_vertices.size(),
                                 vertex_sum.y / object_vertices.size(),
                                 vertex_sum.z / object_vertices.size());

    global_centroids[object] = centre;

    std::cout << "Added centroid for object: " << object << " [" << centre.x << "," << centre.y << "," << centre.z << "]" << std::endl;

    return centre;
}

glm::vec3 rotate_vertex(glm::vec3 vertex, float theta_x, float theta_y, float theta_z, std::string object)
{
    glm::mat3 rot_x;
    glm::mat3 rot_y;
    glm::mat3 rot_z;
    glm::vec3 wrt_origin = vertex - centroid(object);

    if (theta_x != 0)
    {
        rot_x = glm::mat3(glm::vec3(1, 0, 0),
                          glm::vec3(0, cos(theta_x), sin(theta_x)),
                          glm::vec3(0, -sin(theta_x), cos(theta_x)));

        wrt_origin = rot_x * wrt_origin;
    }

    if (theta_y != 0)
    {
        rot_y = glm::mat3(glm::vec3(cos(theta_y), 0, -sin(theta_y)),
                          glm::vec3(0, 1, 0),
                          glm::vec3(sin(theta_y), 0, cos(theta_y)));

        wrt_origin = rot_y * wrt_origin;
    }

    if (theta_z != 0)
    {
        rot_z = glm::mat3(glm::vec3(cos(theta_z), sin(theta_z), 0),
                          glm::vec3(-sin(theta_z), cos(theta_z), 0),
                          glm::vec3(0, 0, 1));

        wrt_origin = rot_z * wrt_origin;
    }

    glm::vec3 wrt_object = wrt_origin + centroid(object);

    return wrt_object;
}

void animate(std::vector<ModelTriangle> initial_triangles, glm::vec3 camera_position)
{
    if (global_animation.empty()) //if no animation is found in any object, just render one frame
    {
        display_obj(initial_triangles, camera_position);
        return;
    }

    std::vector<std::vector<ModelTriangle>> animated_stack;
    int max_frame = 0;

    for (auto item : global_animation) //detect max frame
    {
        std::vector<std::string> animation_info = split(item.second, ',');
        float current_frame = std::stof(animation_info[3]);

        if (current_frame <= 0)
        {
            throw "Frame number cannot be negative!";
        }

        if (current_frame > max_frame)
        {
            max_frame = int(current_frame);
        }
    }

    animated_stack.push_back(initial_triangles);

    for (int f = 1; f < max_frame; f++) //animate until max frame number reached
    {
        std::vector<ModelTriangle> previous_frame = animated_stack.back(); //get last frame rendered
        std::vector<ModelTriangle> animated_frame;

        for (ModelTriangle triangle : previous_frame) //apply animation
        {

            std::string object_name = triangle.type;

            if (global_animation.find(object_name) != global_animation.end())
            {
                std::string vertex_animation = global_animation[object_name];
                std::vector<std::string> animation_info = split(vertex_animation, ',');
                std::string animation_type = animation_info[4]; //scalar or rotation
                glm::vec3 animation_step(std::stof(animation_info[0]),
                                         std::stof(animation_info[1]),
                                         std::stof(animation_info[2]));

                if (f <= std::stof(animation_info[3]))
                {
                    if (animation_type == "s") //if this is a scalar animation
                    {
                        glm::vec3 v0 = triangle.vertices[0] + animation_step;
                        glm::vec3 v1 = triangle.vertices[1] + animation_step;
                        glm::vec3 v2 = triangle.vertices[2] + animation_step;

                        ModelTriangle animated_triangle;
                        std::memcpy(&animated_triangle, &triangle, sizeof(ModelTriangle));
                        animated_triangle.setVertices(v0, v1, v2);

                        animated_frame.push_back(animated_triangle);
                    }

                    if (animation_type == "r") // if this is a rotation animation
                    {
                        float theta_x = std::stof(animation_info[0]);
                        float theta_y = std::stof(animation_info[1]);
                        float theta_z = std::stof(animation_info[2]);

                        glm::vec3 v0 = rotate_vertex(triangle.vertices[0], theta_x, theta_y, theta_z, object_name);
                        glm::vec3 v1 = rotate_vertex(triangle.vertices[1], theta_x, theta_y, theta_z, object_name);
                        glm::vec3 v2 = rotate_vertex(triangle.vertices[2], theta_x, theta_y, theta_z, object_name);

                        ModelTriangle animated_triangle;
                        std::memcpy(&animated_triangle, &triangle, sizeof(ModelTriangle));
                        animated_triangle.setVertices(v0, v1, v2);

                        animated_frame.push_back(animated_triangle);
                    }
                }
                else
                {
                    animated_frame.push_back(triangle); //if animation over, push last one
                }
            }
            else
            {
                animated_frame.push_back(triangle);
            }
        }
        animated_stack.push_back(animated_frame);
    }

    for (int i = 1; i < animated_stack.size(); i++) //export frames
    {
        std::cout << "Generating animation frame: " << i << ", size: " << animated_stack[i].size() << "\n"
                  << std::endl;
        std::string ppm_filename = "frame_" + std::to_string(i) + ".ppm";

        display_obj(animated_stack[i], camera_position);

        savePPM(window, ppm_filename);
        std::cout << "Saved frame: " << ppm_filename << std::endl;
    }
}

float distance_of_vectors(glm::vec3 start, glm::vec3 end)
{
    return sqrt(pow(end.x - start.x, 2.0) + pow(end.y - start.y, 2.0) + pow(end.z - start.z, 2.0));
}

Colour mirror(ModelTriangle triangle, glm::vec3 incoming_ray, std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, std::vector<glm::vec3> light_sources, std::vector<bool> is_shadows)
{
    glm::vec3 triangle_normal = triangle.normal;
    // get Reflection ray
    float N_dot_I = glm::dot(triangle_normal, incoming_ray) * 2.0;
    glm::vec3 ground_ray = N_dot_I * triangle_normal;
    glm::vec3 reflection_ray = incoming_ray - ground_ray;
    glm::vec3 closest_point = glm::vec3(0, 0, 0);
    ModelTriangle closest_triangle = triangles[0];

    float closest_t = FLT_MAX;
    bool is_intersection = false;
    float final_u = 0.0;
    float final_v = 0.0;
    Colour black = Colour(0, 0, 0);
    //loop through every triangle
    for (ModelTriangle triangle : triangles)
    {
        if (triangle.type != "left_wall")
        {
            glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
            glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
            glm::vec3 SPVector = cameraPosition - triangle.vertices[0];
            glm::mat3 DEMatrix(-reflection_ray, e0, e1);
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
                        final_u = u;
                        final_v = v;
                        closest_triangle = triangle;
                        closest_point = triangle.vertices[0] + (u * e0) + (v * e1);
                    }
                }
            }
        }
    }
    if (is_intersection)
    {
        std::vector<bool> mirror_shadows = get_all_shadows(light_sources, triangles, closest_point, closest_triangle);
        Colour output_colour = proximity_lighting(closest_triangle, closest_point, light_sources, mirror_shadows, false, final_u, final_v);
        return output_colour;
    }
    else
    {
        return black;
    }
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
Colour proximity_lighting(ModelTriangle triangle, glm::vec3 intersection_point, std::vector<glm::vec3> light_source, std::vector<bool> is_shadow, bool is_glass, float u, float v)
{
    glm::vec3 triangle_normal = triangle.normal;
    if (triangle.type == "Icosphere")
    {
        glm::vec3 e0 = triangle.vertex_normals[1] - triangle.vertex_normals[0];
        glm::vec3 e1 = triangle.vertex_normals[2] - triangle.vertex_normals[0];
        triangle_normal = triangle.vertex_normals[0] + (u * e0) + (v * e1);
        // triangle_normal = glm::normalize(glm::cross(triangle.vertex_normals[2] - triangle.vertex_normals[0], triangle.vertex_normals[1] - triangle.vertex_normals[0]));
    }
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
        if (is_glass == false)
        {
            for (int i = 0; i < is_shadow.size(); i++)
            {
                if (is_shadow[i] == true)
                {
                    coefficient = coefficient * 0.5;
                }
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
    glm::vec3 A = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 B = triangle.vertices[2] - triangle.vertices[0];
    glm::vec3 cross = glm::cross(B, A);
    glm::vec3 triangle_normal_vector = glm::normalize(cross);
    glm::vec3 entry_ray = glm::normalize(intersection_point - light_source);
    glm::vec3 triangle_normal = (glm::dot(triangle_normal_vector, entry_ray) <= 0) ? (triangle_normal_vector) : (triangle_normal_vector * -1.0f);
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
Colour get_texture_colour(float u, float v, ModelTriangle triangle, std::vector<std::vector<Colour>> rgb_values)
{
    // std::cout << u << " , " << v << std::endl;
    // std::cout << triangle.textures[0].x << " , " << triangle.textures[0].y << std::endl;
    // std::cout << triangle.textures[1].x << " , " << triangle.textures[1].y << std::endl;
    // std::cout << triangle.textures[2].x << " , " << triangle.textures[2].y << std::endl;

    glm::vec2 e0 = triangle.textures[1] - triangle.textures[0];
    glm::vec2 e1 = triangle.textures[2] - triangle.textures[0];
    glm::vec2 closest_point = triangle.textures[0] + (u * e0) + (v * e1);
    // std::cout << closest_point[0] << " , " << closest_point[1] << std::endl;
    int x = round(closest_point[0] * rgb_values.size());
    int y = round(closest_point[1] * rgb_values.size());
    // std::cout << x << " , " << y << std::endl;
    // if (x < 0)
    // {
    //     x = 0;
    // }
    // if (y < 0)
    // {
    //     y = 0;
    // }
    if (x > 299)
    {
        x = 299;
    }
    if (y > 299)
    {
        y = 299;
    }
    return rgb_values[x][y];
}
Colour getClosestIntersection(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, glm::vec3 ray_direction, std::vector<std::vector<Colour>> rgb_values)
{

    glm::vec3 closest_point = glm::vec3(0, 0, 0);
    ModelTriangle closest_triangle = triangles[0];

    float closest_t = FLT_MAX;
    float final_u = 0.0;
    float final_v = 0.0;
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
                    final_u = u;
                    final_v = v;
                    closest_t = t;
                    closest_triangle = triangle;
                    closest_point = triangle.vertices[0] + (u * e0) + (v * e1);
                }
            }
        }
    }

    if (is_intersection == true)
    {
        if (closest_triangle.type == "logo")
        {
            return get_texture_colour(final_u, final_v, closest_triangle, rgb_values);
        }
        glm::vec3 light_source = glm::vec3(1.234, 4.5, 0.04);
        std::vector<glm::vec3> light_sources = get_light_points(light_source);
        std::vector<bool> shadows = get_all_shadows(light_sources, triangles, closest_point, closest_triangle);
        if (closest_triangle.type == "left_wall")
        {
            return mirror(closest_triangle, ray_direction, triangles, closest_point, light_sources, shadows);
        }
        Colour output_colour = proximity_lighting(closest_triangle, closest_point, light_sources, shadows, false, final_u, final_v);
        // Colour output_colour = specular_lighting(closest_triangle, closest_point, cameraPosition, light_sources[0], shadows[0]);
        return output_colour;
        // }
    }
    else
    {
        return black;
    }
}
void intersection_on_pixel(glm::vec3 cameraPosition, std::vector<ModelTriangle> triangles, int focal, glm::mat3 rotation_matrix, std::vector<std::vector<Colour>> rgb_values)
{
    // transformation vector by translation only
    glm::vec3 ts_vector = glm::vec3(0, 2, 3.5);
    float aspect_ratio = WIDTH / HEIGHT;
    float fov = 90;
    float scale = tan((fov * 0.5) * (M_PI / 180));

    //loop through each pixel in image plane coordiantes
    for (int i = 0; i < WIDTH; i++)
    {
        std::cout << "Sampling: " << i << "/" << WIDTH << std::endl;
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
            Colour line_colour = getClosestIntersection(cam_position_transformed, triangles, ray_direction, rgb_values);
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
    std::vector<std::string> files{"cornell-box.obj", "logo.obj"};
    global_triangles = load_files(files);

    if (true)
    {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(&event))
        {
            handleEvent(event);
        }

        //this is the function that does ray tracing
        animate(global_triangles, cameraPosition);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !

        window.renderFrame();
    }

    return 0;
}