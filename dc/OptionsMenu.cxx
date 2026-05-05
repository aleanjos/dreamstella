#include "OptionsMenu.hxx"

OptionsMenu::OptionsMenu(EventHandler *eventHandler)
    : myEventHandler(eventHandler)
{
}

OptionsMenu::~OptionsMenu()
{
}

bool OptionsMenu::showOptionsMenu()
{
   GraphicsUtils graph;

   auto &switches = myEventHandler->eventSwitches();
   auto &event = myEventHandler->event();

   switches.isAdvancedDifficultyP1 = event.get(Event::ConsoleLeftDiffA);
   switches.isAdvancedDifficultyP2 = event.get(Event::ConsoleRightDiffA);
   switches.isBlackAndWhite = event.get(Event::ConsoleBlackWhite);

   uint16_t *backgroundImageBuffer = (uint16_t *)memalign(32, graph.screenWidth * graph.screenHeight * 2);
   uint16_t *menuBuffer = (uint16_t *)memalign(32, graph.screenWidth * graph.screenHeight * 2);

   memcpy(backgroundImageBuffer, vram_s, graph.screenWidth * graph.screenHeight * 2);

   maple_device_t *joysticA = maple_enum_dev(0, 0);
   exitMenu = false;
   cursorPosition = 0;
   inputDelay = 15;

   while (!exitMenu)
   {
      if (inputDelay > 0)
         inputDelay--;

      if (joysticA)
      {
         cont_state_t *contState = (cont_state_t *)maple_dev_status(joysticA);
         if (contState)
         {
            if (!(contState->buttons & (CONT_DPAD_DOWN | CONT_DPAD_UP | CONT_A | CONT_B | CONT_START)))
               inputDelay = 0;

            if (inputDelay == 0)
            {
               if (contState->buttons & CONT_DPAD_DOWN)
               {
                  cursorPosition++;
                  if (cursorPosition > 6)
                     cursorPosition = 0;
                  inputDelay = 8;
               }
               else if (contState->buttons & CONT_DPAD_UP)
               {
                  cursorPosition--;
                  if (cursorPosition < 0)
                     cursorPosition = 6;
                  inputDelay = 8;
               }
               else if (contState->buttons & (CONT_B | CONT_START))
               {
                  exitMenu = true;
               }
               else if (contState->buttons & CONT_A)
               {
                  inputDelay = 15;

                  switch (cursorPosition)
                  {
                  case 0:
                     exitMenu = true;
                     break;

                  case 1:
                     event.set(Event::ConsoleSelect, 1);

                     inputDelay = 15;
                     refreshBackground(backgroundImageBuffer);

                     break;

                  case 2:
                     switches.isAdvancedDifficultyP1 = !switches.isAdvancedDifficultyP1;
                     event.set(Event::ConsoleLeftDiffA, switches.isAdvancedDifficultyP1);
                     event.set(Event::ConsoleLeftDiffB, !switches.isAdvancedDifficultyP1);
                     break;

                  case 3:
                     switches.isAdvancedDifficultyP2 = !switches.isAdvancedDifficultyP2;
                     event.set(Event::ConsoleRightDiffA, switches.isAdvancedDifficultyP2);
                     event.set(Event::ConsoleRightDiffB, !switches.isAdvancedDifficultyP2);
                     break;

                  case 4:
                     switches.isBlackAndWhite = !switches.isBlackAndWhite;

                     event.set(Event::ConsoleBlackWhite, switches.isBlackAndWhite);
                     event.set(Event::ConsoleColor, !switches.isBlackAndWhite);

                     inputDelay = 15;
                     refreshBackground(backgroundImageBuffer);

                     break;

                  case 5:
                     event.set(Event::ConsoleReset, 1);
                     exitMenu = true;
                     break;

                  case 6:
                     exitMenu = true;
                     myEventHandler->quit();
                     break;
                  }
               }
            }
         }
      }

      memcpy(menuBuffer, backgroundImageBuffer, graph.screenWidth * graph.screenHeight * 2);

      graph.drawTranslucentRectangle(menuBuffer, 0, 0, graph.screenWidth, graph.screenHeight);
      graph.drawText(menuBuffer, 124, 32, "Options", Color::White);
      graph.drawText(menuBuffer, 20, 65, "Resume game", (cursorPosition == 0) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 80, "Game select", (cursorPosition == 1) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 95, switches.isAdvancedDifficultyP1 ? "P1 difficulty: Advanced" : "P1 difficulty: Beginner", (cursorPosition == 2) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 110, switches.isAdvancedDifficultyP2 ? "P2 difficulty: Advanced" : "P2 difficulty: Beginner", (cursorPosition == 3) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 125, switches.isBlackAndWhite ? "Color mode: Black and white" : "Color mode: Color", (cursorPosition == 4) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 140, "Reset game", (cursorPosition == 5) ? Color::Cyan : Color::White);
      graph.drawText(menuBuffer, 20, 155, "Quit to roms menu", (cursorPosition == 6) ? Color::Cyan : Color::White);

      sq_cpy(vram_s, menuBuffer, graph.screenWidth * graph.screenHeight * 2);
      vid_waitvbl();
   }

   free(backgroundImageBuffer);
   free(menuBuffer);

   return exitMenu;
}

void OptionsMenu::refreshBackground(uint16_t *backgroundImageBuffer)
{
   GraphicsUtils graph;

   for (int i = 0; i < 3; i++)
   {
      myEventHandler->myOSystem->runFrame();
      vid_waitvbl();
   }
   
   memcpy(backgroundImageBuffer, vram_s, graph.screenWidth * graph.screenHeight * 2);
}
