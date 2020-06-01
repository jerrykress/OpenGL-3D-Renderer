#include <iostream>

class Colour
{
public:
  std::string name;
  std::string type;
  int red;
  int green;
  int blue;

  Colour()
  {
  }

  Colour(std::string n, std::string t) //for ppm type texture
  {
    name = n;
    type = t;
  }

  Colour(int r, int g, int b)
  {
    name = "";
    red = r;
    green = g;
    blue = b;
  }

  Colour(std::string n, int r, int g, int b)
  {
    name = n;
    red = r;
    green = g;
    blue = b;
  }

  Colour(std::string n, int r, int g, int b, std::string t)
  {
    name = n;
    red = r;
    green = g;
    blue = b;
    type = t;
  }

  int getred()
  {
    return red;
  }
  int getblue()
  {
    return blue;
  }
  int getgreen()
  {
    return green;
  }
};

std::ostream &operator<<(std::ostream &os, const Colour &colour)
{
  os << colour.name << " [" << colour.red << ", " << colour.green << ", " << colour.blue << "]" << std::endl;
  return os;
}
