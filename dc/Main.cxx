#include <kos.h>
#include <cstdlib>
#include <string>

#include "Console.hxx"
#include "Settings.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"

#include "Disc.hxx"
#include "GraphicsUtils.hxx"
#include "RomsMenu.hxx"
#include "SplashScreen.hxx"

KOS_INIT_FLAGS(INIT_DEFAULT);

OSystem *theOSystem = nullptr;
Settings *settings = nullptr;
GraphicsUtils *graphicsUtils = nullptr;
RomsMenuDC *romsMenu = nullptr;


int main(int argc, char *argv[])
{
    scanRoms();

    showSplashScreen("/cd/theme/splash-screen.png", 5000);

    graphicsUtils = new GraphicsUtils();
    romsMenu = new RomsMenuDC();
    theOSystem = new OSystem();
    settings = new Settings(theOSystem);
    
    theOSystem->create();

    for (;;)
    {
        romsMenu->showRomsMenu();
        std::string selectedGame = romsMenu->getSelectedGame();

        if (selectedGame != "")
        {
            FilesystemNode romNode(selectedGame);
            theOSystem->createConsole(romNode);
            theOSystem->mainLoop();
            theOSystem->deleteConsole();
        }
    }

    return 0;
}
