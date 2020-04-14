/*
 * voorbeeld-BHW-Belautomaat
 * 20181125 Paul Wiegmans
 * met Belautomaat hardware  !
  20190427 ondersteuning voor zowel analog keypad als I2C keypad
  met automatische detectie. 
  20191016 laatste bug eruit gehaald
  20200107 bug: meer dan 127 signaaltijden stuurt belautomaat het bos in.
  

TO DO lijst
[x] toetsenbordvergrendeling (20190501) toegevoegd
[x] ontgrendeling met pincode is makkelijk te hacken (20190705) weergave verbeterd
[x] toevoegen menu item voor opnieuw vergrendelen (20190801) toegevoegd
[ ] kanaal wordt rooster
[ ] label bij elk roosterkeuze 
[x] informatie over geheugengrootte en gebruik: free RAM weergave, meer niet nodig (20190705)
[x] vereenvoudig custom defined LCD characters voor menugraphics (20190801) simpele pijlen
[x] menu werkt zelfs wanneer toetsenbord op slot
[x] toetsenbordslot na ontgrendeling is niet opnieuw te vergrendelen
[x] 20190916 datuminstelfunctie :-)
[x] roosters>bewerk>bladert van 0..7, moet 0..9, FKS_MAX_ROOSTERS verhoogd van 8 naar 10
[x] 20190919 nieuwe signaaltijd test weekdag
[x] 20190919 nextindex reset na set_time
[x] printfuncties in roostermodule uitgeschakeld, weekdag-print eenduidig
[x] 20190919 Weekdag test gaat fout! logica herschreven voor opslag, weergave en test van datumveld
test: 
    if ((1 << dow) & bm.dayOfWeek) {
weergave:
  "MDWDVZZ"  
  masker = 64;
  masker >>= 1     

RTC: Zondag=0 .. zaterdag=6  
[ x] 20191013 niet alle signaaltijden worden getest op een moment dat deze signaaltijden allemaal 
actief zouden moeten worden. Op actieve tijd wordt nextindex opgehoogd van 1 naar 5, maar moet 7.
--> Rooster::update() herschreven
[ ] 20200114: toegevoegen Backup-naar-RTCFlash, Restore-van-RTCFlash, Clear-OnboardFlash, 
Voorgedefinieerd Rooster
*/

/* ====== INCLUDES ====== */
#include <Belautomaat_menu.h>

/* -------- test -------- */
/* ---------- EDITOR PROVING GROUNDS -------------- */
/*------------------ MAIN ---------------------*/

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void planner_callbackfunc(void) { 
  static unsigned long idler_wake_timer;
  static mainState_t lastState = msMENU;
  int rooster, uitgang;
  timer.update();
  if (millis() < idler_wake_timer) {
    return;
  }
  idler_wake_timer = millis() + UPDATE_INTERVAL_MS; 
  // lees mp3player status 
  mp3DiscardStatus();
  // haal tijd op
  get_time();
  // toon tijd op 7SEG
  sevenseg_out(rtchour, rtcminute, !(((millis() / 200) % 5) != 0));
  // controleer op actief signaal
  rooster = r.update(rtchour, rtcminute, rtcsecond, rtcweekday);
  if (rooster != INFO_NONE_ACTIVE) {
    uitgang = r.haaluitgang(rooster);
    triggerUitgang(uitgang); 
  }

  // Display uitschakelen na vertragen (auto-off)
  if (mainState == msMENU) { // exit menu indien in menu
    if (millis() > time_to_exit_menu) {
      menu.goRoot();
      handleMenu(BACK); 
      if (menu.isExited()) { 
        mainState = msIDLE;
      }
      lcd.noBacklight();      // LCD achtergrondlamp uit
      lcd.clear();
    }
  }
  // menu idle display
  if (mainState == msIDLE) {
    if (lastState != msIDLE) {
      lcd.clear();
      lastState = msIDLE;
    }
    lcd.setCursor(0, 0);
    print_tijd(rtchour, rtcminute, rtcsecond);
    lcd.print(' ');
    lcd.print(r.getNextIndex());
    lcd.print('/');
    lcd.print(r.numtimtab);    
    lcd.print(' ');
    lcd.print(' ');
    lcd.setCursor(0, 1);
    print_datum(rtcyear, rtcmonth, rtcday, rtcweekday);
  }
}

void UI_RoosterBackup(){
  lcd.clear();
  lcd.print(F("Rooster backup"));
  lcd.setCursor(0,1);
  lcd.print(F("Zeker?"));
  if (SELECT == waitforkey()) {     // bevestig 
    waitforkey();    
  }
  lcd.clear();
}
  
void UI_RoosterHerstellen(){
  lcd.clear();
  lcd.print(F("Rooster herstel"));
  lcd.setCursor(0,1);
  lcd.print(F("Zeker?"));
  if (SELECT == waitforkey()) {     // bevestig 
    waitforkey();    
  }
  lcd.clear();
}
  
void UI_RoosterLeegmaken(){
  lcd.clear();
  lcd.print(F("Leeg rooster"));
  lcd.setCursor(0,1);
  lcd.print(F("Zeker?"));
  if (SELECT == waitforkey()) {     // bevestig 
    r.schoonRooster();
    lcd.clear();
    lcd.print(F("Rooster is leeg"));
    waitforkey();    
  }
  lcd.clear();
}

/*======== Rooster uit blik =========*/
int invoegrooster, invoegdow;
void tijd(int hour, int minute) {
  r.signaalInvoegen(hour, minute, invoegdow, invoegrooster);    
  lcd.setCursor(0, 0);
  print_2digit(invoegrooster);  
  lcd.setCursor(0, 1);
  print_2digit(hour);
  lcd.print(":");
  print_2digit(minute);
  lcd.print(" ");
  LCD_Printweekdag(invoegdow);
  delay(100);  
}
void rooster50minuten(int rooster, int dow) {
  invoegrooster = rooster;
  invoegdow = dow;
  tijd(8,15);
  tijd(8,17);
  tijd(8,20);
  tijd(9,10);
  tijd(10,00);
  tijd(10,15);
  tijd(10,17);
  tijd(10,20);
  tijd(11,10);
  tijd(12,00);
  tijd(12,25);
  tijd(12,27);
  tijd(12,30);
  tijd(13,20);
  tijd(14,10);
  tijd(14,20);
  tijd(14,22);
  tijd(14,25);
  tijd(15,15);
  tijd(16,05);    // 20 signalen
}
void rooster45minuten(int rooster, int dow) {
  invoegrooster = rooster;
  invoegdow = dow;
  tijd(8,15);
  tijd(8,17);
  tijd(8,20);
  tijd(9,05);
  tijd(9,50);
  tijd(10,05);
  tijd(10,07);
  tijd(10,10);
  tijd(10,55);
  tijd(11,40);
  tijd(12,05);
  tijd(12,07);
  tijd(12,10);
  tijd(12,55);
  tijd(13,40);
  tijd(14,25);   // 16 signalen
}
void rooster40minuten(int rooster, int dow) {
  invoegrooster = rooster;
  invoegdow = dow;
  tijd(8,15);
  tijd(8,17);
  tijd(8,20);
  tijd(9,00);
  tijd(9,40);
  tijd(9,55);
  tijd(9,57);
  tijd(10,00);
  tijd(10,40);
  tijd(11,20);
  tijd(12,00);
  tijd(12,25);
  tijd(12,27);
  tijd(12,30);
  tijd(13,10);
  tijd(13,50);    // 16 signalen
}
void rooster10minuten(int rooster, int dow) {
  invoegrooster = rooster;
  invoegdow = dow;
  int uur = 18;
  int minuut = 30;
  for (int t = 0; t < 25; t++) {
    tijd(uur, minuut);
    minuut += 10;
    if (minuut >= 60) { 
      uur += 1;
      minuut -= 60;
    }
  }
}
void rooster15minuten(int rooster, int dow) {
  invoegrooster = rooster;
  invoegdow = dow;
  int uur = 18;
  int minuut = 30;
  for (int t = 0; t < 17; t++) {
    tijd(uur, minuut);
    minuut += 15;
    if (minuut >= 60) { 
      uur += 1;
      minuut -= 60;
    }
  }
}

#define HELEWEEK B1111100
#define MADIWOVR B1110100
#define DONDERDAG B0001000

void UI_RoosterUitblik(){
  lcd.clear();
  lcd.print(F("Rooster Uit blik"));
  lcd.setCursor(0,1);
  lcd.print(F("?"));
  if (SELECT == waitforkey()) {     // bevestig 
    lcd.clear();
    lcd.print(F("Leegmaken..."));
    r.schoonRooster();
    lcd.clear();
    
    r.zetnaam(0, "40 min");
    rooster40minuten(0, HELEWEEK);
    
    r.zetnaam(1, "45 min");
    rooster45minuten(1, HELEWEEK);
    
    r.zetnaam(2, "50 min");
    rooster50minuten(2, HELEWEEK);
    
    r.zetnaam(3, "50 MDW-V");
    rooster50minuten(3, MADIWOVR);

    r.zetnaam(4, "45 Dond");
    rooster45minuten(4, DONDERDAG);

    lcd.clear();
    lcd.print(F("Rooster uit blik"));
    waitforkey();    
  }
  lcd.clear();
}

/*===== MENU FUNCTIES TABEL =====
  Bepaalt de actie die volgt op elke menukeuze.
  Definieer {MenuID, functionptr}
  Laatste menuingang heeft menuID -1
*/
menuingang_t menudata[] = {
  {M_R_ACTIVEER,  UI_edit_schakelaar},
  {M_R_BEWERK,    UI_bladerRooster2},
  {M_R_BACKUP,    UI_RoosterBackup},
  {M_R_RESTORE,   UI_RoosterHerstellen},
  {M_R_CLEAR,     UI_RoosterLeegmaken},
  {M_R_UITBLIK,   UI_RoosterUitblik},
  {M_T_HANDREL,   UI_handbedienRelais},
  {M_G_VOLUME,    UI_setVolume},
  {M_G_BLADER,    UI_bladerGeluiden},
  {M_INS_DATUM,   UI_setDate},
  {M_INS_TIJD,    UI_setTime},
  {M_INS_SIGLEN,  UI_EditSignaallengte},
  {M_INS_REBOOT,  resetFunc},
  {M_INF_VERSIE,  UI_toonVersie},
  {M_INF_DATUM,   UI_toondatumtijd},
  {M_TOETSSLOT,   UI_toetsenbordslot},
  {-1, NULL}
};

void setup() {
  Serial.begin(9600);
  DPRINTLN(F("DPRINT actief!"));
  setup_belautomaat_lib();
  belautomaat_idler = planner_callbackfunc;
  setup_belautomaat_menu();
  light_lcd();
/*
  lcd.clear();
  lcd.print(F("Belautomaat"));
  lcd.setCursor(0, 1);
  lcd.print(F("Paul Wiegmans"));  
  delay(1000);
*/

  lcd.clear();
  lcd.print(F("signaallen "));  
  lcd.print(get_signaallengte());
  lcd.setCursor(0, 1);
  lcd.print(F("mp3volume "));  
  lcd.print(get_mp3volume());
  delay(500);
  //UI_toetsenbordslot();
}


void executeMenuAction(int id)
{
  int t; 
  for (t = 0; menudata[t].id > -1; t++) {   // zoek in tabel bijbehorende actie
    if (menu.getId() == menudata[t].id) {   // is gekozen?
      menudata[t].action();                 // dan actie uitvoeren
      break;
    }
  }
  if (menudata[t].id == -1) {   // indien geen actie gekozen
    mp3play(54);                // speel geluid
    //toon_menuresultaat();       // toon menukeuze 
  }
}

void adjustclock(int y, int mon, int d, int h, int m, int s) {
  DateTime adjustedtime(y, mon, d, h, m, s);
  rtc.adjust(adjustedtime);   
  get_time(); 
  r.findNextIndex(rtchour, rtcminute);
}
/*
  Consistente debugtoetsen:
  3: seconde 57
  8: tijd 8:14:55
  9: volgend uur
  0: reset datum en tijd naar di 1 okt 2019 08:14:55
  #: volgende dag, 8:00
*/
 
void loop () {
  char c = getkb();
  switch (mainState) {
    case msIDLE:
      if (c == SELECT) {    // OK opent menu
        menu.goRoot();
        mainState = msMENU;
        handleMenu('0');
/* voor debuggen        
      } else if (c == '3') {
        DateTime adjustedtime(rtcyear, rtcmonth, rtcday, rtchour, rtcminute, 59);
        rtc.adjust(adjustedtime);   
        r.findNextIndex(rtchour, rtcminute);
      } else if (c == '8') {
        adjustclock(rtcyear, rtcmonth, rtcday, 8, 14, 57);
      } else if (c == '9') {
        adjustclock(rtcyear, rtcmonth, rtcday, rtchour, 59, 59);
      } else if (c == '0') {
        adjustclock(2019, 10, 1, 8, 14, 57);
      } else if (c == '#') {
        adjustclock(rtcyear, rtcmonth, rtcday + 1, 8, 0, 0);
*/        
      }
      break;
    case msMENU:
      handleMenu(c);        // navigeer door menu
      if (menu.isExited()) {  // breek af
        mainState = msIDLE;
        lcd.clear();
        //toon_menuresultaat();
      }
      if (menu.isUsed()) {  // menu keuze gemaakt
        mainState = msACTION;
        executeMenuAction(menu.getId());
        lcd.clear();
        mainState = msIDLE;
      }      
  }
}
