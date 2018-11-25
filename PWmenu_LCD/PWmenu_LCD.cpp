/*
  Pmenu.cpp
  20180804 Paul Wiegmans (p.wiegmans@bonhoeffer.nl)

  Eenvoudig menu met ondersteuning voor LCD, onderdeel in 
  Belautomaat-hardware.
  
  This is a simple menu system for Arduino, implementing 
  MenuItem class, which has children and siblings. 
  Navigation through the menu goes typically via 'Up', 'Down',
  'Select' and 'Back' buttons.

  Top level menu is first made by creating a MenuItem, then 
  expanded by adding more menu items using the addSibling() method. 
  Submenus are added by using the addChild() method. 
  Repeatedly calling a menu items addChild() method adds new children 
  to the last in the linked list of children (so these become siblings of the child).

  Lees meer over het verschil tussen argument doorgeven 'pass by pointer' en 'pass by reference' met &.
  https://arduino.stackexchange.com/questions/24033/proper-use-of-and-when-passing-objects-in-methods

  Lees meer over const char* en zo
  https://arduino.stackexchange.com/questions/13429/deprecated-conversion-from-string-constant-to-char

  krijg je errors als deze:
  error: unable to find a register to spill in class 'NO_REGS'
  lees dan:
  https://github.com/arduino/Arduino/issues/3972
  Men wijst op een bug in AVR-GCC. Daar kan ik niet veel mee. Op:
  https://www.cooking-hacks.com/forum/viewtopic.php?f=20&t=9392
  wijst iemand erop dat sprintf soms problemen geeft. En inderdaad 
  verwijderen van sprintf() laat de boel weer correct compileren.  

  Er worden 6 zelfgedefinieerde karakters gebruikt om speciale tekens 
  weer te geven op de LCD. Deze tekens char(1) t/m  char(6) worden 
  geretourneerd als deel van een string. Hierbij is char(0) onbruikbaar. 
  Deze tekens worden op de LCD gedefinieerd in belautomaat_lib.cpp.
  Alternatief is om dit menu rechtsstreeks de LCD aan te sturen.
*/

#include "Arduino.h"
#include "PWmenu_LCD.h"

/*=== MenuItem class ===*/
MenuItem::MenuItem(const char *name) {
    _name = name;
    _id = 0;
    parent = NULL;
    child = NULL;
    nextSibling = NULL;
    previousSibling = NULL;
}

MenuItem::MenuItem(const int id, const char *name) {
    _name = name;
    _id = id;
    parent = NULL;
    child = NULL;
    nextSibling = NULL;
    previousSibling = NULL;
}

void MenuItem::addSibling(MenuItem &s) {
    if (nextSibling) {
        nextSibling->addSibling(s);
    } else {
        nextSibling = &s;
        nextSibling->previousSibling = this; 
        nextSibling->parent = parent;
    }
}
void MenuItem::addSibling2(MenuItem &s, MenuItem &p) {
    if (nextSibling) {
        nextSibling->addSibling2(s, p);
    }
    else {
        nextSibling = &s;
        nextSibling->parent = &p;
        nextSibling->previousSibling = this;
    }
}
bool MenuItem::hasChild() {
    return (child != NULL);
}
bool MenuItem::hasParent() {
    return (parent != NULL);
}
void MenuItem::addChild(MenuItem &c) {
    if (child) {
        child->addSibling2(c, *this);
    }
    else {
        child = &c;
        child->parent = this;
    }
}
MenuItem *MenuItem::getPreviousSibling() {
    return previousSibling;
}
MenuItem *MenuItem::getNextSibling() {
    return nextSibling;
}
MenuItem *MenuItem::getParent() {
    return parent;
}
MenuItem *MenuItem::getChild() {
    return child;
}
const char *MenuItem::getName() {
    return _name;
}
int MenuItem::getId() {
    return _id;
}
const char *MenuItem::getParentName() {
    if (parent) {
        return parent->_name;
    } 
    else {
        return NULL;
    }
}
/*=== PWMenu class ======================================================*/

PWmenu::PWmenu(MenuItem &newroot)
{
    root = &newroot;
    goRoot();
}

void PWmenu::setViewSize(int viewrows, int viewcols) {
    rows = viewrows;  // not used
    cols = viewcols;  // not used
}

void PWmenu::display() {
    int i = 0;
    int si = 0;
    name = current->getName();    

    lcd.clear();  // just for now, use LCD with 2 lines x 16 chars
    lcd.setCursor(0, 0);  // regel 1
    if (current->hasParent()) {
        lcd.print(current->getParentName());
    }
    else {
        lcd.print("Hoofdmenu");
    }
    
    lcd.setCursor(0, 1);  // regel 2
    lcd.print(current->getParent() ? ITEM_LINKS : ITEM_PARENTLESS);
    if (current->getPreviousSibling()) {        
        lcd.print(current->getNextSibling() ? MIDDELSTE_ITEM : LAATSTE_ITEM);
    } else {
        lcd.print(current->getNextSibling() ? EERSTE_ITEM : ENIGE_ITEM);
    }
    lcd.print(current->hasChild() ? ITEM_RECHTS : ITEM_CHILDLESS);
    lcd.print(name);
}
void PWmenu::goUp() {
    if (current->getPreviousSibling()) {
        current = current->getPreviousSibling();
    }
}

void PWmenu::goDown() {
    if (current->getNextSibling()) {
        current = current->getNextSibling();
    }
}

void PWmenu::goSelect() {
    if (current->getChild()) {
        current = current->getChild();
        depth++;
    } else {
        used = true;
    }
}

void PWmenu::goBack() {
    if (current->getParent()) {
        current = current->getParent();
        depth--;
    } else {
        exited = true;
    }
}
void PWmenu::goRoot() {
    current = root;
    exited = false;
    used = false;
}
bool PWmenu::isUsed() {
    return used;
}
bool PWmenu::isExited() {
    return exited;
}
const char *PWmenu::getMenu() {
    return current->getName();
}
const int PWmenu::getId() {
    return current->getId();
}
/*========*/
