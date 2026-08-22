#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>

enum MenuTab {
    TAB_UPGRADES = 0,
    TAB_BANK = 1,
    TAB_REBIRTH = 2,
    TAB_STATS = 3
};

void initGraphics();
void renderTopScreen(MenuTab currentTab, int selectedIndex);
void renderBottomScreen();
void flushBottomScreen();

#endif // GRAPHICS_H
