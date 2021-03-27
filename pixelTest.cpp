#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <boost/iterator/zip_iterator.hpp>
#include <chrono>
#include <execution>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>

float MOUSE_ADD = 100.0;
float MOUSE_MULT = 3;

inline double distance(const std::pair<double, double> &from,
                       const std::pair<double, double> &to) {
  return sqrt(pow(from.first - to.first, 2) + pow(from.second - to.second, 2));
}

class Image {
public:
  Image(const size_t &width, const size_t &height, bool isFilter = false)
      : width(width), height(height), data_(width * height),
        positions_(makePositions(isFilter)){};

  Image(const Image &other)
      : width(other.width), height(other.height),
        data_(other.width * other.height) {
    data_ = other.data_;
    positions_ = other.positions_;
  }

  Image(const size_t width, const size_t height,
        std::vector<float> &&inData) noexcept
      : width(width), height(height), positions_(makePositions()) {
    data_ = std::move(inData);
  };

  Image &operator=(const Image &other) {
    data_ = other.data_;
    return *this;
  }

  const Image operator/(const float divisor) const {
    Image out(width, height);
    auto &outData = out.data_;
    const auto &inData = read();
    const auto divide = [&divisor](const float &dividend) -> float {
      float result = dividend / divisor;
      return result;
    };
    std::transform(inData.begin(), inData.end(), outData.begin(), divide);
    return out;
  }

  float average() {
    auto sum = std::reduce(std::begin(data_), std::end(data_), 0.0f);
    return sum / data_.size();
  }

  const Image operator*(const float &factor) const {
    Image out(width, height);
    auto &outData = out.data_;
    const auto &inData = read();
    std::transform(inData.begin(), inData.end(), std::begin(outData),
                   [&factor](float f) -> float { return f * factor; });
    return out;
  }

  const Image operator+(const float &factor) const {
    Image out(width, height);
    auto &outData = out.data_;
    const auto &inData = read();
    std::transform(inData.begin(), inData.end(), std::begin(outData),
                   [&factor](float f) -> float { return f + factor; });
    return out;
  }

  const Image operator-(const float &factor) const {
    Image out(width, height);
    auto &outData = out.data_;
    const auto &inData = read();
    std::transform(inData.begin(), inData.end(), std::begin(outData),
                   [&factor](float f) -> float { return f - factor; });
    return out;
  }

  Image &operator=(const Image &&other) noexcept {
    data_ = std::move(other.data_);
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Image &image);
  const size_t width;
  const size_t height;

  struct Index {
    Index(size_t x, size_t y) : x(x), y(y){};
    Index() : x(0), y(0){};
    int x;
    int y;

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

  bool indexOutside(const Index &i) const {
    const size_t &x = i.x;
    const size_t &y = i.y;
    return (x >= width || y >= height);
  };

  const std::vector<float> &read() const { return data_; }

  const Index getIndex(const uint i) const {
    return Index(i / width, i % height);
  }

  void set(const Index &idx, float value) noexcept {
    data_[idx.x * width + idx.y] = value;
  }

  void set(size_t x, size_t y, float value) noexcept {
    data_[x * width + y] = value;
  }

  void add(size_t x, size_t y, float value) noexcept {
    data_[x * width + y] +=  value;
  }

  void mult(size_t x, size_t y, float factor) noexcept {
    data_[(x * width) + y] *=  factor;
    //std::cout << "Setting " << x << ":" << y << " to: " << factor << "\n";
    //std::cout << "abs: " <<  (x * width) + y << "\n";
  }

  [[nodiscard]] float get(const Index &idx) const noexcept {
    return data_[idx.x * width + idx.y];
  }

  [[nodiscard]] float get(size_t x, size_t y) noexcept {
    return data_[x * width + y];
  }

  const std::vector<Index> makePositions(bool relative = false) const {
    std::vector<Index> v(width * height);
    int i = 0;
    for (auto &position : v) {
      position = getIndex(i++);
      if (relative) {
        position.x -= width / 2;
        position.y -= height / 2;
      }
    }
    return v;
  }

  const std::vector<Index> &getPositions() const { return positions_; }
  void setZero() {
    for (auto &pixel : data_) {
      pixel = 0.0f;
    }
  }

  static Image conv2d(const Image &input, const Image &filter) {
    Image result(input.height, input.width);
    auto &outData = result.data_;
    const auto &inputPositions = input.getPositions();
    std::transform(
        std::execution::par, std::begin(inputPositions),
        std::end(inputPositions), std::begin(outData),
        [&](auto pos) -> float { return getConvPixel(input, pos, filter); });
    return result;
  }

private:
  std::vector<float> data_;
  std::vector<Index> positions_;
  static float getConvPixel(const Image &input, const Image::Index &pos,
                            const Image &filter) {
    std::vector<float> results(filter.read().size());
    const auto &filterData = filter.read();
    auto positions = filter.getPositions();
    const int w = input.width;
    const int h = input.height;
    std::for_each(std::begin(positions), std::end(positions),
                  [&pos, &w, &h](Index &index) {
                    index.x = (index.x + pos.x) % w;
                    index.y = (index.y + pos.y) % h;
                  });
    const auto beginnings = boost::make_zip_iterator(
        boost::make_tuple(std::begin(filterData), std::begin(positions)));
    const auto ends = boost::make_zip_iterator(
        boost::make_tuple(std::end(filterData), std::end(positions)));
    std::transform(beginnings, ends, std::begin(results),
                   [&](const auto &valuePosPair) {
                     return boost::get<0>(valuePosPair) *
                            input.get(boost::get<1>(valuePosPair));
                   });
    const auto result =
        std::reduce(std::begin(results), std::end(results), 0.0f);
    return result;
  }
};

class PixelBackEnd {
public:
  PixelBackEnd(size_t width, size_t height, size_t convSize)
      : image(width, height), filter(convSize, convSize, true){};
  void step() { image = Image::conv2d(image, filter); }
  float get(size_t x, size_t y) const { return image.get(Image::Index(x, y)); }
  void setFilter(Image f) { filter = f; }
  void setZero() { image.setZero(); }
  void add(size_t x, size_t y) { image.add(x, y, MOUSE_ADD); }
  void mult(size_t x, size_t y) { image.mult(x, y, MOUSE_MULT); }

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
    }
  }
  return os;
}

namespace filters {
Image normalize(const Image &filter) {
  float sum = 0.0f;
  auto data = filter.read();
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

Image ring(size_t size, float gain) {
  Image filter(size, size);
  float center = 0;
  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      const int rx = x - size / 2;
      const int ry = y - size / 2;
      float centerDistance = distance({center, center}, {rx, ry});
      auto magnitude = fabs(centerDistance - (size / 2));
      filter.set(x, y, magnitude);
    }
  }
  filter = normalize(filter);
  filter = (filter - filter.average()) * 2;
  return filter;
}

} // namespace filters

class ConvolutionVisualizer : public olc::PixelGameEngine {
public:
  ConvolutionVisualizer(size_t sceneSize, size_t filterSize, float gain)
      : scene(sceneSize, sceneSize, filterSize),
        positions(makePositions(sceneSize, sceneSize)) {
    sAppName = "ConvolutionVisualizer";
    scene.seed(30000.0f);
    scene.setFilter(filters::circular(filterSize, gain));
  }
  struct Position {
    Position(size_t a, size_t b) {
      x = a;
      y = b;
    }
    size_t x;
    size_t y;
  };

public:
  bool OnUserCreate() override {
    // scene.setZero();
    return true;
  }

  bool OnUserUpdate(float fElapsedTime) override {
    auto start = std::chrono::system_clock::now();
    // std::for_each(std::execution::par, std::begin(positions),
    //              std::end(positions), [&](Position pos) {
    //                const float value = scene.get(pos.x, pos.y);
    //                const auto r = (sin(value) + 1.0f) / 2.0f;
    //                const auto g = (sin(value * 3.0f) + 1.0f) / 2.0f;
    //                const auto b = (sin(value * 5.0f) + 1.0f) / 2.0f;
    //                olc::Pixel pixel = olc::PixelF(r, g, b);
    //                Draw(pos.x, pos.y, pixel);
    //              });
    std::for_each(std::execution::par, std::begin(positions),
                  std::end(positions), [&](Position pos) {
                    const float value =
                        (sin(scene.get(pos.x, pos.y)) + 1.0f) / 2.0f;
                        olc::Pixel pixel = olc::PixelF(value, value, value);
                    Draw(pos.x, pos.y, pixel);
                  });
    if (GetMouse(0).bHeld) {
      scene.add(GetMouseX(), GetMouseY());
      scene.mult(GetMouseX(), GetMouseY());
    }
    scene.step();
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration<float>(end - start);
    // std::cout << "FPS: " << 1.0f / diff.count() << std::endl;
    return true;
  }

private:
  PixelBackEnd scene;
  const std::vector<Position> positions;
  const std::vector<Position> makePositions(size_t width, size_t height) {
    std::vector<Position> v;
    v.reserve(width * height);
    for (size_t x = 0; x < width; ++x) {
      for (size_t y = 0; y < width; ++y) {
        v.emplace_back(x, y);
      }
    }
    return v;
  }
};

int main() {
  size_t size = 512;
  ConvolutionVisualizer demo(size, 5, 0.9998);
  if (demo.Construct(size, size, 4, 4))
    demo.Start();
  return 0;
}
