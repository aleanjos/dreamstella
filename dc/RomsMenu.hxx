#ifndef ROMSMENUDC_HXX
#define ROMSMENUDC_HXX

#include <string>

class RomsMenuDC
{
public:
   RomsMenuDC();
   ~RomsMenuDC();

   void showRomsMenu();

   std::string getSelectedGame() { return _selectedGame; }

private:
   std::string _selectedGame;
};

#endif
