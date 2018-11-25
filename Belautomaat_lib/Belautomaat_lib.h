/*
	Belautomaat-lib.h 
	20181111
	Paul Wiegmans <p.wiegmans@bonhoeffer.nl>
*/

#ifndef Belautomaat_lib_h
#define Belautomaat_lib_h

#include "Belautomaat_driver.h"
#include "belrooster.h"
#include "Timer.h"

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

/* ====== VARIABLES  ====== */
extern Rooster r;

/* ====== EXPORTED FUNCTIONS ====== */
extern boolean once_per_second();
extern t_voidfunc belautomaat_idler; 
/*--------------------------- Belautomaat Algemeen ---------------------------*/
extern void setup_belautomaat_lib();

/*-------------------------- TimeEdit ---------------------------*/
extern void print_2digits(int n);
extern bool is_number_char(char c);
extern bool timeEditor(int &hour, int &minute) ;

#endif // #ifndef Belautomaat_lib_h

