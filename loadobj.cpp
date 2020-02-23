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

std::vector<CanvasTriangle> loadObj(std::string filename){
    //lines from obj file
    std::vector<std::string> vs;
    std::vector<std::string> fs;
    //constructed elements
    std::vector<CanvasPoint> vertices;
    std::vector<CanvasTriangle> faces;
    //metadata
    std::ifstream file(filename.c_str());

    //read in entire file line by line
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            std::vector<std::string> chunks = split(line);
            //find if it's a vertex or a face
            if (chunks[0] == "v")
            {
                vs.push_back(glm::vec3(float(chunks[1]), float(chunks[2]), float(chunks[3])));
            }

            if (chunks[0] == "f")
            {
                fs.push_back(glm::vec3(int(chunks[1]), int(chunks[2]), int(chunks[3])));
            }

            std::cout << line << std::endl;
        }
    }

    //read done, construct vertices
    for (glm::vec3 vertex : vs)
    {
        vertices.push_back(CanvasPoint(vertex.x, vertex.y, vertex.z));
    }
    //build faces from converted vertices
    for (glm::vec3 face : fs)
    {
        faces.push_back(CanvasTriangle(vertices[face.x], vertices[face.y], vertices[face.z]));
    }

    return faces;
}

int main(int argc, char *argv[])
{
    loadObj("teapot.obj");
    return 0;
}
