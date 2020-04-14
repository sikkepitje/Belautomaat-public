/*
	Belautomaat_lib.cpp 
	Paul Wiegmans <p.wiegmans@svok.nl>

  Bibliotheek voor de Belautomaat ondersteunende functies.
  
  Historie
  20181111
	20200414

	Vermeld in je code:
	#include <Belautomaat_lib.h>

  Vermeld in je setup-functie:
  setup_belautomaat_lib();
  belautomaat_idler = idle_func;
 
*/

#include "Belautomaat_lib.h"

/* ====== CONSTANTS  ====== */
char numalfamap[][8] = {
  "0 +-*/?","1 !@#$%","2ABCabc","3DEFdef",
  "4GHIghi","5JKLjkl","6MNOmno","7PRSprs",
  "8TUVtuv","9WXYwxy","*ZzQq"  ,"#^&()"};

/* ====== VARIABLES  ====== */
Rooster r;
Timer timer;
unsigned long time_to_exit_menu;
unsigned long time_to_task;
int signaallengte;
int mp3volume;

mainState_t mainState;

/*---------------------------- timer ----------------------------*/
/* Herinnering: class Timer methods
 *  int8_t every(unsigned long period, void (*callback)(void*), void* context);
 *  int8_t every(unsigned long period, void (*callback)(void*), int repeatCount, void* context);
 *  int8_t after(unsigned long duration, void (*callback)(void*), void* context);
 *  int8_t oscillate(uint8_t pin, unsigned long period, uint8_t startingValue);
 *  int8_t oscillate(uint8_t pin, unsigned long period, uint8_t startingValue, int repeatCount);
 *  int8_t stop(int8_t id);
 *  void update(void);
 */

/* ====== FUNCTIONS ====== */

t_voidfunc belautomaat_idler;  // definitie van algemene idle-functie, deze alleen toekennen en aanroepen

// https://learn.adafruit.com/memories-of-an-arduino/measuring-free-memory

boolean once_per_second() {
  static int lastsecond;
  if (rtcsecond != lastsecond) {
    lastsecond = rtcsecond;
    return true;
  }
  return false;  
}
// dummy idle functie, wordt aangeroepen wanneer belautomaat niets doet.
void dummy_idle() {
  mp3DiscardStatus();
  timer.update();  
}
// LCD backlight aan en reset menu timeout timer
void light_lcd() {
  lcd.backlight();    
  time_to_exit_menu = millis() + MENU_TIMEOUT_MS;
}
// blokkerende functie getKey, roept idle-functie aan
char getkb() {
  char c;
  while (!(c = getKey())) {
    delay(1);
    belautomaat_idler();
  }
  light_lcd();
  return c;
}

char waitforkey() {
  // toon '>' op laatste positie van LCD en getKey()
  lcd.setCursor(15,1);
  lcd.print('>');
  lcd.setCursor(15,1);
  lcd.blink();
  lcd.cursor();
  char c = getkb();
  lcd.noCursor();
  lcd.noBlink();
  lcd.clear();
  return c;
}

// blokkerende functie getKey, roept idle-functie aan
// timeout bevat timeoutwaarde in milliseconden.
char getkb_timeout(unsigned long timeout) {
  char c;
  if (timeout <= 0) return getkb();
  timeout = millis() + timeout;
  while ((millis() < timeout) && !(c = getKey())) {
    delay(1);
    belautomaat_idler();
  }
  light_lcd();
  return c; // wanneer timeout plaatsvond, retourneer false, anders key. 
}

bool alfainputdone(char c) {
  return (c == SELECT || c == BACK);
}

int alfazoekreeks(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  else if (c == '*') return 10;
  else if (c == '#') return 11; 
  else return -1;
}

// letters invoeren op numkeypad zoals op telefoon:
// druk en kies een letters uit de reeks, 
// timeout om te bevestigen
char getalfakey() {
  char c = getkb();
  int keyindex = 0;
  int vorigereeks, reeks;
  if (alfainputdone(c)) {
    return c;
  }
  reeks = alfazoekreeks(c);
  vorigereeks = reeks;
  while (true) {
    // uitvoer
    lcd.setCursor(8, 0);
    lcd.print(numalfamap[reeks]);
    lcd.setCursor(8 + keyindex, 0);
    // invoer
    lcd.cursor();
    lcd.blink();
    c = getkb_timeout(ALFAKEY_TIMEOUT);
    lcd.noBlink();
    lcd.noCursor();
    lcd.setCursor(8, 0);
    lcd.print("      ");
    // verwerking
    // timeout heeft plaatsgevonden
    if (!c) return numalfamap[reeks][keyindex];
    reeks = alfazoekreeks(c);
    // andere toets gedrukt
    if (vorigereeks != reeks) return numalfamap[vorigereeks][keyindex];
    keyindex++;
    if (keyindex >= strlen(numalfamap[reeks])) keyindex = 0;
  }
}

/*-=-----------------------------------------------------*/
void sound_error() {
  mp3play(55);   // error: ongeldige toetsinvoer
}
void sound_enter_success() {
  mp3play(51);    // enter success
}
void sound_enter_fail() {
  mp3play(52);   // enter fail
}
void sound_hour_chime() {
  mp3play(8);   // uursignaal
}
void sound_end_reached() {
  mp3play(59);  // stop , einde bereikt
}
void print_datum(int jaar, int maand, int dag, int weekdag) {
  lcd.print(weekdagenKort[weekdag]);
  lcd.print(' ');
  print_2digit(dag);
  lcd.print('-');
  print_2digit(maand);
  lcd.print('-');
  lcd.print(jaar);
}

void print_tijd(int uur, int minuut, int seconde) {
  print_2digit(uur);
  lcd.print(':');
  print_2digit(minuut);
  if (seconde >= 0) {
    lcd.print(':');
    print_2digit(seconde);
  }
}

void print_dowbitmap( int dayOfWeek)
{
    // print dag van de week voor elke bit in bitveld.
    // buffer moet minimaal 15 tekens lang zijn.
    const char dowNames[] PROGMEM = {"MDWDVZZ"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++)
    {
        if (dayOfWeek & masker) {
            lcd.print(dowNames[index]);
        } else {
            lcd.print((char)0xB0);
        }
        index++;
        masker >>= 1;
    }
}

void printSignaaltijd(int index)
{
  Signaal_t sig;
  r.signaalLezen(index, sig);
  print_tijd(sig.hour, sig.minute, -1);
  lcd.print(' ')  ;
  print_dowbitmap(sig.dayOfWeek);
}

/*--------------------------- relais board ---------------------------*/
void switch1Off() { relaisOff(0); }
void switch2Off() { relaisOff(1); }
void switch3Off() { relaisOff(2); }
void switch4Off() { relaisOff(3); }

/* interessante geluiden: 24,31,35,41,44,49,48,64, 57 
8: koekoek
10: eierwekker
11: powerup
24: mario got a star kort
31: metal
35: dingdong deurbel 
38: honkhonk 
39: saxophone
40: T65 x3
44: mario got a star
48: lang intro 
60: belautomaat startup
64: lang intro bescheiden
57: UI 3 rising 
58: KEN schoolbel violen
*/
void triggerUitgang(int uitgang) {
  unsigned long periode = 1000L * (long)(signaallengte);
  switch (uitgang) {
    case 0: relaisOn(0); timer.after(periode, switch1Off); break;
    case 1: relaisOn(1); timer.after(periode, switch2Off); break;
    case 2: relaisOn(2); timer.after(periode, switch3Off); break;
    case 3: relaisOn(3); timer.after(periode, switch4Off); break;
    case 4: mp3play(44); break;   // Mario got a star
    case 5: mp3play(24); break;   // kort mario star
    case 6: mp3play(11); break;   // powerup
    case 7: mp3play(48); break;   // lang intro
    case 8: mp3play(8); break;   // koekoek
    case 9: mp3play(40); break;   // T65 x3

  }
}

/*-------------------------- Editfuncties ---------------------------*/

void print_2digit(int n) {
  lcd.print( (char)('0' + ((n/10) % 10)));
  lcd.print( (char)('0' + (n%10)));
}

bool is_number_char(char c) {
  return (c>='0') && (c <= '9');
}

/******************  timeEditor *******************/
/* edit_time laat gebruiker een tijd in uren en minuten invoeren.
 *  op een vaste positie op LCD. Cursor verschijnt op 2e regel.
 *  retourneert true wanneer gebruiker heeft bevestigd.
 *  retourneert false wanneer gebruiker heeft afgebroken.
  */
bool UI_edit_time(int posx, int posy, int &hour, int &minute) {
  char c;
  int curpos = 0, indicatory = 1;
  int offx[] = {0, 1, 3, 4};
  const char caret = 94;
  
  while (true) {
    lcd.setCursor(posx + offx[0], posy);
    print_2digit(hour);
    lcd.print(':');
    print_2digit(minute);
    
    lcd.setCursor(posx + offx[curpos], posy);
    lcd.cursor();
    lcd.blink();
    c = getkb();
    lcd.noBlink();
    lcd.noCursor();
    
    switch(c) {
      case SELECT: curpos++; break;
      case BACK: curpos--; break;
      case UP: //  is ++
        switch(curpos) {
          case 0: if (hour + 10 < 24) hour += 10; else hour = 0; break;
          case 1: if (hour + 1 < 24) hour++; else hour = 0; break;
          case 2: if (minute + 10 < 60) minute += 10; else minute = 0; break;
          case 3: if (minute + 1 < 60) minute++; else minute = 0; break;
        }
        break;
      case DOWN: //  is --
        switch(curpos) {
          case 0: 
            if (hour - 10 >= 0) hour -= 10; 
            else hour = 24-1; 
            break;
          case 1: 
            if (hour - 1 >= 0) hour--; 
            else hour = 24-1; 
            break;
          case 2: 
            if (minute - 10 >= 0) minute -= 10; 
            else minute = 60-1; 
            break;
          case 3: 
            if (minute - 1 >= 0) minute--; 
            else minute = 60-1; 
            break;
        }
        break;
      default: 
      if (is_number_char(c)) {
        c = (int)(c - '0');
        switch (curpos) {
          case 0: 
            if ((hour % 10) + 10 * c < 24) { // wanneer er 03 staat en je voert 2 voor tiental in -> 23
              hour = (hour % 10) + 10 * c; 
              curpos++;
            } else if (10 * c < 24) { // wanneer er 04 staat en je voert 2 voor tiental in -> 20
              hour = 23; 
              curpos++;
            } else {              
              sound_error();
            }  
            break;
          case 1: 
            if (10 * (hour / 10) + c < 24) {
              hour = 10 * (hour / 10) + c; 
              curpos++;
            } else {              
              sound_error();
            }
            break;
          case 2: 
            if ((minute % 10) + 10 * c < 60) {
              minute = (minute % 10) + 10 * c; 
              curpos++;
            } else {              
              sound_error();
            }  
            break;
          case 3: 
            if (10 * (minute / 10) + c < 60) {
              minute = 10 * (minute / 10) + c; 
              curpos++;
            } else {              
              sound_error();
            }
        }
      } 
    }
    if (curpos > 3) {
      sound_enter_success();
      return true;
    } 
    if (curpos < 0) {
      sound_enter_fail();
      return false;
    } 
  }
}

/* edit_2digits : bewerk een waarde interactief op LCD.
 *  Bladeren veroorzaakt waardeomslag.
 *  Laatste cijfer invoeren sluit editor met OK, alleen indien invoer geldig was.
 *  retourneert true bij bevestigen, false bij afbreken.
 */
bool UI_edit_2digit(int xoffset, int yoffset, int &value, int limiet) {
  int curpos;
  char c;
  curpos = 0;
  
  while (0 <= curpos && curpos <= 1) {
    // toon
    lcd.setCursor(xoffset, yoffset);
    print_2digit(value);
    lcd.setCursor(xoffset + curpos, yoffset);
    // wacht op invoer
    lcd.blink();
    lcd.cursor();
    c = getkb();
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
          case 0: 
            if (value - 10 >= 0) value -= 10; 
            else value = limiet-1; 
            break;
          case 1: 
            if (value - 1 >= 0) value--; 
            else value = limiet-1; 
            break;
        }
        break;
      default: 
      if (is_number_char(c)) {
        c = (int)(c - '0');
        switch (curpos) {
          case 0: 
            if ((value % 10) + 10 * c < limiet) {
              value = (value % 10) + 10 * c; 
              curpos++;
            } else if (10 * c < limiet) {  // wanneer er 17 staat en je voert 2 voor tiental in -> 20
              value = 10 * c; 
              curpos++;
            } else {              
              sound_error();
            }  
            break;
          case 1: 
            if (10* (value / 10) + c < limiet) {
              value = 10 * (value / 10) + c; 
              curpos++;
            } else {              
              sound_error();
            }
        }
      } 
    }
  }
  if (curpos > 1) {
    sound_enter_success();
    return true;
  }
  else {
    sound_enter_fail();
    return false;
  }
}

/* edit_1digit : bewerk een waarde interactief op LCD.
 *  Bladeren veroorzaakt waardeomslag.
 *  Laatste cijfer invoeren sluit editor met OK, alleen indien invoer geldig was.
 *  retourneert true bij bevestigen, false bij afbreken.
 */
bool UI_edit_1digit(int xoffset, int yoffset, int &value, int limiet) {
  int curpos;
  char c;
  curpos = 0;
  
  while (0 == curpos) {
    // toon
    lcd.setCursor(xoffset, yoffset);
    lcd.print(value % 10);
    lcd.setCursor(xoffset + curpos, yoffset);
    // wacht op invoer
    lcd.blink();
    lcd.cursor();
    c = getkb();
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
        if (value + 1 < limiet) value++; 
        else value = 0; 
        break;
      case DOWN: //  is --
        if (value - 1 >= 0) value--; 
        else value = limiet - 1;
        break;
      default: 
      if (is_number_char(c)) {
        c = (int)(c - '0');
        if (c < limiet) {
          value = c; 
          curpos++;
        } else {              
          sound_error();
        }
      } 
    }
  }
  if (curpos > 0) {
    sound_enter_success();
    return true;
  }
  else {
    sound_enter_fail();
    return false;
  }
}

void UI_handbedienRelais() {
  int uitgang = 0;
  bool result;
  int MAXROOSTERS = 4;
  while (true) {
    lcd.clear();
    lcd.print(F("Kies relais:"));
    bool result = UI_edit_1digit(0, 15, uitgang, 10);
    lcd.clear();
    if (!result) return;
    bool done = false;
    while (!done) {
      lcd.setCursor(0, 0);
      lcd.print(F("Bedien relais "));
      lcd.print(uitgang);
      lcd.setCursor(0, 1);
      char c = getkb();
      switch (c) {
        case OK:
          lcd.print(F("Trigger "));
          triggerUitgang(uitgang);
          break;
        case BACK:
          done = true;
          break;
        case UP:
          lcd.print(F("Aan    "));
          if (uitgang < 4) relaisOn(uitgang);
          break;
        case DOWN:
          lcd.print(F("Uit    "));
          if (uitgang < 4) relaisOff(uitgang);
          break;
      }    
    }
  }
  lcd.clear();
}

/*---------------------------- UI functies ----------------------------*/
void LCD_Printweekdag(int weekdag)
{
    // print dag van de week met 7 letters, één voor elke bit in bitveld.
    const char dowNames[] = {"MDWDVZZ"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++) {
        if (weekdag & masker) {
            lcd.print(dowNames[index]);
        } else {
            lcd.print((char)0xA5);
        }
        index++;
        masker >>= 1;
    }
}

/* ui_edit_weekdag : bewerk een weekdag-waarde interactief op LCD.
 *  Intikken van een cijfer doet overeenkomende dag omslag. 1=ma, 2=di, enz
 *  '#' gaat naar volgende positie of bevestigt op laatste positie, 
 *  '*' gaat positie terug, of breekt af op eerste positie.
 *  UP en DOWN verhoogt/verlaagt, 
 *  SELECT beeindigt invoer onmiddellijk met bevestiging, retourneert true.
 *  BACK beeindigt invoer onmiddellijk met afbreking, retourneert false.
 */
bool UI_edit_weekdag(int xoffset, int yoffset, int &weekdag) 
{
  int curpos = 0;
  while (0 <= curpos and curpos < 7) {
    // toon weekdag
    lcd.setCursor(xoffset, yoffset);
    LCD_Printweekdag(weekdag);
    lcd.setCursor(xoffset + curpos, yoffset);
    // wacht op invoer
    lcd.blink();
    lcd.cursor();
    char c = getkb();
    lcd.noCursor();
    lcd.noBlink();
    // verwerk invoer
    switch(c) {
      case SELECT:    // stop en bevestig
        curpos = 7;   
        break;
      case BACK:      // stop en annuleren
        curpos = -1;  
        break;
      case '#':       // cursor naar rechts
        curpos++;
        break;
      case '*':       // cursor naar links
        curpos--; 
        break;
      case UP:        // dag omschakelen
      case DOWN:  
        weekdag ^= 1 << (6 - curpos);
        break;
      case '8':       // werkdagen omschakelen
        weekdag ^= B1111100;
        break;
      case '9':       // weekend omschakelen
        weekdag ^= B0000011;
        break;
      case '0':       // schakel alle dagen aan of uit
        if (weekdag & B1000000) weekdag = 0; else weekdag = B1111111;
        break;
      default: 
        if (c >= '1' and c <= '7') {
          c = (int)(c - '0');
          weekdag = weekdag ^ ( 1 << (7 - c)); // dag 1-7 omschakelen
        } 
    } // switch
  } // while
  if (curpos >= 7) {
    sound_enter_success();
    return true;
  }
  else {
    sound_enter_fail();
    return false;
  }
}

void UI_setVolume() 
{
  lcd.clear();
  int volume = get_mp3volume();
  lcd.print(F("Volume (0-31)?"));
  bool result = UI_edit_2digit( 13, 1, volume, 32);
  lcd.clear();
  if (result) {
    lcd.print(F("Volume is nu "));
    lcd.print(volume);
    set_mp3volume(volume);
    waitforkey();
  }  
  lcd.clear();
}

void UI_bladerGeluiden() 
{
  static int mp3track = 34;
  bool done = false;
  while (!done) {
    lcd.clear();
    lcd.print(F("Track:"));
    lcd.println(mp3track);
    mp3play(mp3track);
    char c = getkb();
    switch (c) {
      case DOWN:
        if (mp3track > 1) { mp3track--; }
        break;
      case UP:
        if (mp3track < mp3count) { mp3track++; }
        break;
      case BACK:
        done = true;
        break;
      default:
        mp3play(mp3track);
        break;
    }
  lcd.clear();
  }
}

void UI_toonVersie() 
{
  lcd.clear();
  lcd.print(F("Belautomaat"));
  waitforkey();

  lcd.clear();
  lcd.print(F("Auteur:"));
  lcd.setCursor(0,1);
  lcd.print(F("Paul Wiegmans"));
  waitforkey();

  lcd.clear();
  lcd.print(F("compile date"));
  lcd.setCursor(0,1);
  lcd.print(__DATE__);
  waitforkey();

  lcd.clear();
  lcd.print(F("compile time"));
  lcd.setCursor(0,1);
  lcd.print(__TIME__);
  waitforkey();
  lcd.clear();
}

void UI_toondatumtijd() 
{
  unsigned long update_interval = millis();
  char c;
  lcd.clear();
  while (!(c = getKey())) {
    lcd.setCursor(0, 0);
    print_tijd(rtchour, rtcminute, rtcsecond);
    lcd.setCursor(0, 1);
    print_datum(rtcyear, rtcmonth, rtcday, rtcweekday);
    
    while (millis() < update_interval) {
      belautomaat_idler();        
    }
    update_interval = millis() + UPDATE_INTERVAL_MS;
    if (c == SELECT || c == BACK) return;
  }
}

void UI_setTime() 
{
  DateTime dt = rtc.now();
  int hour = dt.hour(), minute = dt.minute();
  lcd.clear();
  lcd.print(F("Nwe Tijd:"));
  if (UI_edit_time(11, 0, hour, minute)) {
    lcd.clear();
    lcd.print(F("Tijd instellen"));
    lcd.setCursor(0,1);
    print_2digit(hour);
    lcd.print(':');
    print_2digit(minute);
    lcd.print(F(":00 OK?"));
    if (SELECT == waitforkey()) {     // bevestig tijdinstelling
      DateTime adjustedtime(dt.year(), dt.month(), dt.day(), hour, minute, 0);
      rtc.adjust(adjustedtime);    
      r.findNextIndex(hour, minute);
    }
  } 
  lcd.clear();
}

void UI_setDate() 
{
  DateTime dt = rtc.now();
  int dag = dt.day();
  int maand = dt.month();
  int jaar = dt.year();
  bool result;
  lcd.clear();
  lcd.print(F("Nieuwe datum"));
  lcd.setCursor(0,1);
  lcd.print(F("Jaar: 20"));
  jaar = jaar % 100;
  // bewerk 2 cijferig jaar 
  if (UI_edit_2digit(8, 1, jaar, 100)) {
    jaar = 2000 + jaar;

    lcd.clear();
    lcd.print(F("Nieuwe datum"));
    lcd.setCursor(0,1);
    lcd.print(F("Maand:"));
    // bewerk maand 0..12
    if (UI_edit_2digit(8, 1, maand, 13)) {
      if (maand < 1) maand = 1;
      
      lcd.clear();
      lcd.print(F("Nieuwe datum"));
      lcd.setCursor(0,1);
      lcd.print(F("Dag:"));
      // bewerk dag 0..32
      if ( UI_edit_2digit(8, 1, dag, 32)) {
        if (dag < 1) dag = 1; 
        
        lcd.clear();
        lcd.print(F("Nieuwe datum"));
        lcd.setCursor(0,1);
        lcd.print(dag);
        lcd.print('-');
        lcd.print(maand);
        lcd.print('-');
        lcd.print(jaar);
        lcd.print(F(" OK?"));
        if (waitforkey() == SELECT) {     // bevestig tijdinstelling
          DateTime dt = rtc.now(); 
          DateTime adjusted(jaar, maand, dag, dt.hour(), dt.minute(), dt.second());
          rtc.adjust(adjusted);
          r.findNextIndex(dt.minute(), dt.second());
        }
      }  
    } 
  } 
  lcd.clear();
}

void UI_toetsenbordslot() {
  int state = 0;
  char c = ' ';
  bool done = false;
  char ontgrendelcode[5] = "1902";
  mainState = msKEYLOCK;
  while (!done) {    
    lcd.clear();
    lcd.print(F("Toetsenbord"));
    lcd.setCursor(0,1);
    lcd.print(F("op slot "));
    lcd.print(c);  // geeft uitsluitend feedback over laatst ingetikte teken
    c = getkb();    
    if (c == ontgrendelcode[state]) {
      state++;
      if (state > 3) {
        done = true;  // ontgrendeld
        mainState = msIDLE;
        mp3play(60); // play "magical accent"
        return;
      }
    } else {
      state = 0;
    }    
  }
  mainState = msIDLE;
}

/* ---------------- rooster bladermachine -------------- */
/*****************************************************************************/
/*
 * Hoe gaan we bladeren in de database met signalen? Gebruiker kiest eerst 
 * een kanaal. Alle signalen(indexen) in het gekozen kanaal worden in een tabel
 * bewaard.
 * fks = Filtered Kanaal Set ("Kanaalfilter")
 * beter: rfilter = roosterfilter
 * Na elke wijziging in de fks (wissen, toevoegen) wordt de fks-tabel opnieuw gevuld.
 * Gebruiker kan zo snel door deze signalen bladeren. 
 * Vanuit het gekozen signaal kan de gebruiker vanuit een vervolgmenu kiezen:
 * nieuw signaal, bewerk (huidig) signaal, wis (huidig) signaal.
 */

int fkanaal = 0;        // huidig gekozen kanaal
int fks[ROOSTERSIZE];   // filtered set voor dit kanaal
int fksmax;             // aantal signalen in filtered set
int fksindex = 0;
bool fks_needupdate;      // geeft aan of opnieuw filtered kanaalset opbouw nodig is

void vul_fks(int kfilter) {
  int kindex = 0;
  Signaal_t sig;
  for (int t = 0; t < r.aantalSignalen(); t++) {
    r.signaalLezen(t, sig);
    if (sig.channel == kfilter) {
      fks[kindex++] = t;
    }
  }
  fksmax = kindex;
}

#define F_NIEUW   0
#define F_BEWERK  1
#define F_WIS     2
const char* functienamen[3] = {"Nieuw?","Bewerk?","Wis?"};

void UI_PrintRoosterStatus(int status) { 
  switch (status) {
    case ERROR_TIMTAB_FULL:  lcd.print(F("Tabel vol")); break;
    case ERROR_EEPROM_FULL:  lcd.print(F("EEPROM vol")); break;
    case INFO_NONE_ACTIVE:   lcd.print(F("Geen actief")); break;
    case WARN_RECORD_MERGED: lcd.print(F("Samengevoegd")); break;
    default:                 lcd.print(F("Succes")); break;
}}

void zeker_invoeren(int uur, int minuut, int weekdag, int fkanaal) {
  int insresult = r.signaalInvoegen(uur, minuut, weekdag, fkanaal);
  if (insresult < 0) {
    lcd.clear();
    lcd.print(F("Fout bij invoegen"));
    lcd.setCursor(0,1);
    UI_PrintRoosterStatus(insresult);
    sound_enter_fail();
    waitforkey();
  } else {
    sound_enter_success();
  }      

}
void UI_NieuwSignaal() {
  int uur = rtchour, minuut = rtcminute, weekdag = B1111100;
  lcd.clear();
  lcd.print(F("Nieuw signaal"));
  lcd.setCursor(0,1);
  lcd.print(F("Tijd:"));

  if (UI_edit_time(11, 1, uur, minuut)) {
    lcd.clear();
    lcd.print(F("Nieuw signaal"));
    print_tijd(uur, minuut, -1);
    lcd.setCursor(0,1);
    lcd.print(F("Weekdag:"));

    if ( UI_edit_weekdag(8, 1, weekdag)) {
      zeker_invoeren(uur, minuut, weekdag, fkanaal);
    }
  }
  lcd.clear();
}

void UI_BewerkSignaal() {
  Signaal_t sig;
  r.signaalLezen(fks[fksindex], sig);
  int uur = sig.hour; 
  int minuut = sig.minute;
  int weekdag = sig.dayOfWeek;

  lcd.clear();
  lcd.print(F("Bewerk signaal"));
  lcd.setCursor(0, 1);
  lcd.print(F("Tijd:"));

  if (UI_edit_time(11,1,uur, minuut)) {
    lcd.clear();
    lcd.print(F("Bewerk signaal"));
    print_tijd(uur, minuut, -1);
    lcd.setCursor(0,1);
    lcd.print(F("Weekdag:"));

    if (UI_edit_weekdag(8,1, weekdag)) {
      r.signaalWissen(fks[fksindex]);
      zeker_invoeren(uur, minuut, weekdag, fkanaal);
    }
  }
  lcd.clear();
}

void UI_WisSignaal() {
  lcd.clear();
  lcd.print(F("Signaal wissen"));
  lcd.setCursor(0, 1);
  lcd.print(F("Wissen?"));
  if (waitforkey() == SELECT) {
    r.signaalWissen(fks[fksindex]);
    sound_enter_success();    
  }
  lcd.clear();
}

void doeFunctieUI(int functie) {
  switch (functie) {
    case F_NIEUW:
      UI_NieuwSignaal();
      break;
    case F_BEWERK:
      UI_BewerkSignaal();
      break;
    case F_WIS:
      UI_WisSignaal();
      break;             
  }
  fks_needupdate = true;
  r.findNextIndex(rtchour, rtcminute);
}

void UI_signaal_kies() {
  bool innerdone = false;
  int functie = F_NIEUW;
  while (!innerdone) {
    lcd.clear();  
    //lcd.print(r.printRoosterRegel(fks[fksindex]));
    printSignaaltijd(fks[fksindex]);
    lcd.setCursor(0,1);  // toon functie
    lcd.print(functie+1);
    lcd.print(' ');
    if (functie >= 0 && functie <= 2) {
      lcd.print(functienamen[functie]);
    }
    switch (getkb()) {
      case BACK:    innerdone = true; break;
      case SELECT:  doeFunctieUI(functie); innerdone = true; break;
      case UP: 
        if (functie > 0) {functie--;} else {sound_end_reached();} break;
      case DOWN:
        if (functie < 2) {functie++;} else {sound_end_reached();} break;
      case '1':     doeFunctieUI(F_NIEUW); innerdone = true; break;
      case '2':     doeFunctieUI(F_BEWERK); innerdone = true; break;
      case '3':     doeFunctieUI(F_WIS); innerdone = true; break;             
    }
  }
  lcd.clear();
}

/* ---------------- signaallengte -------------- */
/* edit_1digit : bewerk een waarde interactief op LCD.
 *  Waarde is alleen te bewerken met UP en DOWN. Directe invoer is niet mogelijk.
 *  Laatste cijfer invoeren sluit editor met OK, alleen indien invoer geldig was.
 *  retourneert true bij bevestigen, false bij afbreken.
 *  WERK IN UITVOERING
 */
bool UI_edit_number(int xoffset, int yoffset, int &value, int limiet) {
  int curpos = 0;
  char c;
  lcd.blink();
  lcd.cursor();
  while (0 == curpos) {
    // toon
    lcd.setCursor(xoffset, yoffset);
    print_2digit(value);
    lcd.setCursor(xoffset+1, yoffset);
    c = getkb();
    // verwerk invoer
    switch(c) {
      case SELECT:
        curpos++;
        break;
      case BACK: 
        curpos--; 
        break;
      case UP: //  is ++
        if (value + 1 < limiet) value++; 
        else sound_end_reached();
        break;
      case DOWN: //  is --
        if (value - 1 >= 0) value--; 
        else sound_end_reached();
        break;
    }
  }
  lcd.noCursor();
  lcd.noBlink();
  if (curpos > 0) {
    sound_enter_success();
    return true;
  }
  else {
    sound_enter_fail();
    return false;
  }
}

bool UI_edit_naam(char *naam) {
  // naam invoeren, gebruikt telefoontoetsenbord
  // retourneert false bij afbreken, true bij invoer klaar
  bool b;
  int curpos = 0;
  char c;
  bool succes = false, done = false;  
  while (!done) {
    // output
    lcd.clear();
    lcd.print(F("naam?"));
    lcd.setCursor(0,1);
    for (int t = 0; t < MAX_NAAMLEN; t++) {
      if (naam[t] > 128) naam[t] = 32;
      if (naam[t] < 32) naam[t] = 32;
      lcd.print( naam[t] > 0? char(naam[t]): char(0xfe));
    }
    // input
    lcd.setCursor(curpos, 1);
    lcd.blink();
    lcd.cursor();
    c = getalfakey();
    lcd.noCursor();
    lcd.noBlink();
    // process
    switch (c) {
      case SELECT:
        curpos++; 
        if (curpos >= 8) return true;
        break;
      case UP:
        if (naam[curpos] < 127) naam[curpos]++;
        break;
      case DOWN:
        if (naam[curpos] > 0) naam[curpos]--;
        break;
      case BACK:
        curpos--; 
        if (curpos < 0) return false;
        naam[curpos] = ' ';
        break;
      default:
        naam[curpos] = c;
        curpos++;
        if (curpos >= 8) return true;
    }  
  } 
}

/* signaallengte wordt opgeslagen in en uitgelezen uit EEPROM */
void UI_EditSignaallengte() {
  int lengte = get_signaallengte();
  lcd.clear();
  lcd.print(F("Signaallengte is"));
  lcd.setCursor(0, 1);
  lcd.print(lengte);
  waitforkey();
  
  lcd.print(F("Signaal(0-9)?"));
  lcd.setCursor(0, 1);
  lcd.print(F("seconden"));
  bool result = UI_edit_number( 14, 1, lengte, 9);
  lcd.clear();
  if (result) {
    if (lengte < 1) lengte = 1;
    lcd.print(F("Signaallengte is"));
    lcd.setCursor(0,1);
    lcd.print(lengte);
    lcd.print(F(" seconden"));
    set_signaallengte(lengte);
    waitforkey();
  }
}

void UI_edit_roosternaam(int roosternr) {
  char* naam = r.haalnaam(roosternr); 
  // LET OP! Ik misbruik de static char array in Rooster::haalnaam t.b.v deze editorfunctie !

  bool result = UI_edit_naam(naam);
  if (result) {
    r.zetnaam(roosternr, naam);
  }
}

void UI_edit_schakelaar() {
  bool b;
  int curpos = 0;
  int roosternr = 0; 
  char c;
  // /0123456789012345\
  //  1:[nmnmnmnm] AAN
  //  Uitgang: 5
  // cijfer: instellen Uitgang
  // OK : AAN of UIT  
  // UP/DOWN : blader roosternummer
  // #: instellen naam
  while (0 == curpos) {
    lcd.clear();
    lcd.print('R');
    lcd.print(roosternr);
    lcd.print(' ');
    lcd.print('"');
    lcd.print(r.haalnaam(roosternr));
    lcd.print('"');
    
    lcd.setCursor(0,1);
    lcd.print(F("Uitgang "));
    lcd.print(r.haaluitgang(roosternr));
    lcd.print( r.isactiefrooster(roosternr)? F(" AAN"): F(" UIT"));
    
    c = getkb();
    switch (c) {
      case SELECT:  // schakel rooster AAN of UIT
        r.zetactiefrooster(roosternr, not r.isactiefrooster(roosternr)); break;
      case DOWN:    // blader volgend rooster
        if (roosternr < MAXROOSTER-1) roosternr++; break;
      case UP:      // blader vorig rooster
        if (roosternr > 0) roosternr--; break;
      case BACK:    // terug naar menu
        curpos = -1; break;
      case '#':     // bewerk roosternaam
        UI_edit_roosternaam(roosternr); break;
      default: 
        if (c >= '0' && c <= '9') {     // cijfer: kies uitgang
          c = c - '0'; // teken naar nummer
          r.zetuitgang(roosternr, c);
        }
    }  
  }
}


#define KR_ALLE (-1)
#define KR_GEEN (-2)

int UI_kiesRooster(int rost) {
  /*  kies rooster, retourneer :
   *   0..FKS_MAX_ROOSTER: gekozen rooster
   *   -1: toon alle roosters
   *   -2: geen keuze gemaakt / afgebroken
  */
  char c;
  while (true) {
    lcd.clear();
    lcd.print(F("Toon rooster:"));
    lcd.setCursor(0, 1);
    lcd.print(' ');
    if (rost == KR_ALLE) {
      lcd.print(EERSTE_ITEM);
    } else if (rost == FKS_MAX_ROOSTERS) {
      lcd.print(LAATSTE_ITEM);
    } else {
      lcd.print(MIDDELSTE_ITEM);
    }
    lcd.print('R');
    if (rost >= 0) {
        lcd.print(rost);
        lcd.print(' ');
        lcd.print(r.haalnaam(rost));
    } else {
      lcd.print(F("* Alle roosters"));
    }
    lcd.setCursor(15,1);
    lcd.print(ITEM_RECHTS);
    c = getkb();
    switch (c) {
      case UP:
        if (rost > KR_ALLE) { rost--; } else { sound_end_reached(); }
        break;
      case DOWN:
        if (rost < FKS_MAX_ROOSTERS-1) { rost++; } else { sound_end_reached(); }
        break;
      case SELECT:
        return rost;
      case EXIT:
        return KR_GEEN;
      default:
        if (c >= '0' && c <= '9') {     // cijfer: kies uitgang
          return c - '0'; // teken naar nummer
        }
        if (c == '*') return KR_ALLE;
    }
  }
  return rost;
}

void UI_bladerEenRooster(int roosternr) {
  char c;
  fks_needupdate = true;  
  fkanaal = roosternr;
  fksindex = 0;
  while (true) {
    if (fks_needupdate) {
      vul_fks(fkanaal);
      if (fksindex >= fksmax) {
        fksindex = fksmax - 1;
      }
      fks_needupdate = false;
    }
    lcd.clear();
    lcd.print(r.haalnaam(fkanaal));
    lcd.print(F(" ")); 
    lcd.print(fksindex + 1);  // toont index als tellend vanaf 1 
    lcd.print('/'); 
    lcd.print(fksmax); 
    lcd.print(' ');
    lcd.print(' ');
    lcd.setCursor(0,1);
    if (fksindex < fksmax and fksindex >= 0) {
      //lcd.print(r.printRoosterRegel(fks[fksindex]));      
      printSignaaltijd(fks[fksindex]);
    } else {
      lcd.print(F("Geen signalen"));
    }
    c = getkb();
    switch (c) {
      case BACK:
        return;
      case SELECT:
        UI_signaal_kies();
        break;
      case DOWN:  // volgende
        if (fksindex < fksmax-1) { 
          fksindex++; 
        } else { sound_end_reached(); }
        break;
      case UP:    // vorige
        if (fksindex > 0) {
          fksindex--;
        } else { sound_end_reached(); }
        break;
    }
  }
}

void LCD_printDOW_map(int dayOfWeek)
{
    // print dag van de week voor elke bit in bitveld.
    const char dowNames[] PROGMEM = {"MDWDVZZ"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++)
    {
        if (dayOfWeek & masker) {
            lcd.print(dowNames[index]);
        } else {
            lcd.print('_');
        }
        index++;
        masker >>= 1;
    }
}

void UI_bladerAlleRoosters() {
  char c;
  int index = 0;
  Signaal_t sig;
  while (true) {
    lcd.clear();
    lcd.print(F("Alles ")); 
    lcd.print(' '); 
    lcd.print(index + 1);  // toont index als tellend vanaf 1 
    lcd.print('/'); 
    lcd.print(r.aantalSignalen()); 
    lcd.print(' ');
    lcd.setCursor(0,1);
    if (0 <= index and index < r.aantalSignalen()-1) {
      r.signaalLezen(index, sig);
      lcd.print('R');
      lcd.print( (char)('0' + (sig.channel % 10)));
      lcd.print(' ');
      print_tijd(sig.hour, sig.minute, -1);
      lcd.print(' ');
      LCD_printDOW_map(sig.dayOfWeek);
      
    } else {
      lcd.print(F("Geen signalen"));
    }
    c = getkb();
    switch (c) {
      case UP:
        if (index > 0) {index--; }
        else { sound_end_reached(); }
        break;
      case DOWN:
        if (index < r.aantalSignalen() - 1) {index++; }
        else { sound_end_reached(); }
        break;        
      case SELECT: break;
      case EXIT:
        return;
    }
  }
}

void UI_bladerRooster2() {
  //bool result = UI_edit_1digit(0, 15, fkanaal, FKS_MAX_ROOSTERS);
  //if (!result) return;
  int keuze = KR_ALLE;
  while (true) {
    keuze = UI_kiesRooster(keuze);
    switch (keuze) {
      case KR_GEEN:
        return;
      case KR_ALLE:
        UI_bladerAlleRoosters();
        break;
      default:
        UI_bladerEenRooster(keuze);
        break;
    }
  }
}

/* ---------------- UI dummy functie -------------- */
void UI_dummy() {
  lcd.clear();
  lcd.print(F("Dummy functie"));
  waitforkey();
  lcd.clear();
}

/*---------------------------- Belautomaat Algemeen ----------------------------*/
/*
Werken met EEVars :
Uitgangspunt is dat de lokale variabele altijd goede waarde heeft.
In Setup:
1. éénmaal ophalen uit EEPROm
2. sanity check
3. bewaar in non-public lokale variabele.
In Loop:
4. get: lees lokale variabele via getter-functie 
5. set: schrijf naar lokale variabele en naar EEPROM via setter-functie 
*/
int get_mp3volume() {
  return mp3volume;
}

void set_mp3volume(int value) {
  mp3volume = value;
  mp3.volume(mp3volume);
  EEVarStore(EEVAR_MP3VOL, mp3volume);
}

void setup_mp3volume() {
  mp3volume = EEVarLoad(EEVAR_MP3VOL);
  if (mp3volume > 31 || mp3volume < 0) {
    set_mp3volume(12);
  }
}

int get_signaallengte() {
  return signaallengte;
}

void set_signaallengte(int value) {
  EEVarStore(EEVAR_SGNLEN, value);
}

void setup_signaallengte() {
  signaallengte = EEVarLoad(EEVAR_SGNLEN);
  if (signaallengte > 9 && signaallengte < 1) {
    set_signaallengte(4);
  }
}

void setup_belautomaat_lib() {
  DPRINTLN(F("Init belautomaat lib"));
  setup_belautomaat_driver();
  setup_mp3volume();
  setup_signaallengte();
  belautomaat_idler = dummy_idle;  // biedt veilige functie als plaatshouder.
  mp3play(60); // play "magical accent"
  mainState = msIDLE;
}
