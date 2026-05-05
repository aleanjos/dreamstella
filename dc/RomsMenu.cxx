#include <kos.h>
#include <cstdio>
#include <string>

#include "Disc.hxx"
#include "GraphicsUtils.hxx"
#include "RomsMenu.hxx"

static int cursorPosition = 0;

RomsMenuDC::RomsMenuDC()
{
}

RomsMenuDC::~RomsMenuDC()
{
}

void RomsMenuDC::showRomsMenu()
{
    vid_clear(0, 0, 0);
    thd_sleep(50);
    vid_set_mode(DM_640x480, PM_RGB565);
    
    GraphicsUtils graph;
    _selectedGame = "";

    uint16_t *backbuffer = (uint16_t *)memalign(32, graph.screenWidth * graph.screenHeight * 2);

    if (cursorPosition >= (int)romList.size())
        cursorPosition = 0;

    if (romList.size() < 1)
    {
        memset(backbuffer, 0, graph.screenWidth * graph.screenHeight * 2);
        graph.drawText(backbuffer, 50, 240, "Rom folder is empty.", Color::White);
        memcpy(vram_s, backbuffer, graph.screenWidth * graph.screenHeight * 2);
        thd_sleep(2000);
        free(backbuffer);
        return;
    }

    int inputDelay = 0;
    int selectedPrevious = -1;

    Image backgroundImage = graph.loadImage("/cd/theme/background.png");
    Image coverImage = {nullptr, 0, 0};

    int coverTimer = 0;
    bool loadedCover = false;
    int delayNecessary = 6;

    bool exitMenu = false;

    while (!exitMenu)
    {
        if (cursorPosition != selectedPrevious)
        {
            graph.freeImage(coverImage);
            loadedCover = false;
            coverTimer = 0;
            selectedPrevious = cursorPosition;
        }

        if (!loadedCover && !romList[cursorPosition].isDirectory)
        {
            coverTimer++;
            if (coverTimer > delayNecessary)
            {
                std::string fileName = romList[cursorPosition].name;
                size_t dot = fileName.find_last_of(".");
                std::string baseName = (dot != std::string::npos) ? fileName.substr(0, dot) : fileName;

                std::string subFolder = "";
                if (currentDirectory.find("/cd/roms") == 0)
                    subFolder = currentDirectory.substr(8);

                char coverPath[256];
                snprintf(coverPath, sizeof(coverPath), "/cd/covers%s/%s.png", subFolder.c_str(), baseName.c_str());

                coverImage = graph.loadImage(coverPath);
                loadedCover = true;
            }
        }

        if (inputDelay > 0)
            inputDelay--;

        maple_device_t *joysticA = maple_enum_dev(0, 0);
        if (joysticA)
        {
            cont_state_t *contState = (cont_state_t *)maple_dev_status(joysticA);
            if (contState)
            {
                if (!(contState->buttons & (CONT_DPAD_DOWN | CONT_DPAD_UP | CONT_DPAD_LEFT | CONT_DPAD_RIGHT | CONT_A | CONT_B)))
                    inputDelay = 0;

                if (inputDelay == 0)
                {
                    if (contState->buttons & CONT_DPAD_DOWN)
                    {
                        cursorPosition++;
                        if (cursorPosition >= (int)romList.size())
                            cursorPosition = 0;
                        inputDelay = 4;
                    }
                    else if (contState->buttons & CONT_DPAD_UP)
                    {
                        cursorPosition--;
                        if (cursorPosition < 0)
                            cursorPosition = (int)romList.size() - 1;
                        inputDelay = 4;
                    }
                    else if (contState->buttons & CONT_DPAD_RIGHT)
                    {
                        cursorPosition += 15;
                        if (cursorPosition >= (int)romList.size())
                            cursorPosition = (int)romList.size() - 1;
                        inputDelay = 4;
                    }
                    else if (contState->buttons & CONT_DPAD_LEFT)
                    {
                        cursorPosition -= 15;
                        if (cursorPosition < 0)
                            cursorPosition = 0;
                        inputDelay = 4;
                    }
                    else if (contState->buttons & (CONT_A))
                    {
                        inputDelay = 8;

                        if (romList[cursorPosition].isDirectory)
                        {
                            if (romList[cursorPosition].fullPath == "..")
                            {
                                size_t lastSlash = currentDirectory.find_last_of("/");
                                if (lastSlash != std::string::npos && currentDirectory != "/cd/roms")
                                {
                                    currentDirectory = currentDirectory.substr(0, lastSlash);
                                }
                            }
                            else
                            {
                                currentDirectory = romList[cursorPosition].fullPath;
                            }
                            scanRoms();
                            cursorPosition = 1;
                        }
                        else
                        {
                            _selectedGame = romList[cursorPosition].fullPath;
                            exitMenu = true;
                        }
                    }
                    else if (contState->buttons & CONT_B)
                    {
                        inputDelay = 8;
                        if (currentDirectory != "/cd/roms" && currentDirectory != "/cd")
                        {
                            size_t lastSlash = currentDirectory.find_last_of("/");
                            if (lastSlash != std::string::npos)
                            {
                                currentDirectory = currentDirectory.substr(0, lastSlash);
                                scanRoms();
                                cursorPosition = 0;
                            }
                        }
                    }
                }
            }
        }

        if (backgroundImage.pixels)
            memcpy(backbuffer, backgroundImage.pixels, graph.screenWidth * graph.screenHeight * 2);
        else
            memset(backbuffer, 0, graph.screenWidth * graph.screenHeight * 2);

        graph.drawTranslucentRectangle(backbuffer, 20, 74, 380, 348);

        int pageStart = (cursorPosition / 15) * 15;
        for (int i = 0; i < 15 && (pageStart + i) < (int)romList.size(); i++)
        {
            int currentItem = pageStart + i;
            int y = 84 + (i * 22);

            std::string romName = romList[currentItem].name;

            size_t lastDot = romName.find_last_of(".");
            if (lastDot != std::string::npos && lastDot > 0)
                romName = romName.erase(lastDot);

            char lineText[256];
            if (currentItem == cursorPosition)
                sprintf(lineText, " %.54s", romName.c_str());
            else
                sprintf(lineText, "%.54s", romName.c_str());

            graph.drawText(backbuffer, 36, y, lineText, (currentItem == cursorPosition) ? Color::Cyan : Color::White);
        }

        if (loadedCover)
            graph.drawImage(backbuffer, 420, 164, coverImage);

        vid_waitvbl();
        memcpy(vram_s, backbuffer, graph.screenWidth * graph.screenHeight * 2);
    }

    graph.freeImage(backgroundImage);
    graph.freeImage(coverImage);

    if (backbuffer)
    {
        free(backbuffer);
        backbuffer = nullptr;
    }
}
