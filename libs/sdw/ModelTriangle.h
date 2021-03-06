#include <glm/glm.hpp>
#include "Colour.h"
#include <string>

class ModelTriangle
{
public:
  glm::vec3 vertices[3];
  glm::vec2 textures[3];
  glm::vec3 normals[3];
  glm::vec3 normal;
  Colour colour;
  std::string type;
  glm::vec3 vertex_normals[3];

  ModelTriangle()
  {
  }

  ModelTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
  {
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));
  }

  ModelTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, Colour trigColour)
  {
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    colour = trigColour;
    normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));
  }

  void setVertices(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2){
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
  }

  void setTexture(glm::vec2 t0, glm::vec2 t1, glm::vec2 t2)
  {
    textures[0] = t0;
    textures[1] = t1;
    textures[2] = t2;
  }

  void setNormal(glm::vec3 n0, glm::vec3 n1, glm::vec3 n2)
  {
    normals[0] = n0;
    normals[1] = n1;
    normals[2] = n2;
  }

  void setColour(Colour c)
  {
    colour = c;
  }

  void setType(std::string t)
  {
    type = t;
  }
  void setNormal(glm::vec3 n)
  {
    normal = n;
  }
  void setVertex_Normals(int index, glm::vec3 v1)
  {
    vertex_normals[index] = v1;
  }
};

std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle)
{
  os << "(" << triangle.vertices[0].x << ", " << triangle.vertices[0].y << ", " << triangle.vertices[0].z << ")" << std::endl;
  os << "(" << triangle.vertices[1].x << ", " << triangle.vertices[1].y << ", " << triangle.vertices[1].z << ")" << std::endl;
  os << "(" << triangle.vertices[2].x << ", " << triangle.vertices[2].y << ", " << triangle.vertices[2].z << ")" << std::endl;
  os << std::endl;
  return os;
}
