#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GraphicsUtils.hxx"

GraphicsUtils::GraphicsUtils()
{
    screenWidth = vid_mode->width;
    screenHeight = vid_mode->height;

    for (int i = 0; i < 256; i++)
    {
        _font11px.table[i] = {0, 0, 0, 0, 0, 0, 8};
        _font16px.table[i] = {0, 0, 0, 0, 0, 0, 8};
    }

    _font11px = loadFont("/rd/theme/font11px.fnt", "/rd/theme/font11px.png");
    _font16px = loadFont("/rd/theme/font16px.fnt", "/rd/theme/font16px.png");
}

GraphicsUtils::~GraphicsUtils()
{
    freeImage(_font11px.texture);
    freeImage(_font16px.texture);
}

Image GraphicsUtils::loadImage(const char *path)
{
    int w, h, n;

    unsigned char *data = stbi_load(path, &w, &h, &n, 4);
    if (!data)
        return {nullptr, 0, 0};

    uint16_t *pixels = (uint16_t *)malloc(w * h * sizeof(uint16_t));
    if (!pixels)
    {
        stbi_image_free(data);
        return {nullptr, 0, 0};
    }

    for (int i = 0; i < w * h; i++)
    {
        int a = data[i * 4 + 3];

        if (a < 128)
        {
            pixels[i] = 0x0000;
        }
        else
        {
            int r = data[i * 4] >> 3;
            int g = data[i * 4 + 1] >> 2;
            int b = data[i * 4 + 2] >> 3;

            uint16_t color = (r << 11) | (g << 5) | b;

            if (color == 0x0000)
                color = Color::Transparent;

            pixels[i] = color;
        }
    }

    stbi_image_free(data);
    return {pixels, w, h};
}

void GraphicsUtils::drawImage(uint16_t *buffer, int x, int y, const Image &img)
{
    if (!img.pixels)
        return;

    for (int py = 0; py < img.height; py++)
    {
        for (int px = 0; px < img.width; px++)
        {
            int screenX = x + px;
            int screenY = y + py;

            if (screenX >= 0 && screenX < screenWidth && screenY >= 0 && screenY < screenHeight)
            {
                uint16_t pixel = img.pixels[py * img.width + px];

                if (pixel != 0x0000)
                {
                    buffer[screenY * screenWidth + screenX] = pixel;
                }
            }
        }
    }
}

void GraphicsUtils::freeImage(Image &img)
{
    if (img.pixels)
    {
        free(img.pixels);
        img.pixels = nullptr;
    }
    img.width = 0;
    img.height = 0;
}

FontData GraphicsUtils::loadFont(const char *fontPath, const char *texturePath)
{
    FILE *file = fopen(fontPath, "r");
    FontData fontData;

    if (!file)
    {
        printf("Font file not loaded.\n");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "char id=", 8) == 0)
        {
            int id;
            MetricsLetter m;
            if (sscanf(line, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
                       &id, &m.x, &m.y, &m.w, &m.h, &m.xoffset, &m.yoffset, &m.xadvance) == 8)
            {
                if (id >= 0 && id < 256)
                {
                    fontData.table[id] = m;
                }
            }
        }
    }

    fclose(file);

    fontData.texture = loadImage(texturePath);

    return fontData;
}

void GraphicsUtils::drawText(uint16_t *buffer, int x, int y, const std::string &text, uint16_t color)
{
    if (!_font11px.texture.pixels && !_font16px.texture.pixels)
        return;

    int curX = x;
    for (char c : text)
    {
        MetricsLetter &m = screenWidth == 320 ? _font11px.table[(unsigned char)c] : _font16px.table[(unsigned char)c];

        for (int py = 0; py < m.h; py++)
        {
            for (int px = 0; px < m.w; px++)
            {
                uint16_t p = screenWidth == 320 ? (_font11px.texture.pixels[(m.y + py) * _font11px.texture.width + (m.x + px)])
                                                : (_font16px.texture.pixels[(m.y + py) * _font16px.texture.width + (m.x + px)]);

                if (p != 0x0000)
                {
                    int screenX = curX + px + m.xoffset;
                    int screenY = y + py + m.yoffset;

                    if (screenX >= 0 && screenX < screenWidth && screenY >= 0 && screenY < screenHeight)
                    {
                        buffer[screenY * screenWidth + screenX] = color;
                    }
                }
            }
        }

        curX += m.xadvance;
    }
}

void GraphicsUtils::drawRectangle(uint16_t *buffer, int x, int y, int w, int h, uint16_t color)
{
    for (int py = y; py < y + h; py++)
    {
        for (int px = x; px < x + w; px++)
        {
            if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight)
            {
                buffer[py * screenWidth + px] = color;
            }
        }
    }
}

void GraphicsUtils::drawTranslucentRectangle(uint16_t *buffer, int x, int y, int w, int h)
{
    for (int py = y; py < y + h; py++)
    {
        for (int px = x; px < x + w; px++)
        {
            if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight)
            {
                uint16_t bgColor = buffer[py * screenWidth + px];
                buffer[py * screenWidth + px] = blend50(bgColor, 0x0000);
            }
        }
    }
}
