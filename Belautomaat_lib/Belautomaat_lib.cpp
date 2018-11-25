/*
	Belautomaat-lib.cpp 
  20181111
	20181125
	Paul Wiegmans <p.wiegmans@bonhoeffer.nl>

  Bibliotheek voor de Belautomaat ondersteunende functies.
  
	Vermeld in je code:
	#include <Belautomaat_lib.h>
*/

#include "Arduino.h"
#include "Belautomaat_lib.h"

/* ====== VARIABLES  ====== */
Rooster r;

/* ====== FUNCTIONS ====== */

t_voidfunc belautomaat_idler;  // definitie van algemene idle-functie, deze alleen toekennen en aanroepen

boolean once_per_second() {
  static int lastsecond;
  if (rtcsecond != lastsecond) {
    lastsecond = rtcsecond;
    return true;
  }
  return false;  
}

// dummy idle functie 
void dummy_idle() {
}

// blokkerende functie getKey, roept idle-functie aan
char getKey() {
  char c;
  while (!(c = keypad.getKey())) {
    delay(1);
    belautomaat_idler();
  }
  return c;
}

/*---------------------------- Belautomaat Algemeen ----------------------------*/
void setup_belautomaat_lib() {
  setup_belautomaat_driver();
  belautomaat_idler = dummy_idle;  // offer a safe-to-call function to fill the pointer
}

//--- TimeEdit ---
// is een editor voor een tijd met uur en minuut, gebruikmakend van lcd en keypad.

void print_2digits(int n) {
  lcd.print( (char)('0'+(n/10)));
  lcd.print( (char)('0'+(n%10)));
}

bool is_number_char(char c) {
  return (c>='0') && (c <= '9');
}
/* timeedit laat gebruiker een tijd in uren en minuten invoeren.
  retourneert true wanneer gebruiker heeft bevestigd.
  retourneert false wanneer gebruiker heeft afgebroken.
  */
bool timeEditor(int &hour, int &minute) {
  char c;
  int curpos, lastpos;
  int basex = 11;
  int teposx[] = {11, 12, 14, 15};
  const char caret = 94;

  lcd.clear();
  curpos = 0; lastpos=0;
  
  while (true) {
    lcd.home();
    lcd.print("WijzigTijd");
    lcd.setCursor(teposx[0], 0);
    print_2digits(hour);
    lcd.print(":");
    print_2digits(minute);
    if (curpos != lastpos) {
      lcd.setCursor(teposx[lastpos], 1);
      lcd.print(" ");
      lastpos = curpos;
    }
    // regel 1
    lcd.setCursor(teposx[curpos], 1);
    lcd.print("^");

    while (!(c = keypad.getKey())) {
      delay(1);
      belautomaat_idler();
    }
    
    switch(c) {
      case SELECT: curpos++; break;
      case BACK: curpos--; break;
      case UP: //  is ++
        switch(curpos) {
          case 0: if (hour <= 13) {hour += 10; } break;
          case 1: hour++; if (hour >= 24) hour -= 24; break;
          case 2: minute += 10; if (minute >= 60) minute -= 60; break;
          case 3: minute++; if (minute >= 60) minute -= 60; break;
        }
        break;
      case DOWN: //  is --
        switch(curpos) {
          case 0: if (hour >= 10) { hour -= 10; } break;
          case 1: hour--; if (hour < 0) hour += 24; break;
          case 2: minute -= 10; if (minute < 0) minute += 60; break;
          case 3: minute--; if (minute < 0) minute += 60; break;
        }
        break;
      default: 
      if (is_number_char(c)) {
        c = (int)(c - '0');
        switch (curpos) {
          case 0: if (c < 2) { hour = 10 * c; curpos++; break; }
          case 1: if (!(hour >= 20 && c > 3)) { hour += c; curpos++; break; }
          case 2: if (c <= 5) { minute = 10 * c; curpos++; break; }
          case 3: minute += c; curpos++; break;
        }

      } 
    }
    if (curpos > 3) {
      return true;
    } 
    if (curpos < 0) {
      return false;
    } 
  }
}
