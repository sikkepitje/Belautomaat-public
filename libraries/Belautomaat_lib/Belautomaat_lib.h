/*
	Belautomaat_lib.h 
	Paul Wiegmans <p.wiegmans@svok.nl>
*/

#ifndef Belautomaat_lib_h
#define Belautomaat_lib_h

//#include "Arduino.h"
#include "Belautomaat_driver.h"
#include "belrooster.h"
/*--- timer ---*/
#include "Timer.h"

#define MENU_TIMEOUT_MS 15000
#define UPDATE_INTERVAL_MS 20
#define ALFAKEY_TIMEOUT 1000
#define FKS_MAX_ROOSTERS 10

// zelfgedefineerde tekens op LCD , voor weergave van UI_kiesRooster()
#define EERSTE_ITEM char(1)
#define MIDDELSTE_ITEM char(2)
#define LAATSTE_ITEM char(3)
#define ITEM_RECHTS char(5)

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

/* ====== TYPES ====== */
typedef void (*t_voidfunc)();
enum mainState_t { msIDLE, msMENU, msACTION, msKEYLOCK };

/* ====== VARIABLES  ====== */
extern mainState_t mainState;
extern Rooster r;
extern Timer timer;
extern unsigned long time_to_dim_lcd;
extern unsigned long time_to_exit_menu;
extern unsigned long time_to_task;

extern int fkanaal;        // huidig gekozen kanaal
extern int fks[ROOSTERSIZE];   // filtered set voor dit kanaal
extern int fksmax;             // aantal signalen in filtered set
extern int fksindex;
extern bool fks_needupdate;      // geeft aan of opnieuw filtered kanaalset opbouw nodig is

/* ====== EXPORTED FUNCTIONS ====== */
extern boolean once_per_second();
extern t_voidfunc belautomaat_idler; 
/*--------------------------- Belautomaat Algemeen ---------------------------*/
extern void light_lcd();
extern char getkb();
extern char waitforkey();
extern char getkb_timeout(unsigned long timeout);
extern char getalfakey();

extern void sound_error();
extern void sound_enter_success();
extern void sound_enter_fail();
extern void sound_end_reached();
extern void print_datum(int jaar, int maand, int dag, int weekdag);
extern void print_tijd(int uur, int minuut, int seconde);
extern void print_dowbitmap( int dayOfWeek);
extern void printSignaaltijd(int index);

extern void triggerUitgang(int uitgang);

int get_mp3volume();
void set_mp3volume(int value);
int get_signaallengte();
void set_signaallengte(int value);

/*-------------------------- Editfuncties ---------------------------*/
extern void print_2digit(int n);
extern bool is_number_char(char c);
extern bool UI_edit_time(int posx, int posy, int &hour, int &minute);
extern bool UI_edit_2digit(int xoffset, int yoffset, int &value, int limiet);
extern bool UI_edit_1digit(int xoffset, int yoffset, int &value, int limiet);
extern bool UI_edit_number(int xoffset, int yoffset, int &value, int limiet);
extern bool UI_edit_naam(char *naam);
/*-------------------------- UI functies ---------------------------*/
extern void LCD_Printweekdag(int weekdag);
extern bool UI_edit_weekdag(int xoffset, int yoffset, int &weekdag);
extern void UI_setVolume();
extern void UI_bladerGeluiden();
extern void UI_toonVersie();
extern void UI_toondatumtijd();
extern void UI_setTime();
extern void UI_setDate();
extern void UI_toetsenbordslot();
extern void UI_dummy();
extern void UI_handbedienRelais();
extern void vul_fks(int kfilter);
extern void UI_signaal_kies();
extern void UI_EditSignaallengte();

extern void UI_edit_roosternaam(int roosternr);
extern void UI_edit_schakelaar();
extern void UI_bladerRooster2();

/*-------------------------- Lib setup ---------------------------*/
extern void setup_belautomaat_lib();

#endif // #ifndef Belautomaat_lib_h

