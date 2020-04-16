#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <chrono>
#include <vector>
#include <math.h>

class PixelBackEnd {
public:
  PixelBackEnd(int width, int height)
      : width(width), height(height), data(width * height), oldData(data) {}

  void set(int x, int y, float value) { 
    if (value > 1.0f)
    {
      value = 1.0f;
    }
    else if (value < 0.0f)
    {
      value = 0.0f;
    }
    data[x * width + y] = value; 
  }
  float get(int x, int y) const { return oldData[x * width + y]; }

  void step() {
    for (int i = 0; i < width; ++i)
      for (int j = 0; j < height; ++j) {
        set(i, j, convOdd(i, j));
      }
    endStep();
  }

  void endStep() { oldData = data; }

  inline int mod(int x, int m) const {
    int r = x % m;
    return r < 0 ? r + m : r;
  }

  float convOdd(int x, int y) const {
    float result = 0.0f;
    for (int i = -2; i <= 2; ++i) {
      for (int j = -2; j <= 2; ++j) {
        if (i == 0 && j == 0) continue; 
        int xp = (x + i);
        int yp = (y + j);
        if (xp >= width || yp >= height || xp <= 0 || yp <= 0)
          continue;
        result += get(xp, yp) / 23.9f;
      }
    }
    if (result > 1.0f)
    {
      result = 0.0f;
    }
    return result;
  }

private:
  const int height;
  const int width;
  std::vector<float> data;
  std::vector<float> oldData;
};

class Example : public olc::PixelGameEngine {
public:
  Example() : scene(511, 511) {
    sAppName = "Example";
    scene.set(255, 255, 1.0f);
    scene.endStep();
  }

public:
  bool OnUserCreate() override { return true; }

  bool OnUserUpdate(float fElapsedTime) override {
    auto start = std::chrono::system_clock::now();
    for (int x = 0; x < ScreenWidth(); x++)
      for (int y = 0; y < ScreenHeight(); y++) {
        uint32_t value = uint(scene.get(x, y) *255*255*255);
        olc::Pixel pixel(value>>16, value>>8, value);
        Draw(x, y, pixel);
      }
    scene.step();
    auto end = std::chrono::system_clock::now();
    auto diff = end - start;
    std::cout << "Time to render frame was: " << fElapsedTime << std::endl;
    return true;
  }

private:
  PixelBackEnd scene;
};

int main() {
  Example demo;
  if (demo.Construct(511, 511, 4, 4))
    demo.Start();
  return 0;
}
