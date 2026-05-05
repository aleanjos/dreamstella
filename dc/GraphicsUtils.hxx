#ifndef GRAPHICSDC_UTILS_HXX
#define GRAPHICSDC_UTILS_HXX

#include <kos.h>
#include <string>
#include <vector>

struct Image
{
    uint16_t *pixels = nullptr;
    int width = 0;
    int height = 0;
};

struct MetricsLetter
{
    int x, y, w, h;
    int xoffset, yoffset, xadvance;
};

struct Color {
    static constexpr int White = 0xFFFF;
    static constexpr int Cyan = 0x07FF;
    static constexpr int Transparent = 0x0001;
};

struct FontData {
    Image texture;
    MetricsLetter table[256];
};

class GraphicsUtils
{
public:
    GraphicsUtils();
    ~GraphicsUtils();

    FontData loadFont(const char *fontPath, const char *texturePath);

    Image loadImage(const char *path);
    void freeImage(Image &img);

    void drawPixel(uint16_t *buffer, int x, int y, uint16_t color);
    void drawRectangle(uint16_t *buffer, int x, int y, int w, int h, uint16_t color);
    void drawTranslucentRectangle(uint16_t *buffer, int x, int y, int w, int h);

    void drawImage(uint16_t *buffer, int x, int y, const Image &img);
    void drawText(uint16_t *buffer, int x, int y, const std::string &text, uint16_t color);

    uint16_t screenWidth;
    uint16_t screenHeight;

private:
    FontData _font11px;
    FontData _font16px;

    inline uint16_t blend50(uint16_t c1, uint16_t c2)
    {
        return ((((c1 & 0xF81F) + (c2 & 0xF81F)) >> 1) & 0xF81F) |
               ((((c1 & 0x07E0) + (c2 & 0x07E0)) >> 1) & 0x07E0);
    }
};

#endif
