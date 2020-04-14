/*
	Belautomaat_menu.h 

	Paul Wiegmans <p.wiegmans@svok.nl>
*/

#ifndef Belautomaat_menu_h
#define Belautomaat_menu_h

#include "Arduino.h"
#include <PWmenu_LCD.h>
//#include <Belautomaat_lib.h>


//  definitie van menuid's
#define M_ROOSTER     10
#define M_R_BEWERK    11
#define M_R_ACTIVEER  12
#define M_R_BACKUP    13
#define M_R_RESTORE   14
#define M_R_CLEAR     15
#define M_R_UITBLIK   16

#define M_TEST        20
#define M_T_HANDREL   21

#define M_GELUID      30
#define M_G_VOLUME    31
#define M_G_BLADER    32

#define M_INSTELLING  40
#define M_INS_DATUM   41
#define M_INS_TIJD    42
#define M_INS_SIGLEN  44
#define M_INS_REBOOT  45

#define M_INFORMATIE  50
#define M_INF_VERSIE  51
#define M_INF_DATUM   52

#define M_TOETSSLOT   60

typedef struct { 
    int id;
    void (*action)();
  } menuingang_t;


extern PWmenu menu;
extern void handleMenu(char c);
extern void setup_belautomaat_menu();

#endif // #ifndef Belautomaat_lib_h

