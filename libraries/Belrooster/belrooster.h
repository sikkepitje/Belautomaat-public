/*
  belrooster.h
  20180819 
  Paul Wiegmans <p.wiegmans@svok.nl>
*/

#ifndef Belrooster_h
#define Belrooster_h

#include <EEPROM.h> 

/* ======== Debug hulpjes ======== */
#ifndef DEBUG
//#define DEBUG
#endif
#ifndef DPRINT
  #ifdef DEBUG
    #define DPRINT(...)     Serial.print(__VA_ARGS__)
    #define DPRINTLN(...)   Serial.println(__VA_ARGS__)
  #else
    #define DPRINT(...)
    #define DPRINTLN(...)
  #endif
#endif

/* Roostertabel en timtab grootte is afhankelijk van EEPROM grootte en board.  
The symbol E2END is the last EEPROM address.
In runtime wordt EEPROM-grootte gegeven door EEPROM.length(); 
*/
#define EEPROMSIZE (E2END+1)
#define INT_WIDTH (2)
#define MARKER_STRING "Belrooster 20190501"
// limieten
#define MAXROOSTER 10
#define MAX_NAAMLEN 8
#define MAX_EEVAR 6
#define ROOSTERSIZE ((EEPROMSIZE-EEADR_ROOSTER)/4)
// adressen in EEPROM
#define EEADR_NAAM 32
#define EEADR_EEVAR (EEADR_NAAM+(MAXROOSTER*MAX_NAAMLEN))
#define EEADR_UITGANG (EEADR_EEVAR+(MAX_EEVAR*INT_WIDTH))
#define EEADR_ROOSTER (EEADR_UITGANG+(MAXROOSTER*INT_WIDTH))

#define EMPTY 255
// adressen van EEVARS mp3volume, signaallengte, hoofdschakelaar
#define EEVAR_MP3VOL 0
#define EEVAR_SGNLEN 1
#define EEVAR_HOOFDS 2

// Retourwaarden van findFreeEeprom()
#define ERROR_TIMTAB_FULL (-2)
#define ERROR_EEPROM_FULL (-3)
#define INFO_NONE_ACTIVE (-4) 
#define WARN_RECORD_MERGED (-5)
#define WARN_NOMORESIGNALS (-6)

struct Signaal_t {
    uint8_t hour;
    uint8_t minute; 
    uint8_t dayOfWeek; // bitmap voor dagweek; bit 6 = maandag, bit 5 = dinsdag, enz
    uint8_t channel;
};
 
/* Algemene hulpfuncties */
extern int EEVarLoad(int index);                // laad variabele waarde uit EEPROM op index 
extern void EEVarStore(int index, int value);   // bewaar een variabele waarde in EEPROM op index

class Rooster {
private:
    // rooster tabel in EEPROM
    int lastFreeEeprom;     // versnelt findFreeEprom door bewaren laat gevonden vrije plaats in EEPROM.
    int nextindex;  // Signaal met deze index is volgende om actief te worden.
    void initTimtab(void);           // indextabel opbouwen en sorteren.
    bool testEepromMarker();      // controller marker in EEPROM, geef true indien aanwezig
    int index2adres(int eeindex);   // omrekenen van index naar adres in EEPROM 
    int adres2index(int adres);     // omrekenen van adres naar index in EEPROM
    void leesSignaalEEPROM(int eeindex, Signaal_t &sig);  // primitieve functie Signaal ophalen uit EEPROM 
    void schrijfSignaalEEPROM(int eeindex, Signaal_t &sig);  // primitieve functie Signaal opslaan in EEPROM    
public:
    int numtimtab;
    Signaal_t nextSignaal;
    int timtab[ROOSTERSIZE];
    unsigned int hoofdschakelaar; // bool array: is rooster[n] actief? PUBLIC VOOR DEBUGGING

    Rooster(void);          // constructor
    /*---- hulp methoden ----*/
    void schoonRooster(void);         // initialiseer alle tabellen, zet marker in EEPROM
    int vergelijkSignaal(Signaal_t &sa, Signaal_t &sb);   // vergelijk 2 Signaalen, geef -1, 0 of 1
    int vergelijkSignaalEeprom(int eeidx1, int eeidx2);    // vergelijk 2 Signaalen aangeduid met index
    void sorteerTimtab(void);
    void randomSignaal(Signaal_t &sig);  // DEBUG geef een random Signaal 
    bool binairZoeken(Signaal_t &sig, int &piInsert);
    bool simpelZoeken(Signaal_t &sig, int &matchpos);
    int vindVrijeEeindex(void);  // zoek eerst vrije adres in EEPROM, geef index of ERROR_EEPROM_FULL
    void signaalWissen(int tijd);    // Signaal verwijderen uit timtab, behoud sortering
    int signaalInvoegen(Signaal_t &sig);  // Signaal invoegen, retourneer status = 0 bij succes, <0 bij fout
    int signaalInvoegen(int hour, int minute, int dayOfWeek, int rooster);
    void signaalLezen(int tijd, Signaal_t &sig);  // Signaal ophalen uit timtab 
    //char *printDayOfWeek2(char *buffer, int dayOfWeek);  // print bitmap weekdag, 2 tekens per dag
    //void haalSignaaltijd(int index, Signaal_t &sig);    // duplicaat van signaalLezen()
    bool isDagActief(int dow, int dowbitmap); // bepaalt of signaal actief op DOW-bitmap  
    int update(int hour, int minute, int second, int dow);  // hart van de scheduler, roep dit aan eens per minuut.
    int getNextIndex(void);                     // geef nextindex
    void findNextIndex(int hour, int minute);   // werk nextindex bij voor de gegeven tijd
    int aantalSignalen();        // geef aantal signaaltijden in EEPROM of timtab
    int haalVolgend(int index, int filter);     // blader naar volgend signaal in gegeven kanaal
    int haalVorig(int index, int filter);       // blader naar vorig singnaal in gegeven kanaal
    void zetactiefrooster(int index, bool isactief);
    bool isactiefrooster(int index);
    void zetnaam(int nummer, char *naam);
    char* haalnaam(int nummer);
    void zetuitgang(int nummer, int uitgang);
    int haaluitgang(int nummer);
#ifdef DEBUG
    char *printDayOfWeek1(char *buffer, int dayOfWeek);  // print weekdag bitmap, 1 teken per dag
    char *print2decimals(char *buffer, int value);
    char *printndecimals(char *buffer, int value, int numberofchars);
    char *printHex(char *buffer, int num);
    char *printSignaal(char *buffer, Signaal_t &sig);
    char *printRoosterRegel(int index);  // DEBUG
    void printRooster();   // DEBUG
    void printStatus(int status);  // DEBUG
#endif 
};

#endif
// #ifndef Belrooster_h