#pragma once
#include <stdexcept>
#include <fstream>

struct Pixel
{
    unsigned char r, g, b;
    void setPixel(Vec3 rgb) noexcept
    {
        this->r = unsigned char(rgb.r());
        this->g = unsigned char(rgb.g());
        this->b = unsigned char(rgb.b());
    }
};

struct Image
{
    explicit Image(size_t width, size_t height)
        : width(width), height(height)
    {
        pixels = new Pixel[height * width];
    }
    Image(const Image&) = delete;
    ~Image() { delete[] pixels; }
    Pixel& getPixel(size_t x, size_t y)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            throw std::invalid_argument("bad position");
        return pixels[y * width + x];
    }
    Pixel& getPixel(size_t x, size_t y) const
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            throw std::invalid_argument("bad position");
        return pixels[y * width + x];
    }
    Pixel* getBuffer() { return pixels; }
    void clear() noexcept { memset(pixels, 0, sizeof(Pixel) * width * height); }

    Pixel* pixels;
    size_t width, height;
};

inline void writeImage(std::ofstream&& file, const Image& image)
{
    file << "P3\n" << image.width << " " << image.height << "\n255\n";
    for (int j = image.height - 1; j >= 0; j--)
        for (int i = 0; i < image.width; i++)
        {
            auto& pixel = image.getPixel(i, j);
            file << int(pixel.r) << " " << int(pixel.g) << " " << int(pixel.b) << "\n";
        }
    file.flush();
    file.close();
}
