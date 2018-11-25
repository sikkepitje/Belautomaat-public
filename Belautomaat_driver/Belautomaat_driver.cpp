/*
    Belautomaat-lib.cpp 
    20181125
    Paul Wiegmans <p.wiegmans@bonhoeffer.nl>

    Bibliotheek voor de Belautomaat-hardware:
    * 7-sec LED 
    * 16*2 LCD (Benselectronics GO5)
    * 3*4 phonestyle I2C keypad
    * 4 menuknoppen
    * RTC DS3231
    * active buzzer 
    * relais board

    Vermeld in je code:
    #include <Belautomaat_driver.h>
*/

#include "Arduino.h"
#include "Belautomaat_driver.h"

/*--- RTC---*/
RTC_DS3231 rtc;
char weekdagenLang[7][10] = {"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"};
char weekdagenKort[7][3] = {"Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"};
 int rtcyear, rtcmonth, rtcday, rtchour, rtcminute, rtcsecond, rtcweekday;

/*--- I2C LCD (Benselectronics GO5) --- */
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/*---I2C keypad---*/
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3',DOWN},
  {'4','5','6',SELECT},
  {'7','8','9',BACK},
  {'*','0','#',UP}
};

// Paul Wiegmans I2C keypad, bit numbers of PCF8574 i/o port
byte keypad_rowPins[KEYPAD_ROWS] = {4,5,6,7}; //connect to the row pinouts of the keypad
byte keypad_colPins[KEYPAD_COLS] = {0, 1, 2, 3}; //connect to the column pinouts of the keypad
Keypad_I2C keypad( makeKeymap(keys), keypad_rowPins, keypad_colPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD_I2CADDR, PCF8574 );

/*--- sevenseg ---*/
Adafruit_7segment sevenseg = Adafruit_7segment();

/*--- timer ---*/
Timer timer;

/*--- active buzzer ---*/

/*--- relais board ------*/

/* ====== FUNCTIONS ====== */

/*---------------------------- RTC--------------------------*/
void setup_rtc() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void get_time() {
  DateTime now = rtc.now();
  rtcyear = now.year();
  rtcmonth = now.month();
  rtcday = now.day();
  rtchour = now.hour();
  rtcminute = now.minute();
  rtcsecond = now.second();
  rtcweekday = now.dayOfTheWeek();
}

char* get_datestring() {
  static char buffer[16];
  sprintf(buffer, "%s %4d-%02d-%02d",
    weekdagenKort[rtcweekday], rtcyear, rtcmonth, rtcday);
  return buffer;
}

char* get_timestring() {
  static char buffer[24];
  sprintf(buffer, "%02d:%02d:%02d", rtchour, rtcminute, rtcsecond);
  return buffer;
}

/*---------------------------- I2C LCD ----------------------------*/
const uint8_t charBitmap[][8] = {
  { // dummy karakter (kan geen 0 gebruiken in een string)
  0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 },
  { // bol met lijn omlaag
  0x00, 0x00, 0x06, 0x09, 0x09, 0x06, 0x06, 0x06 },
  { // bol met lijn omhoog en omlaag
  0x06, 0x06, 0x06, 0x09, 0x09, 0x06, 0x06, 0x06 },
  { // bol met lijn omhoog
  0x06, 0x06, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00 },
  { // bol 
  0x00, 0x00, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00 },
  { // item rechts
  0x00, 0x00, 0x00, 0x00, 0x10, 0x18, 0x0C, 0x06 },
  { // item links 
  0x0C, 0x06, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00 },
  { // bol onder
    0x04, 0x04, 0x04, 0x04, 0x04, 0x0A, 0x0A, 0x04 } 
};

/*

*/
void setup_i2c_lcd() {
  int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));  
  lcd.begin(16,2);               // initialize the lcd 

  for ( int i = 0; i < charBitmapSize; i++ )
  {
     lcd.createChar ( i, (uint8_t *)charBitmap[i] );
  }
}

void test_i2c_lcd() {
  lcd.home ();
  lcd.print("Hallo wereld!");
}

/*---------------------------- I2C keypad ----------------------------*/
void setup_keypad() {
  keypad.begin( makeKeymap(keys) );
}

void test_keypad() {
  char key = keypad.getKey();  
  if (key){
    lcd.home();
    lcd.print("key pressed:");
    lcd.println(key);
   }  
}


/*---------------------------- sevenseg ----------------------------*/
void setup_sevenseg() {
  sevenseg.begin(SEVENSEG_ADDR);
}

void sevenseg_out(int hour, int minute, bool dots) {
  sevenseg.writeDigitNum(0, hour / 10, false);
  sevenseg.writeDigitNum(1, hour % 10, false);
  sevenseg.drawColon(dots);
  sevenseg.writeDigitNum(3, minute / 10, false);
  sevenseg.writeDigitNum(4, minute % 10, false);
  sevenseg.writeDisplay();
}

void test_sevenseg() {
  static int lastsecond;
  boolean dots = false;
  dots = ((rtcsecond % 2) == 0);
  sevenseg_out(rtchour, rtcminute, dots);
}

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
/*---------------------------- active buzzer ----------------------------*/
void setup_buzzer() {
  pinMode(BUZZER, OUTPUT);
}

void buzzerOn() {
  digitalWrite(BUZZER, HIGH);
}

void buzzerOff() {
  digitalWrite(BUZZER, LOW);
}

void klik() {
  buzzerOn();
  delay(2);
  buzzerOff();
}

void test_buzzer() {
  klik();
}

/*---------------------------- Belautomaat Algemeen ----------------------------*/
void setup_belautomaat_driver() {
  Wire.begin();
  setup_rtc();
  setup_i2c_lcd();
  setup_keypad();
  setup_sevenseg();
  setup_buzzer();
}
