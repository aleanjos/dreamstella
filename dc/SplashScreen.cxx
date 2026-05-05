#include "GraphicsUtils.hxx"
#include <kos.h>
#include <malloc.h>

#include "SplashScreen.hxx"

void showSplashScreen(char *imagePath, uint16_t timeInMilliseconds)
{
    GraphicsUtils graph;
    Image image = graph.loadImage(imagePath);

    vid_clear(0, 0, 0);

    uint16_t *backbuffer = (uint16_t *)memalign(32, 640 * 480 * 2);

    if (image.pixels)
        memcpy(backbuffer, image.pixels, 640 * 480 * 2);
    else
        memset(backbuffer, 0, 640 * 480 * 2);

    memcpy(vram_s, backbuffer, 640 * 480 * 2);

    vid_waitvbl();

    thd_sleep(timeInMilliseconds);

    graph.freeImage(image);

    if (backbuffer)
        free(backbuffer);

    vid_clear(0, 0, 0);
}
