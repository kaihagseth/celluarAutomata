#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <chrono>
#include <execution>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>

inline double distance(std::pair<double, double> from,
                       std::pair<double, double> to) {
  return sqrt(pow(from.first - to.first, 2) + pow(from.second - to.second, 2));
}

class Image {
public:
  Image(const size_t width, const size_t height)
      : width(width), height(height), data(width * height), positions(makePositions()){};

  Image(const Image &other)
      : width(other.width), height(other.height),
        data(other.width * other.height) {
    data = other.data;
    positions = other.positions;
  }

  Image(const size_t width, const size_t height, std::vector<float> &&inData)
      : width(width), height(height), positions(makePositions()) {
    if (data != inData) {
      data = inData;
    }
  };

  Image &operator=(const Image &other) {
    data = other.data;
    return *this;
  }

  const Image operator/(const float divisor) const {
    Image out(width, height);
    auto *outData = &out.data;
    const auto &inData = read();
    auto divide = [&divisor](float dividend) -> float {
      float result = dividend / divisor;
      return result;
    };
    std::transform(inData.begin(), inData.end(), outData->begin(), divide);
    return out;
  }

  const Image operator*(const float factor) const {
    Image out(width, height);
    auto *outData = &out.data;
    const auto &inData = read();
    std::transform(inData.begin(), inData.end(), outData->begin(),
                   [&factor](float f) -> float { return f * factor; });
    return out;
  }

  Image &operator=(const Image &&other) {
    data = other.data;
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Image &image);
  const size_t width;
  const size_t height;

  struct Index {
    Index(size_t x, size_t y) : x(x), y(y){};
    Index() : x(0), y(0){};
    size_t x;
    size_t y;

    const Index operator+(const Index &other) const {
      return Index(x + other.x, y + other.y);
    }

    const Index operator-(const Index &other) const {
      return Index(x - other.x, y - other.y);
    }

    const Index operator/(const size_t &divisor) const {
      return Index(x / divisor, y / divisor);
    }
  };
  const Index shape() const { return Index(width, height); }

  bool indexOutside(const Index i) const {
    const size_t &x = i.x;
    const size_t &y = i.y;
    if (x >= width || y >= height) {
      return true;
    }
    return false;
  };
  const std::vector<float> &read() const { return data; }

  const Index getIndex(const uint i) const {
    return Index(i % width, i / height);
  }

  void set(Index idx, float value) noexcept {
    data[idx.x * width + idx.y] = value;
  }

  void set(size_t x, size_t y, float value) noexcept {
    data[x * width + y] = value;
  }

  float get(Index idx) const noexcept { return data[idx.x * width + idx.y]; }

  const std::vector<Index> makePositions() const {
    std::vector<Index> v(width * height);
    size_t i = 0;
    for (auto &position : v) {
      position = getIndex(i++);
    }
    return v;
  }
  const std::vector<Index> getPositions() const {
    return positions;

  }
  void setZero() {
    for (auto &pixel : data) {
      pixel = M_PI / 2.0f;
    }
  }

  static Image conv2d(const Image &input, const Image &filter) {
    Image result(input.width, input.height);
    const auto& inputPositions = input.getPositions();
    std::for_each(
        std::execution::par, std::begin(inputPositions), std::end(inputPositions),
        [&](Index pos) { result.set(pos, getConvPixel(input, pos, filter)); });
    return result;
  }

private:
  std::vector<float> data;
  std::vector<Index> positions;
  static float getConvPixel(const Image &input, const Image::Index &pos,
                            const Image &filter) {
    float result = 0.0f;
    size_t i = 0;
    for (const auto pixel : filter.read()) {
      auto filterIndex = filter.getIndex(i);
      ++i;
      auto halfShape = filter.shape() / 2;
      int xshift = filterIndex.x - halfShape.x;
      int yshift = filterIndex.y - halfShape.y;
      Index index(pos.x + xshift, yshift + pos.y);
      result += pixel * input.get(index);
    }
    return fmod(result, M_PI_2 * 10);
  }
};

class PixelBackEnd {
public:
  PixelBackEnd(size_t width, size_t height, size_t convSize)
      : image(width, height), filter(convSize, convSize){};
  void step() { image = Image::conv2d(image, filter); }
  float get(size_t x, size_t y) const { return image.get(Image::Index(x, y)); }
  void setFilter(Image f) { filter = f; }

  void setZero() { image.setZero(); }

  // Places a point with a given value in the middle of the canvas.
  void seed(float sum) {
    bool horizontalOdd = image.width % 2;
    bool verticalOdd = image.height % 2;
    if (horizontalOdd) {
      if (verticalOdd) {
        Image::Index idx(image.width / 2, image.height / 2);
        image.set(idx, sum);
      } else {
        image.set(image.width / 2, image.height / 2, sum / 2.0f);
        image.set(image.width / 2, (image.height / 2) + 1, sum / 2.0f);
      }
    } else {
      if (verticalOdd) {
        image.set(image.width / 2, (image.height / 2) + 1, sum / 2.0f);
        image.set(image.width / 2, image.height / 2, sum / 2.0f);
      } else {
        image.set(image.width / 2, image.height / 2, sum / 4.0f);
        image.set(image.width / 2, (image.height / 2) + 1, sum / 4.0f);
        image.set((image.width / 2) + 1, image.height / 2, sum / 4.0f);
        image.set((image.width / 2) + 1, (image.height / 2) + 1, sum / 4.0f);
      }
    }
  }

private:
  Image image;
  Image filter;
};

std::ostream &operator<<(std::ostream &os, const Image &image) {
  size_t counter = 0;
  for (const auto &pixel : image.read()) {
    os << pixel << ", ";
    ++counter;
    if (counter >= image.width) {
      counter = 0;
      std::cout << std::endl;
    }
  }
  return os;
}
namespace filters {
Image normalize(const Image &filter) {
  float sum = 0.0f;
  auto data = filter.read();
  Image out(filter);
  sum = std::reduce(data.begin(), data.end(), 0.0f);
  return filter / sum;
}

Image circular(size_t size, float gain) {
  Image filter(size, size);
  float center = 0;
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      const int rx = x - size / 2;
      const int ry = y - size / 2;
      float centerDistance = distance({center, center}, {rx, ry});
      filter.set(x, y, 1.0f / (1 + centerDistance));
    }
  }
  filter = normalize(filter);
  return filter * gain;
}

} // namespace filters

class ConvolutionVisualizer : public olc::PixelGameEngine {
public:
  ConvolutionVisualizer(size_t sceneSize, size_t filterSize)
      : scene(sceneSize, sceneSize, filterSize), positions(makePositions(sceneSize, sceneSize)) {
    sAppName = "ConvolutionVisualizer";
    scene.seed(1.0f);
    scene.setFilter(filters::circular(filterSize, 1.0f));
  }
  struct Position {
    Position(size_t a, size_t b){
      x = a;
      y = b;
    }
    size_t x;
    size_t y;
  };

public:
  bool OnUserCreate() override {
    scene.setZero();
    return true;
  }

  bool OnUserUpdate(float fElapsedTime) override {
    auto start = std::chrono::system_clock::now();
    std::for_each(std::execution::par, std::begin(positions),
                  std::end(positions), [&](Position pos) {
                    const float value = scene.get(pos.x, pos.y);
                    olc::Pixel pixel = olc::PixelF(0, 0, sin(value) + 1.0f);
                    Draw(pos.x, pos.y, pixel);
                  });
    scene.step();
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration<float>(end - start);
    std::cout << "FPS: " << 1.0f / diff.count() << std::endl;
    return true;
  }

private:
  PixelBackEnd scene;
  const std::vector<Position> positions;
  const std::vector<Position> makePositions(size_t width, size_t height) {
    std::vector<Position> v;
    for (size_t x = 0; x < width; ++x) {
      for (size_t y = 0; y < width; ++y) {
        v.emplace_back(Position(x, y));
      }
    }
    return v;
  }
};

int main() {
  size_t size = 311;
  ConvolutionVisualizer demo(size, 7);
  if (demo.Construct(size, size, 4, 4))
    demo.Start();
  return 0;
}
