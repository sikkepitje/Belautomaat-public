/*
 * voorbeeld-BHW-Belautomaat
 * 20181125 Paul Wiegmans
 * met Belautomaat hardware
 */

/* ====== INCLUDES ====== */
#include <PWmenu_LCD.h>
#include <Belautomaat_lib.h>
#include <belrooster.h>

//=============== MENU SETUP ==============================

// unieke menu id's
#define M_TEST        1
#define M_RELAIS1     2
#define M_ROOSTER     10
#define M_R_TOON      11
#define M_R_NIEUW     12
#define M_R_WIJZIG    13
#define M_R_WIS       14
#define M_SIGNAAL     20
#define M_S_TOON      21
#define M_S_NIEUW     22
#define M_S_WIJZIG    23
#define M_S_WIS       24
#define M_INSTELLING  30
#define M_I_DATUM     31
#define M_I_TIJD      32
#define M_I_ZOMERTIJD 33
#define M_INFORMATIE  40
#define M_INF_VERSIE  41

MenuItem miTest(        M_TEST,       "Test 1");
MenuItem miRelais1(     M_RELAIS1,    "Relais 1");
MenuItem miRooster(     M_ROOSTER,    "Rooster");
MenuItem miRNieuw(      M_R_NIEUW,    "Nieuw");
MenuItem miRToon(       M_R_TOON,     "Toon");
MenuItem miRWijzig(     M_R_WIJZIG,   "Wijzig");
MenuItem miRWis(        M_R_WIS,      "Wis");
MenuItem miSignaal(     M_R_WIS,      "Signaal");
MenuItem miSNieuw(      M_S_NIEUW,    "Nieuw");
MenuItem miSToon(       M_S_TOON,     "Toon");
MenuItem miSWijzig(     M_S_WIJZIG,   "Wijzig");
MenuItem miSWis(        M_S_WIS,      "Wis");
MenuItem miInstelling(  M_INSTELLING, "Instelling");
MenuItem miInsDatum(    M_I_DATUM,    "Datum");
MenuItem miInsTijd(     M_I_TIJD,     "Tijd");
MenuItem miInsZomertijd(M_I_ZOMERTIJD,"Zomertijd");
MenuItem miInformatie(  M_INFORMATIE, "Informatie");
MenuItem miInfVersie(   M_INF_VERSIE, "Versie");
PWmenu menu(miRooster);

void menuinit() {
  miRooster.addSibling(miTest);
  miRooster.addSibling(miRelais1);
  miRooster.addSibling(miSignaal);
  miRooster.addSibling(miInstelling);
  miRooster.addSibling(miInformatie);
  miRooster.addChild(miRToon);
  miRooster.addChild(miRNieuw);
  miRooster.addChild(miRWijzig);
  miRooster.addChild(miRWis);  
  miSignaal.addChild(miSToon);
  miSignaal.addChild(miSNieuw);
  miSignaal.addChild(miSWijzig);
  miSignaal.addChild(miSWis);  
  miInstelling.addChild(miInsDatum);
  miInstelling.addChild(miInsTijd);
  miInstelling.addChild(miInsZomertijd);  
  miInformatie.addChild(miInfVersie);
}

//-------------------------------------------------------
enum mainState_t { msIDLE, msMENU, msTEST, msVERSION, meSETTIME };
mainState_t mainState;

void handleMenu(char c) {
  if (c == UP) 
    menu.goUp();
  else if (c == DOWN)
    menu.goDown();
  else if (c == BACK)
    menu.goBack();
  else if (c == SELECT)
    menu.goSelect();
  menu.display();
}

#define IDLEPERIOD_MS 50

void planner_func(void) { 
  static unsigned long time_to_wake;
  if (millis() < time_to_wake) {
    return;
  }
  time_to_wake = millis() + IDLEPERIOD_MS; 

  // toon klok
  get_time();
  //static bool dots = false;
  //dots = not dots;
  bool dots = (((millis() / 250) % 4) == 0); 
  sevenseg_out(rtchour, rtcminute, dots);

  int kanaal = r.update(rtchour, rtcminute, rtcweekday);
  if (kanaal >= 0) {
    schakelKanaal(kanaal, 1); 
  }
}
/* -------- relais board -------- */
// pinnen van relaisboard
#define RELAIS1   2
#define RELAIS2   3
#define RELAIS3   4
#define RELAIS4   5

void schakelKanaal(int kanaal, bool onoff) {
  int pinval = onoff? LOW : HIGH;
  switch (kanaal) {
    case 0: digitalWrite(RELAIS1, pinval); break;
    case 1: digitalWrite(RELAIS2, pinval); break;
    case 2: digitalWrite(RELAIS3, pinval); break;
    case 3: digitalWrite(RELAIS4, pinval); break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Start ----------------------------"));
  setup_belautomaat_lib();
  belautomaat_idler = planner_func;
  menu.setViewSize(2, 16);
  menuinit();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Voorbeeld PWmenu");
  pinMode(RELAIS1, OUTPUT); digitalWrite(RELAIS1, HIGH); 
  pinMode(RELAIS2, OUTPUT); digitalWrite(RELAIS2, HIGH); 
  pinMode(RELAIS3, OUTPUT); digitalWrite(RELAIS3, HIGH); 
  pinMode(RELAIS4, OUTPUT); digitalWrite(RELAIS4, HIGH); 
}

void bedienRelais1() {
  while (true) {
    switch (getKey()) {
      case OK:
      case BACK:
        return;
      case UP:
        schakelKanaal(0, true);
        break;
      case DOWN:
        schakelKanaal(0, false);
        break;
    }    
  }
}
void toon_menuresultaat() {
  lcd.clear();
  lcd.setCursor(0,0);
  if (menu.isExited()) {
    lcd.print("Uit menu");    
  } else if (menu.isUsed()) {
    lcd.print("Gekozen:");
    lcd.print(menu.getMenu());
    lcd.setCursor(0,1);
    lcd.print(menu.getId());
  }
}

/* valueEditor_2d  laat gebruiker een waarde bewerken. Waarde wordt getoond op LCD.
 *  Bladeren veroorzaakt geen waardeomslag.
 *  Laatste cijfer invoeren bevestigt niet automatisch.
 *  Gebruiker moet altijd bevestigen met OK.
 */
void valueEditor_2d(int xoffset, int yoffset, int &result, int &value, int limiet) {
  int curpos;
  char c;
  curpos = 0;
  
  while (0 <= curpos && curpos <= 1) {
    // toon
    lcd.setCursor(xoffset, yoffset);
    print_2digits(value);
    lcd.setCursor(xoffset + curpos, yoffset);
    // wacht op invoer
    lcd.blink();
    lcd.cursor();
    c = getKey();
    lcd.noCursor();
    lcd.noBlink();
    // verwerk invoer
    switch(c) {
      case SELECT:
        curpos++;
        break;
      case BACK: 
        curpos--; 
        break;
      case UP: //  is ++
        switch(curpos) {
          case 0: if (value + 10 < limiet) value += 10; else value = 0; break;
          case 1: if (value + 1 < limiet) value++; else value = 0; break;
        }
        break;
      case DOWN: //  is --
        switch(curpos) {
          case 0: if (value - 10 >= 0) value -= 10; else value = limiet-1; break;
          case 1: if (value - 1 >= 0) value--; else value = limiet-1; break;
        }
        break;
      default: 
      if (is_number_char(c)) {
        c = (int)(c - '0');
        switch (curpos) {
          case 0: 
            if ((value % 10) + 10 * c < limiet) {
              value = (value % 10) + 10 * c; curpos++;}
            else if (10 * c < limiet) {
              value = 10 * c; curpos++;}  
            break;
          case 1: if (10* (value / 10) + c < limiet) value = 10 * (value / 10) + c; break;
        }
      } 
    }
  }
  if (curpos > 1) result = true; else result = false;
}

void testEditor() {
  int hour, result;
  hour = 4;
  lcd.clear();
  lcd.print("Wijzig uur:");
  valueEditor_2d( 13, 1, result, hour, 24);
  if (result) {
    lcd.clear();
    lcd.print("OK ");
    lcd.print(hour);
  } else {
    lcd.home();
    lcd.print("Afgebroken");
  }
  
}

void editCurrentTime() {
  DateTime dt = rtc.now();
  int hour = dt.hour(), minute = dt.minute();
  bool result = timeEditor(hour, minute);
  if (result) {
    DateTime adjustedtime(dt.year(), dt.month(), dt.day(), hour, minute, dt.second());
    rtc.adjust(adjustedtime);    
  }
}

void loop () {
  char c = keypad.getKey();
  switch (mainState) {
    case msIDLE:
      if (c) {
        menu.goRoot();
        mainState = msMENU;
        handleMenu('0');
      }
      break;
    case msMENU:
      if (c) {
        handleMenu(c);
      }
      if (menu.isExited()) {
        mainState = msIDLE;
        toon_menuresultaat();
      }
      if (menu.isUsed()) {
        mainState = msIDLE;
        toon_menuresultaat();
        switch (menu.getId()) {
          case M_I_TIJD:
            editCurrentTime();
            mainState = msIDLE;
            break;
          case M_TEST:
            testEditor();
            mainState = msIDLE;
            break;
          case M_INF_VERSIE:
            mainState = msVERSION;
            break;
          case M_RELAIS1:
            bedienRelais1();
            mainState = msIDLE;
            break;
        }
      }
      break;
    case msVERSION:
      lcd.clear();
      lcd.print("Versie 1.0");
      lcd.setCursor(0,1);
      lcd.print("2018-11-20");
      mainState = msIDLE;
      break;
  }
  belautomaat_idler();
  delay(1);
}
