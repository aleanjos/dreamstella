#ifndef OPTIONS_MENU_HXX
#define OPTIONS_MENU_HXX

#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "OSystem.hxx"
#include "EventHandler.hxx"

#include "GraphicsUtils.hxx"

class OptionsMenu
{
public:
   OptionsMenu(EventHandler *EventHandler);
   ~OptionsMenu();

   bool showOptionsMenu();

private:
   void refreshBackground(uint16_t *backgroundImageBuffer);

   bool exitMenu = false;
   bool returnToGame = true;

   uint16_t currentItem;
   int16_t cursorPosition;
   u_int16_t inputDelay;

protected:
   EventHandler *myEventHandler;
};

#endif
