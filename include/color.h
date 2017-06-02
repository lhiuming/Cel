#ifndef CEL_COLOR_H
#define CEL_COLOR_H

#include <ostream>

/*
 * color.h
 * Define how color is stored and computed.
 */

namespace CEL {

class Color {
public:

  // Color components. All ranges in [0, 1].
  float r;
  float g;
  float b;
  float a;

  // Default constructor
  Color() = default;

  // Initialize with RGBA or RGBA channels.
  Color(float r, float g, float b, float a = 1.0) : r(r), g(g), b(b), a(a) {};

  // Scalar Multiplication. Useful for interpolation
  Color operator*(float c) { return Color(r * c, g * c, b * c, a * c); }

};

// Print out the color
std::ostream& operator<<(std::ostream& os, const Color& col);

} // namespace CEL

#endif
