#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "menuGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "settingsProvider.hpp"
#include "stringsProvider.hpp"
#include "hardwareProvider.hpp"

typedef scrollbar TScrollbar;

int doMenu(Menu* menu, MenuItemIcon* icontable) {
  /* returns an integer depending on what the user did.
   * selection is on menu->selection. menu->selection starts at 1!
   */
  // char Y where to start drawing the menu items. Having a title increases this by one:
  int itemsStartY=menu->startY;

  int itemsHeight=menu->height;
  int showTitle = menu->title != NULL;
  int showScrollbar = menu->numitems > menu->height - showTitle ? 1 : 0;
  if (showTitle) {
    itemsStartY++;
    itemsHeight--;
  }
  if(menu->selection < 1) menu->selection = 1;
  else if(menu->selection > menu->numitems) menu->selection = menu->numitems;
  if(menu->selection > menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems))
    menu->scroll = menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
  if(menu->selection-1 < menu->scroll)
    menu->scroll = menu->selection -1;
  
  while(1) {
    if(menu->statusText != NULL) DefineStatusMessage(menu->statusText, 1, 0, 0);
    // Clear the area of the screen we are going to draw on
    if(0 == menu->pBaRtR)
      drawRectangle(18*(menu->startX-1), 24*(menu->miniMiniTitle ? itemsStartY:menu->startY),
                    18*menu->width+(menu->scrollout || menu->width == 21 ?6:0),
                    24*menu->height-(menu->miniMiniTitle ? 24:0), COLOR_WHITE);
    if (menu->numitems>0) {
      for(int curitem=menu->scroll;
          menu->scroll > curitem-itemsHeight && curitem < menu->numitems;
          curitem++) { // print the menu item only when appropriate
        char menuitemarr[70];
        char* menuitem = menuitemarr;
        if(menu->type == MENUTYPE_MULTISELECT) {
          //allow for the folder and selection icons on MULTISELECT menus (e.g. file browser):
          strcpy(menuitem, "  ");
          menuitem += 2;
        }
        menuitem += strncpy_retlen(menuitem, menu->items[curitem].text, 67);
        if(menu->items[curitem].type != MENUITEM_SEPARATOR) {
          // make sure we have a string big enough to have background when item is selected:          
          // MB_ElementCount is used instead of strlen because multibyte chars count as two with
          // strlen, while graphically they are just one char, making fillerRequired become wrong
          int fillerRequired = menu->width +
                               // if the menu has the full width,
                               // fill the half-character at the end of the screen:
                               (menu->width == 21 ? 1 : 0) -
                               MB_ElementCount(menu->items[curitem].text) -
                               (menu->type == MENUTYPE_MULTISELECT ? 2 : 0);
          for(int i = 0; i < fillerRequired; i++) {
            *menuitem = ' ';
            menuitem++;
          }
          *menuitem = 0;
          mPrintXY(menu->startX, curitem+itemsStartY-menu->scroll, menuitemarr,
                   menu->selection == curitem+1 ? TEXT_MODE_INVERT:TEXT_MODE_TRANSPARENT_BACKGROUND,
                   menu->items[curitem].color);
        } else {
          int textX = (menu->startX-1) * 18;
          int textY = curitem*24+itemsStartY*24-menu->scroll*24-24+6;
          clearLine(menu->startX, curitem+itemsStartY-menu->scroll,
                    menu->selection == curitem+1 ?
                          textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE);
          drawLine(textX, textY+24-4, LCD_WIDTH_PX-2, textY+24-4, COLOR_GRAY);
          PrintMini(&textX, &textY, menuitemarr, 0, 0xFFFFFFFF, 0, 0, 
                    menu->selection == curitem+1 ?
                          COLOR_WHITE : textColorToFullColor(menu->items[curitem].color),
                    menu->selection == curitem+1 ?
                          textColorToFullColor(menu->items[curitem].color) : COLOR_WHITE,
                    1, 0);
        }
        // deal with menu items of type MENUITEM_CHECKBOX
        if(menu->items[curitem].type == MENUITEM_CHECKBOX) {
          mPrintXY(menu->startX+menu->width-1,curitem+itemsStartY-menu->scroll,
                   menu->items[curitem].value == MENUITEM_VALUE_CHECKED ? "\xe6\xa9" : "\xe6\xa5",
                   menu->selection == curitem+1 ?
                       TEXT_MODE_INVERT : (menu->pBaRtR == 1 ?
                                           TEXT_MODE_TRANSPARENT_BACKGROUND : TEXT_MODE_NORMAL),
                   menu->items[curitem].color);
        }
        // deal with multiselect menus
        if(menu->type == MENUTYPE_MULTISELECT) {
          if((curitem+itemsStartY-menu->scroll)>=itemsStartY &&
            (curitem+itemsStartY-menu->scroll)<=(itemsStartY+itemsHeight) &&
            icontable != NULL
          ) {
            unsigned short mask = getHardwareModel() == 3 ? 0xFBE0 : 0xf81f;
            
            if (menu->items[curitem].isfolder == 1) {
              // assumes first icon in icontable is the folder icon
              CopySpriteMasked(icontable[0].data, (menu->startX)*18,
                               (curitem+itemsStartY-menu->scroll)*24, 0x12, 0x18, mask);
            } else if(menu->items[curitem].icon >= 0) {
              CopySpriteMasked(icontable[menu->items[curitem].icon].data, (menu->startX)*18,
                               (curitem+itemsStartY-menu->scroll)*24, 0x12, 0x18, mask);
            }
          }
          if (menu->items[curitem].isselected) {
            if (menu->selection == curitem+1) {
              mPrintXY(menu->startX, curitem+itemsStartY-menu->scroll, "\xe6\x9b",
                       TEXT_MODE_TRANSPARENT_BACKGROUND,
                       menu->items[curitem].color == TEXT_COLOR_GREEN ?
                              TEXT_COLOR_BLUE : TEXT_COLOR_GREEN);
            } else {
              mPrintXY(menu->startX, curitem+itemsStartY-menu->scroll, "\xe6\x9b",
                       TEXT_MODE_NORMAL, TEXT_COLOR_PURPLE);
            }
          }
        }
      }
      if (showScrollbar) {
        TScrollbar sb;
        sb.I1 = 0;
        sb.I5 = 0;
        sb.indicatormaximum = menu->numitems;
        sb.indicatorheight = itemsHeight;
        sb.indicatorpos = menu->scroll;
        sb.barheight = itemsHeight*24;
        sb.bartop = (itemsStartY-1)*24;
        sb.barleft = menu->startX*18+menu->width*18 - 18 - (menu->scrollout ? 0 : 5);
        sb.barwidth = 6;
        Scrollbar(&sb);
      }
      if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0)
        drawFkeyLabels(0x0037); // SELECT (white)
    } else if(menu->nodatamsg != NULL) {
      printCentered(menu->nodatamsg, (itemsStartY*24)+(itemsHeight*24)/2-12,
                    COLOR_BLACK, COLOR_WHITE);
    }
    if(showTitle) {
      if(menu->miniMiniTitle) {
        int textX = 0, textY=(menu->startY-1)*24;
        PrintMiniMini(&textX, &textY, menu->title, 16, menu->titleColor, 0);
      } else
        mPrintXY(menu->startX, menu->startY, menu->title, TEXT_MODE_TRANSPARENT_BACKGROUND,
                 menu->titleColor);
      if(menu->subtitle != NULL) {
        int textX=(MB_ElementCount(menu->title)+menu->startX-1)*18+10, textY=6;
        PrintMini(&textX, &textY, menu->subtitle, 0, 0xFFFFFFFF, 0, 0,
                  COLOR_BLACK, COLOR_WHITE, 1, 0);
      }
    }
    if(menu->darken) {
      DrawFrame(COLOR_BLACK);
      invertArea(menu->startX*18-18, menu->startY*24,
                 menu->width*18-(menu->scrollout || !showScrollbar ? 0 : 5), menu->height*24);
    }
    if(menu->type == MENUTYPE_NO_KEY_HANDLING)
      return MENU_RETURN_INSTANT; // we don't want to handle keys
    int key;
    mGetKey(&key, menu->darken);
    switch(key) {
      case KEY_CTRL_DOWN:
        if(menu->selection == menu->numitems) {
          if(menu->returnOnInfiniteScrolling) {
            return MENU_RETURN_SCROLLING;
          } else {
            menu->selection = 1;
            menu->scroll = 0;
          }
        } else if(++menu->selection >
                menu->scroll+(menu->numitems>itemsHeight ? itemsHeight : menu->numitems)) {
          menu->scroll =
                    menu->selection -(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
        }
        if(menu->pBaRtR==1)
          return MENU_RETURN_INSTANT;
        break;
      case KEY_CTRL_UP:
        if(menu->selection == 1) {
          if(menu->returnOnInfiniteScrolling) {
            return MENU_RETURN_SCROLLING;
          } else {
            menu->selection = menu->numitems;
            menu->scroll =
                      menu->selection-(menu->numitems>itemsHeight ? itemsHeight : menu->numitems);
          }
        } else {
          menu->selection--;
          if(menu->selection-1 < menu->scroll)
            menu->scroll = menu->selection -1;
        }
        if(menu->pBaRtR==1)
          return MENU_RETURN_INSTANT;
        break;
      case KEY_CTRL_F1:
        if(menu->type==MENUTYPE_MULTISELECT && menu->fkeypage == 0 && menu->numitems > 0) {
          if(menu->items[menu->selection-1].isselected) {
            menu->items[menu->selection-1].isselected=0;
            menu->numselitems = menu->numselitems-1;
          } else {
            menu->items[menu->selection-1].isselected=1;
            menu->numselitems = menu->numselitems+1;
          }
          //return on F1 too so that parent subroutines have a chance to e.g. redraw fkeys:
          return key;
        } else if (menu->type == MENUTYPE_FKEYS) {
          return key;
        }
        break;
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
      case KEY_CTRL_F5:
      case KEY_CTRL_F6:
      case KEY_CTRL_DEL:
        if (menu->type == MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT)
          return key; // MULTISELECT also returns on Fkeys
        break;
      case KEY_CTRL_PASTE:
      case KEY_CTRL_CLIP:
        if (menu->type==MENUTYPE_MULTISELECT)
          return key; // MULTISELECT also returns on paste and clip
        break;
      case KEY_CTRL_OPTN:
        if (menu->type==MENUTYPE_FKEYS || menu->type==MENUTYPE_MULTISELECT)
          return key;
        break;
      case KEY_CTRL_FORMAT:
        if (menu->type==MENUTYPE_FKEYS)
          return key; // return on the Format key, to change event categories for example
        break;
      case KEY_CTRL_RIGHT:
        if(!menu->returnOnRight) break;
        // else fallthrough
      case KEY_CTRL_EXE:
        if(menu->numitems>0)
          return MENU_RETURN_SELECTION;
        break;
      case KEY_CTRL_LEFT:
        if(!menu->returnOnLeft) break;
        // else fallthrough
      case KEY_CTRL_EXIT: return MENU_RETURN_EXIT;
        break;
      case KEY_CHAR_1:
      case KEY_CHAR_2:
      case KEY_CHAR_3:
      case KEY_CHAR_4:
      case KEY_CHAR_5:
      case KEY_CHAR_6:
      case KEY_CHAR_7:
      case KEY_CHAR_8:
      case KEY_CHAR_9:
        if(menu->numitems>=(key-0x30)) {
          menu->selection = (key-0x30);
          return MENU_RETURN_SELECTION;
        }
        break;
      case KEY_CHAR_0:
        if(menu->numitems>=10) {menu->selection = 10; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CTRL_XTT:
        if(menu->numitems>=11) {menu->selection = 11; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LOG:
        if(menu->numitems>=12) {menu->selection = 12; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LN:
        if(menu->numitems>=13) {menu->selection = 13; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_SIN:
      case KEY_CHAR_COS:
      case KEY_CHAR_TAN:
        if(menu->numitems>=(key-115)) {menu->selection = (key-115); return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_FRAC:
        if(menu->numitems>=17) {menu->selection = 17; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CTRL_FD:
        if(menu->numitems>=18) {menu->selection = 18; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_LPAR:
      case KEY_CHAR_RPAR:
        if(menu->numitems>=(key-21)) {menu->selection = (key-21); return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_COMMA:
        if(menu->numitems>=21) {menu->selection = 21; return MENU_RETURN_SELECTION; }
        break;
      case KEY_CHAR_STORE:
        if(menu->numitems>=22) {menu->selection = 22; return MENU_RETURN_SELECTION; }
        break;
    }
    if(menu->type == MENUTYPE_INSTANT_RETURN) return MENU_RETURN_INSTANT;
  }
  return MENU_RETURN_EXIT;
}

int getMenuSelectionSeparators(Menu* menu, int* noSeparators, int* onlySeparators) {
  *onlySeparators = 0;
  *noSeparators = 0;
  for(int i = 0; i<=menu->selection-1; i++) {
    if(menu->items[i].type == MENUITEM_SEPARATOR) (*onlySeparators)++;
    else (*noSeparators)++;
  }
  return menu->items[menu->selection-1].type == MENUITEM_SEPARATOR;
}

// not really related to the menu, but had to go somewhere as a general GUI helper:

int closeMsgBox(int yesno, int msgY) {
  // if yesno is true, returns 1 on yes and 0 on no.
  // else, waits for user to exit a simple info box, and calls MsgBoxPop for you!
  // draws appropriate message(s) at provided locations
  if(!yesno) PrintXY_2(TEXT_MODE_NORMAL, 1, msgY, 2, TEXT_COLOR_BLACK); // press exit message
  else {
    PrintXY_2(TEXT_MODE_NORMAL, 1, msgY, 3, TEXT_COLOR_BLACK); // yes, F1
    PrintXY_2(TEXT_MODE_NORMAL, 1, msgY+1, 4, TEXT_COLOR_BLACK); // no, F6
  }
  int key;
  while(1) {
    mGetKey(&key);
    if(!yesno) switch(key) {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_AC:
        mMsgBoxPop(); 
        return 0;
    }
    else switch(key) {
      case KEY_CTRL_F1:
        mMsgBoxPop();
        return 1;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        mMsgBoxPop();
        return 0;
    }
  }
}