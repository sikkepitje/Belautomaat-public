/*
  Belautomaat_driver.h 
  Paul Wiegmans <p.wiegmans@svok.nl>

*/

#ifndef Belautomaat_driver_h
#define Belautomaat_driver_h

#include <Wire.h>

/* ======== Debug hulpjes ======== */
// Definieer DEBUG om debug-print-statements in de code actief te maken.
#define DEBUG

#ifndef DPRINT
  #ifdef DEBUG
    #define DPRINT(...)     Serial.print(__VA_ARGS__)
    #define DPRINTLN(...)   Serial.println(__VA_ARGS__)
  #else
    #define DPRINT(...)
    #define DPRINTLN(...)
  #endif
#endif

/*--- RTC---*/
#include <RTClib.h>
// #define RTC_ADDR 0x68 // impliciet adres

/*--- I2C LCD  --- */
#include <LiquidCrystal_I2C.h> 
#define I2CADDR_LCD1 0x3f
#define I2CADDR_LCD2 0x27
/*--------------------------- Keypad ----------------------------*/
// keypad met PCF874
#include <FabricaDigital_MCP23008.h>
#include <Keypad_MCP23008.h>
// Keypad met MCP23008
#include <Keypad_I2C.h>
#include <Keypad.h>

#define UP     '\21'
#define DOWN   '\22'
#define SELECT '\23'
#define BACK   '\24'
#define OK     SELECT 
#define EXIT   BACK

/* Keypad 1 is matrixkeyboard met PCF8574 op adres 0x3C */
#define KEYPAD1_I2CADDR 0x3c
/* Keypad 2 is matrixkeyboard met MCP23008 op adres 0x24 */
#define KEYPAD2_I2CADDR 0x24

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

/*--------------------------- sevenseg --------------------------*/
//#include <Adafruit_GFX.h>
//#include <Adafruit_LEDBackpack.h>
#define SEVENSEG_ADDR 0x70
#include "HT16K33_7segLED.h"

/*--------------------------- active buzzer ---------------------*/
#define BUZZER 7

/*--------------------------- relais board ----------------------*/
// pinnen van relaisboard
#define RELAISPIN1 5
#define RELAISPIN2 4
#define RELAISPIN3 3
#define RELAISPIN4 2
#define RELAISON 0
#define RELAISOFF 1

/*--------------------------- DFPlayer --------------------------*/
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
// Arduino pin 11 (software serial TX) verbonden met DFPlayer pin 2 (RX)
#define SOFTSER_TX 11
// Arduino pin 10 (software serial RX) verbonden met DFPlayer pin 3 (TX)
#define SOFTSER_RX 10

/* ================================== TYPES ==================================== */

/* ================================== VARIABLES  =============================== */
/*--------------------------- RTC--------------------------------*/
extern RTC_DS3231 rtc;
extern const char weekdagenLang[7][10];
extern const char weekdagenKort[7][3];
extern int rtcyear, rtcmonth, rtcday, rtchour, rtcminute, rtcsecond, rtcweekday;
/*--------------------------- I2C LCD -------------------------- */
extern LiquidCrystal_I2C lcd;
/*--------------------------- Keypad ----------------------------*/
extern bool kp1_present;
extern bool kp2_present;
extern const char keys[KEYPAD_ROWS][KEYPAD_COLS];

/*--------------------------- sevenseg --------------------------*/
//extern Adafruit_7segment sevenseg;
extern HT16K33_7segled sevenseg;
/*--------------------------- active buzzer ---------------------*/
/*--------------------------- relais board ----------------------*/
/*--------------------------- DFPlayer --------------------------*/
extern DFRobotDFPlayerMini mp3; 
extern int mp3count;
/*--------------------------- Belautomaat Algemeen --------------*/

/* ================================== FUNCTIONS  =============================== */
/*--------------------------- RTC--------------------------------*/
extern void setup_rtc();
extern void get_time();
/*--------------------------- I2C LCD ---------------------------*/
extern void setup_i2c_lcd();
extern void test_i2c_lcd();
/*--------------------------- Keypad ----------------------------*/
extern char getKey();
extern void setup_keypad();
extern void test_keypad();
/*--------------------------- sevenseg --------------------------*/
extern void setup_sevenseg();
extern void sevenseg_out(int hour, int minute, bool dots);
extern void test_sevenseg();
/*--------------------------- active buzzer ---------------------*/
extern void setup_buzzer();
extern void buzzerOn();
extern void buzzerOff();
extern void klik();
extern void test_buzzer();
/*--------------------------- relais board ----------------------*/
extern void setup_relaisboard();
extern void relaisOff(int kanaal);
extern void relaisOn(int kanaal);
/*--------------------------- DFPlayer --------------------------*/
extern void mp3play(int nummer);
extern bool mp3IsKlaar();
//extern void mp3PrintStatus();
extern void mp3DiscardStatus(); 
/*--------------------------- Belautomaat Algemeen --------------*/
extern void mp3volume_save(int newvolume);
extern void signaallengte_save(int nwlengte);
extern bool i2c_device_present(byte address);
extern void setup_belautomaat_driver();

#endif // #ifndef Belautomaat_driver_h

