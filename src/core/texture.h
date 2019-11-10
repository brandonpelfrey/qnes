#pragma once
#include <cstring>
#include <csignal>
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

  Texture(const Texture &other)
  {
    width = other.width;
    height = other.height;
    data = new u8[width * height * 3];
    memcpy(data, other.data, width * height * 3);
  }

  void Resize(int width, int height)
  {
    delete[] data;
    this->width = width;
    this->height = height;
    data = new u8[3 * width * height];
  }

  u16 GetWidth() const { return width; }
  u16 GetHeight() const { return height; }
  u8 *Data() { return data; }

  ~Texture() { delete[] data; }
};
