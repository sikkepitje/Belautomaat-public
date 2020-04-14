/*
  Belautomaat_driver.cpp
	Paul Wiegmans <p.wiegmans@svok.nl>


  historie
  11-11-2018 eerste versie
  14-4-2020 actuele versie
  
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

  20200105
  * toegevoegd keypad met MCP23008 
  * verwijderd analogkeypad
  * detectie van adressen voor I2C LCD adapter
*/

#include "Arduino.h"
#include "Belautomaat_driver.h"
/* ========================== VARIABLES  ======================= */

/*--------------------------- RTC--------------------------------*/
RTC_DS3231 rtc;
//const char weekdagenLang[7][10] = {"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"};
const char weekdagenKort[7][3] = {"Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"};
int rtcyear, rtcmonth, rtcday, rtchour, rtcminute, rtcsecond, rtcweekday;

/*--------------------------- I2C LCD -------------------------- */
// Hoe krijg ik dit zover dat het test op 2 adressen en 
// de juiste LCD wordt benaderd in classvariabele  'lcd' ??
bool lcd1_present, lcd2_present;
LiquidCrystal_I2C lcd(I2CADDR_LCD1, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
LiquidCrystal_I2C lcd2(I2CADDR_LCD2, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/*--------------------------- Keypad ----------------------------*/
bool kp1_present, kp2_present;
const char keys1[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3',DOWN},
  {'4','5','6',SELECT},
  {'7','8','9',BACK},
  {'*','0','#',UP}
};
byte kp1_rowPins[KEYPAD_ROWS] = {4,5,6,7}; //connect to the row pinouts of the keypad
byte kp1_colPins[KEYPAD_COLS] = {0,1,2,3}; //connect to the column pinouts of the keypad
Keypad_I2C keypad1( makeKeymap(keys1), kp1_rowPins, kp1_colPins, KEYPAD_ROWS, KEYPAD_COLS, KEYPAD1_I2CADDR, PCF8574 );

char keys2[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3',DOWN},
  {'4','5','6',SELECT},
  {'7','8','9',BACK},
  {'*','0','#',UP}
};
byte kp2_rowPins[KEYPAD_ROWS] = {4, 5, 6, 7}; //connect to the row pinouts of the keypad
byte kp2_colPins[KEYPAD_COLS] = {1, 2, 3, 0}; //connect to the column pinouts of the keypad
Keypad_MCP23008 keypad2 = Keypad_MCP23008(kp2_rowPins, kp2_colPins, KEYPAD_ROWS, KEYPAD_COLS);

/*--------------------------- sevenseg --------------------------*/
//Adafruit_7segment sevenseg = Adafruit_7segment();
HT16K33_7segled sevenseg;
/*--------------------------- active buzzer ---------------------*/
/*--------------------------- relais board ----------------------*/
/*--------------------------- DFPlayer --------------------------*/
SoftwareSerial mp3Serial(SOFTSER_RX, SOFTSER_TX); // RX, TX
DFRobotDFPlayerMini mp3;

/*--------------------------- Belautomaat Algemeen --------------*/

/* ========================== FUNCTIONS  ======================= */

/*--------------------------- RTC--------------------------------*/
void setup_rtc() {
  if (! rtc.begin()) {
    DPRINTLN(F("Couldn't find RTC"));
    lcd.clear();
    lcd.print(F("Couldn't find RTC"));
    while (1);
  }
}

void get_time() {   // sla datum en tijd uit RTC op in globale variabelen
  DateTime now = rtc.now();
  rtcyear = now.year();
  rtcmonth = now.month();
  rtcday = now.day();
  rtchour = now.hour();
  rtcminute = now.minute();
  rtcsecond = now.second();
  rtcweekday = now.dayOfTheWeek();
}

/*--------------------------- I2C LCD ---------------------------*/
// gebruik de coole online character editor op https://maxpromer.github.io/LCD-Character-Creator/
// zelf-gedefinieerde tekens: dichte pijlen
// zie ook: "C:\Users\p.wiegmans\OneDrive - Stichting Voortgezet Onderwijs Kennemerland\Documents\Belautomaat 2018\Code\notities.txt"
const uint8_t charBitmap[][8] = {
  { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 }, // dummy karakter (kan geen 0 gebruiken in een string)    
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x0E, 0x04 },// pijl omlaag    
  { 0x04, 0x0E, 0x1F, 0x00, 0x00, 0x1F, 0x0E, 0x04 },// pijl omhoog en omlaag    
  { 0x04, 0x0E, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00 },// pijl omhoog    
  { 0x00, 0x00, 0x00, 0x0E, 0x0E, 0x0E, 0x00, 0x00 },// bol     
  { 0x00, 0x00, 0x08, 0x0C, 0x0E, 0x0C, 0x08, 0x00 },// pijl rechts    
  { 0x00, 0x00, 0x02, 0x06, 0x0E, 0x06, 0x02, 0x00 },// pijl links     
  { 0x04, 0x04, 0x04, 0x04, 0x04, 0x0A, 0x0A, 0x04 } // bol onder    
};

void setup_i2c_lcd() {
  int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));  
  DPRINTLN(F("Init i2c lcd"));
  lcd1_present = i2c_device_present(I2CADDR_LCD1);
  lcd2_present = i2c_device_present(I2CADDR_LCD2);
  if (lcd1_present) {
    DPRINTLN(F("  detected LCD1"));
  } else if (lcd2_present)
  {
    DPRINTLN(F("  detected LCD2"));
    lcd = lcd2;
  } else 
  {
    DPRINTLN(F("fatal error: no LCD detected"));
  }
  
  lcd.begin(16,2);               // initialize the lcd 
  for ( int i = 0; i < charBitmapSize; i++ )  {
     lcd.createChar ( i, (uint8_t *)charBitmap[i] );
  }
  lcd.clear();
}

void test_i2c_lcd() {
  lcd.clear();
  lcd.print(F("Hallo werld "));
  lcd.print(rtcsecond);
}

/*--------------------------- Keypad ----------------------------*/
void setup_keypad() {
  DPRINTLN(F("Init keypad"));
  Wire.begin();
  kp1_present = i2c_device_present(KEYPAD1_I2CADDR);
  kp2_present = i2c_device_present(KEYPAD2_I2CADDR);
  if (kp1_present) {
    //keypad.begin( makeKeymap(keys) );
    DPRINTLN(F("  detected keypad1"));
    keypad1.begin( makeKeymap(keys1) );    
  } 
  else if (kp2_present) {
    DPRINTLN(F("  detected keypad2"));
    keypad2.begin(KEYPAD2_I2CADDR, makeKeymap(keys2));
  } else {
    lcd.clear();
    lcd.println(F("Fatal error"));
    lcd.println(F("No keypad"));
    while (1) ; // loop forever
  }
}

char getKey() {
  char key;
  if (kp1_present) {
    key = keypad1.getKey(); 
  } else {
    key = keypad2.getKey();
  }
  return key;
}

void test_keypad() {
  // zie "voorbeeld-BHW-Keypad.ino"
}
/*--------------------------- sevenseg --------------------------*/
void setup_sevenseg() {
  DPRINTLN(F("Init sevenseg"));
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

/*--------------------------- active buzzer ---------------------*/
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

/*--------------------------- relais board ----------------------*/
const int relaispinnen[4] = {RELAISPIN1,RELAISPIN2,RELAISPIN3,RELAISPIN4};

void relaisState(int kanaal, int state) {
  digitalWrite(relaispinnen[kanaal], state);  
}
void relaisOff(int kanaal) { 
  relaisState(kanaal, RELAISOFF);
}
void relaisOn(int kanaal) { 
  relaisState(kanaal, RELAISON);
}

void setup_relaisboard() {
  DPRINTLN(F("Init relaisboard"));
  pinMode(RELAISPIN1, OUTPUT); relaisOff(0);
  pinMode(RELAISPIN2, OUTPUT); relaisOff(1);
  pinMode(RELAISPIN3, OUTPUT); relaisOff(2);
  pinMode(RELAISPIN4, OUTPUT); relaisOff(3);
}

/*--------------------------- DFPlayer --------------------------*/
bool mp3PlayFinished;
int mp3count;           // number of available mp3 tracks (in folder 01)

void setup_dfplayer() {
  DPRINTLN(F("Init dfplayer"));
  mp3Serial.begin(9600);
  if (!mp3.begin(mp3Serial)) {  //Use softwareSerial "mp3Serial" to communicate with DFplayer.
    DPRINTLN(F("  failed"));
  }  // fail but continue
  mp3.volume(10);
  mp3.setTimeOut(500); //Set serial communication time out 500ms
  mp3.EQ(DFPLAYER_EQ_NORMAL);
  mp3.outputDevice(DFPLAYER_DEVICE_SD);
  mp3count = mp3.readFileCountsInFolder(1);
}

void mp3play(int nummer) {
  mp3.playFolder(1, nummer);  // speel file SD:/01/nnn.mp3
  mp3PlayFinished = false;
}

void mp3DiscardStatus() {
  if (mp3.available()) {
    mp3.readType();
    mp3.read();
  }
}

bool mp3IsKlaar() {
  if (mp3.available()) {
    int type = mp3.readType();
    int value = mp3.read();
    if (type == DFPlayerPlayFinished) {
      mp3PlayFinished = true;
    }
  }
  return mp3PlayFinished;
}

/*--------------------------- Belautomaat Algemeen --------------*/

byte i2c_test_address(int address) {
  // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.
  // error == 0 : device found at address <address>
  Wire.beginTransmission(address);
  return Wire.endTransmission();
}

bool i2c_device_present(byte address) {
  byte status = i2c_test_address(address);
  DPRINT("I2C address:0x");
  DPRINT(address, HEX);
  DPRINT(" present:");
  DPRINTLN(status == 0);
  return status == 0;
}

void setup_belautomaat_driver() {
  DPRINTLN(F("Init belautomaat driver"));
  Wire.begin();
  setup_i2c_lcd();
  setup_rtc();
  setup_sevenseg();
  setup_relaisboard();
  setup_dfplayer();
  setup_buzzer();
  setup_keypad();
}
