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

std::vector<std::string> split(std::string str, char delimiter);
void draw_line(Colour line_colour, CanvasPoint start, CanvasPoint end);
std::map<std::string, Colour> load_mtl(std::string filename);

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

std::vector<CanvasTriangle> project(std::vector<ModelTriangle> faces, glm::vec3 camera_position)
{
    std::vector<CanvasTriangle> projected;
    int focal = 5 * SCALING;
    
    // cameraPosition = camera_rotation(0, 0, cameraPosition);
    // cameraPosition[2] = cameraPosition[2] + 10;

    for (ModelTriangle face : faces)
    {
        face.vertices[0] = face.vertices[0] - camera_position;
        face.vertices[1] = face.vertices[1] - camera_position;
        face.vertices[2] = face.vertices[2] - camera_position;

        projected.push_back(CanvasTriangle(CanvasPoint(face.vertices[0].x * focal / ((face.vertices[0].z * -1)), (face.vertices[0].y * -1) * focal / ((face.vertices[0].z * -1))),
                                           CanvasPoint(face.vertices[1].x * focal / ((face.vertices[1].z * -1)), (face.vertices[1].y * -1) * focal / ((face.vertices[1].z * -1))),
                                           CanvasPoint(face.vertices[2].x * focal / ((face.vertices[2].z * -1)), (face.vertices[2].y * -1) * focal / ((face.vertices[2].z * -1))),
                                           face.colour));
    }

    return projected;
}

void display_obj(std::vector<ModelTriangle> triangles, glm::vec3 camera_position)
{
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

    glm::vec3 cameraPosition = glm::vec3(0, 1, 3);
    std::vector<CanvasTriangle> projected_triangles = project(triangles, cameraPosition);

    for (int i = 0; i < projected_triangles.size(); i++)
    {
        stroke_triangle(projected_triangles[i]);
        // filled_triangle(triangles[i], triangles[i].colour);
    }
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

    for (int i = 0; i < animated_stack.size(); i++) //!export frames
    {
        std::cout << "Generating animation frame: " << i << ", size: " << animated_stack[i].size() << "\n"
                  << std::endl;
        std::string ppm_filename = "frame_" + std::to_string(i) + ".ppm";

        glm::vec3 animation_camera = camera_position + glm::vec3(-0.01 * i, 0, 0);
        display_obj(animated_stack[i], animation_camera);

        savePPM(window, ppm_filename);
        std::cout << "Saved frame: " << ppm_filename << std::endl;

        window.clearPixels(); //! Rasteriser needs clear screen
    }
}

float distance_of_vectors(glm::vec3 start, glm::vec3 end)
{
    return sqrt(pow(end.x - start.x, 2.0) + pow(end.y - start.y, 2.0) + pow(end.z - start.z, 2.0));
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    glm::vec3 cameraPosition = glm::vec3(0, 0, -1);

    //load multiple files, give list as input
    std::vector<std::string> files{"cornell-box.obj", "logo.obj", "sphere.obj"};
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