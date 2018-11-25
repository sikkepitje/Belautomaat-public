/*
  Pmenu.h
  20180804 Paul Wiegmans (p.wiegmans@bonhoeffer.nl)
*/

#ifndef Pwmenu_h
#define Pwmenu_h

#include "Arduino.h"
#include "Belautomaat_lib.h"

#define MENU_BUFLEN (20)

/*=== karakters voor menu ===*/
#define EERSTE_ITEM char(1)
#define MIDDELSTE_ITEM char(2)
#define LAATSTE_ITEM char(3)
#define ENIGE_ITEM char(4)
#define ITEM_RECHTS char(5)
#define ITEM_LINKS char(6)
#define ITEM_PARENTLESS ' '
#define ITEM_CHILDLESS ' '

class MenuItem
{
  private:
    const char *_name;
    int _id;
    MenuItem *child;
    MenuItem *parent;
    MenuItem *previousSibling;
    MenuItem *nextSibling;

  public:
    MenuItem(const char *name);
    MenuItem(const int id, const char *name);
    void addSibling(MenuItem &s);
    void addSibling2(MenuItem &s, MenuItem &p);
    void addChild(MenuItem &c);
    bool hasChild();
    bool hasParent();
    MenuItem *getPreviousSibling();
    MenuItem *getNextSibling();
    MenuItem *getParent();
    MenuItem *getChild();
    const char *getName();
    int getId();
    const char *getParentName();
};

class PWmenu
{
  private:
    MenuItem *root;
    MenuItem *current;
    const char *name;     // kopie van MenuItem.name voor gemak
    bool used;      // flag if user chose SELECT in MenuItem without child
    bool exited;    // flas if user chose BACK in MenuItem without parent
    int rows, cols; // size of view
    int depth;      // current level depth, starts at 0 and counting up
    char viewbuffer[MENU_BUFLEN];

  public:
    PWmenu(MenuItem &newroot);
    void setViewSize(int viewrows, int viewcols);
    void display(); // display menu line(s) on LCD
    void goUp();             // current menu go up
    void goDown();           // current menu go down
    void goSelect();         // current menu select  or go to submenu if any
    void goBack();           // current menu back / go to parent
    void goRoot();          // ga naar root
    bool isUsed();           // flag when user selected last child MenuItem
    bool isExited();         // flas when user backed out of last parent
    const char *getMenu();   // return current menu name
    const int getId();
    MenuItem *getCurrent();  // return current menu (pointer)
};

#endif // ifdef Pwmenu_h