/*
	Belautomaat-driver.h 
	20181111
	Paul Wiegmans <p.wiegmans@bonhoeffer.nl>
*/

#ifndef Belautomaat_driver_h
#define Belautomaat_driver_h

#include <Wire.h>
/*--- RTC---*/
#include <RTClib.h>
// #define RTC_ADDR 0x68 // impliciet adres

/*--- I2C LCD  --- */
#include <LiquidCrystal_I2C.h> 

/*---I2C keypad---*/
#include <Keypad_I2C.h>
#include <Keypad.h>
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
#define KEYPAD_I2CADDR 0x3c
#define UP     'u'
#define DOWN   'd'
#define SELECT 's'
#define OK     's'
#define BACK   'b'
#define EXIT   'b'

/*--- sevenseg ---*/
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#define SEVENSEG_ADDR 0x70

/*--- timer ---*/
#include "Timer.h"

/*--- active buzzer ---*/
#define BUZZER 7

/*--- relais board ------*/

/* ====== TYPES ====== */

/* ====== VARIABLES  ====== */
/*--- RTC---*/
extern RTC_DS3231 rtc;
extern char weekdagenLang[7][10];
extern char weekdagenKort[7][3];
extern int rtcyear, rtcmonth, rtcday, rtchour, rtcminute, rtcsecond, rtcweekday;

/*--- I2C LCD --- */
extern LiquidCrystal_I2C lcd;
/*---I2C keypad---*/
extern char keys[KEYPAD_ROWS][KEYPAD_COLS];
extern byte keypad_rowPins[KEYPAD_ROWS];
extern byte keypad_colPins[KEYPAD_COLS];
extern Keypad_I2C keypad;
/*--- sevenseg ---*/
extern Adafruit_7segment sevenseg;
/*--- timer ---*/
extern Timer timer;
/*--- active buzzer ---*/
/*--- relais board ------*/

/* ====== EXPORTED FUNCTIONS ====== */
/*--------------------------- RTC---------------------------*/
extern void setup_rtc();
extern void get_time();
extern char* get_datestring();
extern char* get_timestring();
/*--------------------------- I2C LCD --------------------------- */
extern void setup_i2c_lcd();
extern void test_i2c_lcd();
/*---------------------------I2C keypad---------------------------*/
extern void setup_keypad();
extern void test_keypad();
extern char getKey();
/*--------------------------- sevenseg ---------------------------*/
extern void setup_sevenseg();
extern void sevenseg_out(int hour, int minute, bool dots);
extern void test_sevenseg();
/*--------------------------- active buzzer ---------------------------*/
extern void setup_buzzer();
extern void buzzerOn();
extern void buzzerOff();
extern void klik();
extern void test_buzzer();
/*--------------------------- relais board ---------------------------*/
/*--------------------------- Belautomaat Algemeen ---------------------------*/
extern void setup_belautomaat_driver();

#endif // #ifndef Belautomaat_driver_h

