/*
	Belautomaat_menu.cpp 
	Paul Wiegmans <p.wiegmans@svok.nl>

  Bibliotheek voor de Belautomaat menu.
  
  Historie
	20181111 eerste versie 
  20190104
	20200414 

	Vermeld in je code:
	#include <Belautomaat_menu.h>

  Vermeld in je setup-functie:
  setup_belautomaat_menu();
 
*/

#include "Belautomaat_menu.h"

//=============== MENU SETUP ==============================
/*
 * Menudefinitie:
 * 1. definieer unieke Menu-ID in de Belautomaat_menu.h,
 * 2. definieer een menuitem voor elk menu hieronder,
 * 3. definieer menustructuur in menuinit(),
 * 4. Definieer in tabel menudata een functie voor elk menuID in hoofdprogramma. 
 */

/* Definitie van menu items en weergavetekst */
MenuItem miRooster(     M_ROOSTER,    "Roosters");
MenuItem miRBewerk(     M_R_BEWERK,   "Bewerk");
MenuItem miRActiveren(  M_R_ACTIVEER, "Activeren");
MenuItem miRBackup(     M_R_BACKUP,   "Backup");
MenuItem miRRestore(    M_R_RESTORE,  "Herstellen");
MenuItem miRClear(      M_R_CLEAR,    "Leegmaken");
MenuItem miRUitblik(    M_R_UITBLIK,  "Uit blik");

MenuItem miTest(        M_TEST,       "Testfuncties");
MenuItem miTHandRel(    M_T_HANDREL,  "Bedien relais");

MenuItem miGeluid(      M_GELUID,     "Geluid");
MenuItem miGVolume(     M_G_VOLUME,   "Volume");
MenuItem miGBlader(     M_G_BLADER,   "Blader");

MenuItem miInstelling(  M_INSTELLING, "Instelling");
MenuItem miInsDatum(    M_INS_DATUM,  "Datum");
MenuItem miInsTijd(     M_INS_TIJD,   "Tijd");
MenuItem miInsSiglen(   M_INS_SIGLEN, "Signaallengte");
MenuItem miInsReboot(   M_INS_REBOOT, "Herstart");

MenuItem miInformatie(  M_INFORMATIE, "Informatie");
MenuItem miInfVersie(   M_INF_VERSIE, "Versie");
MenuItem miInfDatum(    M_INF_DATUM, "Datum en tijd");

MenuItem miToetsslot(   M_TOETSSLOT, "Vergrendelen");

/* miRooster is root van menu */
PWmenu menu(miRooster);  

/* Definitie van menustructuur*/
void menuinit() {

  miRooster.addChild(miRBewerk);
  miRooster.addChild(miRActiveren);
  miRooster.addChild(miRBackup);
  miRooster.addChild(miRRestore);
  miRooster.addChild(miRClear);
  miRooster.addChild(miRUitblik); 

  miRooster.addSibling(miTest);
  miTest.addChild(miTHandRel);

  miRooster.addSibling(miGeluid);
  miGeluid.addChild(miGVolume);
  miGeluid.addChild(miGBlader);

  miRooster.addSibling(miInstelling);
  miInstelling.addChild(miInsDatum);
  miInstelling.addChild(miInsTijd); 
  miInstelling.addChild(miInsSiglen);  
  miInstelling.addChild(miInsReboot);  

  miRooster.addSibling(miInformatie);
  miInformatie.addChild(miInfVersie);
  miInformatie.addChild(miInfDatum);
  
  miRooster.addSibling(miToetsslot);
}

//-------------------------------------------------------

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

/*
  definieer een array van menudata als volgt:
menuingang_t menudata[] = {
  {M_TESTVDE, testVed},
  {M_RELAIS1, bedienRelais1},
  {M_TESTTDE, testTde},
  {M_EDITTDE, setTimeTde},
  {M_I_TIJD, setTimeVed},
  {M_G_VOLUME, setVolume},
  {M_G_BLADER, bladerGeluiden},
  {M_INF_VERSIE, toonVersion},
  {-1, NULL}
};

in main loop:
      handleMenu(c);
      if (menu.isExited()) {
        mainState = msIDLE;
        toon_menuresultaat();
      }
      if (menu.isUsed()) {
        int t=0;
        for (t = 0; menudata[t].id != -1; t++) {
          if (menu.getId() == menudata[t].id) {
            menudata[t].action();
            break;
          }
        }
        if (menudata[t].id == -1) {   // indien niet in menudata...
          mp3play(54);
          toon_menuresultaat();       // dan menuid tonen
        }
        mainState = msIDLE;
*/

/*---------------------------- Belautomaat Algemeen ----------------------------*/
void setup_belautomaat_menu() {
  menu.setViewSize(2, 16);
  menuinit();
}

