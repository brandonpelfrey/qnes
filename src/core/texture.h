#pragma once

#include "core/types.h"

class Texture
{
private:
  u16 width, height;
  u8 *data;

public:
  Texture() : width(0), height(0), data(nullptr) {}
  Texture(int width, int height)
      : width(width), height(height)
  {
    data = new u8[width * height * 3];
  }

  u16 GetWidth() const { return width; }
  u16 GetHeight() const { return height; }
  u8 *Data() { return data; }

  ~Texture() { delete[] data; }
};
