/*
  belrooster.h
  20180819 Paul Wiegmans
*/

#ifndef Belrooster_h
#define Belrooster_h

#define DEBUG
#include <EEPROM.h> 

/* ======== Debug hulpjes ======== */
//#define DEBUG
#ifdef DEBUG
  #define DPRINT(...)     Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)   Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)
  #define DPRINTLN(...)
#endif

// Roostertabel en timtab grootte is afhankelijk van board hardware  
// zie definities in "C:\Program Files (x86)\Arduino\hardware\arduino\avr\boards.txt"
#define EEPROM_ROOSTER_START_ADRES 64
#if defined(ARDUINO_AVR_MEGA2560)       
    // Arduino Mega EEPROM 4KB
    #define ROOSTERSIZE 512
#elif defined(ARDUINO_AVR_UNO)     
    // Arduino Uno EEPROM 1KB
    #define ROOSTERSIZE 240
#elif defined(ARDUINO_AVR_DUEMILANOVE)     
    // Arduino Duemilanove EEPROM: 512B (ATMega168) of 1KB (ATMega328)
    #define ROOSTERSIZE 112
#elif defined(ARDUINO_AVR_LEONARDO)     
    // Arduino Leonardo of Micro Pro: 1KB (ATMega32U4)
    #define ROOSTERSIZE 112
#else
    #error "Unknown board"
#endif
#define EMPTY 255

// returned by findFreeEeprom
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
 
class Rooster {
public:
    int numtimtab;
    int nextindex;  // Signaal met deze index is volgende om actief te worden.
    // rooster tabel in EEPROM
    Signaal_t nextSignaal;
    int timtab[ROOSTERSIZE];
    int lastFreeEeprom;     // versnelt findFreeEprom door bewaren laat gevonden vrije plaats in EEPROM.

    Rooster(void);          // constructor
    /*---- hulp methoden ----*/
    void schoonRooster(void);         // initialiseer alle tabellen, zet marker in EEPROM
    int vergelijkSignaal(Signaal_t &bm1, Signaal_t &bm2);   // vergelijk 2 Signaalen, geef -1, 0 of 1
    int vergelijkSignaalEeprom(int eeidx1, int eeidx2);    // vergelijk 2 Signaalen aangeduid met index
    void sorteerTimtab(void);

    void randomSignaal(Signaal_t &bm);  // DEBUG geef een random Signaal 
    void signaalWissen(int ttindex);    // Signaal verwijderen uit timtab, behoud sortering
    bool binairZoeken(Signaal_t &bm, int &piInsert);
    bool simpelZoeken(Signaal_t &bm, int &matchpos);
    int vindVrijeEeindex(void);  // zoek eerst vrije adres in EEPROM, geef index of ERROR_EEPROM_FULL
    int signaalInvoegen(Signaal_t &bm);  // Signaal invoegen, retourneer status = 0 bij succes, <0 bij fout
    int signaalInvoegen(int hour, int minute, int dayOfWeek, int channel);
    char *printDayOfWeek2(char *buffer, int dayOfWeek);  // print bitmap weekdag, 2 tekens per dag
    char *printDayOfWeek1(char *buffer, int dayOfWeek);  // print weekdag bitmap, 1 teken per dag
    char *print2decimals(char *buffer, int value);
    char *printndecimals(char *buffer, int value, int numberofchars);
    char *printHex(char *buffer, int num);
    char *printSignaal(char *buffer, Signaal_t &bm);
    void printRoosterRegel(int index);  // DEBUG
    void printRooster();   // DEBUG
    void printStatus(int status);  // DEBUG
    uint8_t update(int hour, int minute, int dow);  // hart van de scheduler, roep dit aan eens per minuut.
    int aantalSignalen();        // geef aantal signaaltijden in EEPROM of timtab
    int haalVolgend(int index, int filter);     // blader naar volgend signaal in gegeven kanaal
    int haalVorig(int index, int filter);       // blader naar vorig singnaal in gegeven kanaal
private:
    void initTimtab(void);           // indextabel opbouwen en sorteren.
    bool testEepromMarker();      // controller marker in EEPROM, geef true indien aanwezig
    int index2adres(int eeindex);   // omrekenen van index naar adres in EEPROM 
    int adres2index(int adres);     // omrekenen van adres naar index in EEPROM
    void laadSignaal(int eeindex, Signaal_t &bm);  // Signaal ophalen uit EEPROM 
    void bergopSignaal(int eeindex, Signaal_t &bm);  // Signaal opslaan in EEPROM
};

#endif
