/*
belrooster.cpp 20180819 Paul Wiegmans

Belrooster is een bibliotheek voor het beheer van belautomaatsignalen in EEPROM
van de Arduino.

De tabel met signalen bestaat uit een
* rooster: array van Signaal_t in EEPROM
* timtab: een array van uint8_t in RAM met indexen naar array rooster. In timtab
  staan de indexen op tijd gesorteerd.

Een Signaal wordt gedefinieerd door de struct TimeRecord. Het bevat uur, minuut,
dow (dag van de week) en channel (kanaal). Hierbij bevat dow een bitmasker,
waarbij elke bit staat voor één dag van de week. Elk bit van dow is hoog, voor
de dag van de week waarop het schakelmoment actief is. Channel bepaalt welk van
de 4 relais geschakeld wordt, en biedt ruimte voor uitbreiding, zoals kanalen
voor geluidsignalen. (Welke van de 4 relais de bel schakelen, wordt bepaald door
elk van de 4 schakelaars die in serie met elk relais is verbonden. In een
toekomstige versie bevat Channel het nummer van het rooster, dat door de
beheerders in deze applicatie wordt gekozen. Voor nu zijn standaard alle rooster
actief en worden alle relais bekrachtigd. )

Timtab is een tabel van indexen naar de array rooster, dus indexen naar signalen
in EEPROM. De indexen kunnen worden vertaald naar een adres in EEPROM door de
functie index2adres(). De Arduino Mega heeft 4 KB EEPROM en 8 KB RAM. Er is nu
gekozen voor een grootte van de tabel rooster van 512 signalen groot, en timtab
is int[512] groot. De applicatie beheert de array rooster. Of plaatsen in EEPROM
al dan niet in gebruik zijn, wordt uitsluitend bepaald of de index ervan in
array timtab voorkomt of niet, maar ook of de geheugenplaats EMPTY bevat. Staat
een struct Signaal_t in EEPROM leeg, dan staat deze niet in timtab. Andersom
staat een index niet in timtab, dan is deze per definitie leeg en vrij om te
gebruiken. Deze administratie wordt automatisch beheerd, door gebruik te maken
van functies signaalWissenEeprom() en signaalInvoegen(). Voor nu is er ook voor
gekozen om in lege geheugenplaatsen EMPTY te schrijven, om aan te duiden dat
deze leeg zijn. 
*/
/*
EEPROM geheugen indeling ziet er dan als volgt uit: 
    0..14:      unieke marker "Belautomaat v1"  
                (15 bytes lang inclusief afsluitende nul) 
    15..63:     niet in gebruik 
    64..EIND:   roostertabel met plaats voor 512 signalen van elk 4
                bytes groot. 
*/

#include "belrooster.h"
#include "arduino.h"

const char markerString[] = "Belautomaat v1";

Rooster::Rooster(void)
{
    // constructor voor class Timetable
    numtimtab = 0;
    nextindex = 0;
    if (!testEepromMarker())
    {
        schoonRooster();
    }
    initTimtab();
    lastFreeEeprom = EEPROM_ROOSTER_START_ADRES;
}
void Rooster::schoonRooster(void)
{
    // initialiseer variabelen, vul EEPROM met markerstring en vul verder met
    // EMPTY waarden.
    Serial.println("! schoon rooster");
    int adres, index;
    for (adres = 0, index = 0; markerString[index] != 0; adres++, index++)
    {
        EEPROM[adres] = markerString[index]; 
        // kopieer marker naar EEPROM, inclusief nullchar
    }
    for (; adres < EEPROM.length(); adres++)
    {
        EEPROM[adres] = EMPTY; // vul de rest met EMPTY
    }
    numtimtab = 0; // nul indexen in timtab
}
 
int Rooster::vergelijkSignaal(Signaal_t &bm1, Signaal_t &bm2)
{
    // vergelijk twee signalen. Alleen uur, minuut en kanaal worden vergeleken.
    // geeft 1 indien bm1 > bm2
    // geeft 0 indien bm1 == bm2 (d.w.z. hour, minute en dayOfWeek zijn gelijk)
    // geeft -1 indien bm1 < bm2
    // http://www.cplusplus.com/reference/cstdlib/qsort/
    if (bm1.hour > bm2.hour)
        return 1;
    if (bm1.hour < bm2.hour)
        return -1;
    if (bm1.minute > bm2.minute)
        return 1;
    if (bm1.minute < bm2.minute)
        return -1;
    if (bm1.channel > bm2.channel)
        return 1;
    if (bm1.channel < bm2.channel)
        return -1;
    return 0;
}

int Rooster::vergelijkSignaalEeprom(int eeidx1, int eeidx2)
{
    // vergelijk 2 signalen, gegeven door indexen in timtab.
    Signaal_t st1, st2;
    laadSignaal(eeidx1, st1);
    laadSignaal(eeidx2, st2);
    return vergelijkSignaal(st1, st2);
}
void Rooster::sorteerTimtab(void) {
    // nu timtab sorteren: bubblesort van 
    // http://hwhacks.com/2016/05/03/bubble-sorting-with-an-arduinoc-application/
    for (int i = 0; i < (numtimtab - 1); i++)
    {
        for (int o = 0; o < (numtimtab - (i + 1)); o++)
        {
            int result = vergelijkSignaalEeprom(timtab[o], timtab[o + 1]);
            if (result > 0)
            {
                int t = timtab[o];
                timtab[o] = timtab[o + 1];
                timtab[o + 1] = t;
            }
        }
    }
}
void Rooster::initTimtab(void)
{
    // vul timtab door Signalen in EEPROM te tellen, sorteer.
    int ttindex = 0;
    Signaal_t bm;
    for (int eeindex = 0; eeindex < ROOSTERSIZE; eeindex++)
    {
        laadSignaal(eeindex, bm);
        if (bm.hour != EMPTY)
        {
            timtab[ttindex++] = eeindex;
        }
    }
    numtimtab = ttindex;
    sorteerTimtab();
}
bool Rooster::testEepromMarker(void)
{
    // geef true wanneer marker is gevonden in EEPROM adres 0
    for (int index = 0; markerString[index] != 0; index++)
    {
        if (EEPROM[index] != markerString[index])
        {
            return false;
        }
    }
    return true; // Marker in EEPROM gevonden
}
int Rooster::index2adres(int eeindex)
{
    // reken index om naar adres in EEPROM
    return EEPROM_ROOSTER_START_ADRES + sizeof(Signaal_t) * eeindex;
}
int Rooster::adres2index(int adres)
{
    // reken adres om naar index in EEPROM
    return (adres - EEPROM_ROOSTER_START_ADRES) / sizeof(Signaal_t);
}
void Rooster::laadSignaal(int eeindex, Signaal_t &bm)
{
    // lees Signaal met index
    EEPROM.get(index2adres(eeindex), bm);
}
void Rooster::bergopSignaal(int eeindex, Signaal_t &bm)
{
    // schrijf Signaal met index
    int adres = index2adres(eeindex);
    EEPROM.put(adres, bm);
}
void Rooster::randomSignaal(Signaal_t &bm)
{
    bm.hour = random(0, 24);
    bm.minute = random(0, 60);
    bm.dayOfWeek = random(0, 128);
    bm.channel = random(0, 8);
}

int Rooster::vindVrijeEeindex(void)
{
    // vind eerste vrije plek in EEPROM, retourneer index in EEPROM
    // Begin vanaf laatst gevonden vrije plaats, en ga rond naar 
    // beginadres van rooster. 
    int startadres = lastFreeEeprom;
    int adres = lastFreeEeprom + sizeof(Signaal_t);
    uint8_t value;
    value = EEPROM.read(adres);
    while (value != EMPTY)
    {
        adres += sizeof(Signaal_t) ;
        if (adres > EEPROM.length()) {
            adres = EEPROM_ROOSTER_START_ADRES;
        }
        if (adres == startadres) {      // zijn we al rond geweest?
            return ERROR_EEPROM_FULL;   // stop met foutmelding
        }
        value = EEPROM.read(adres);
    }
    lastFreeEeprom = adres;
    int eeindex = adres2index(adres);
    Serial.print("  vindVrijeEeindex:");  Serial.println(eeindex);
    return eeindex;
}

void Rooster::signaalWissen(int ttindex)
{
    // verwijder Signaal uit timtab, behoud sortering
    // wis ingang in timtab, wis record in eeprom, tellers bijwerken
    Serial.print("> signaalWissen...");
    Serial.println(ttindex);
    
    int index;
    if (numtimtab <= 0)
        return; // geen signalen
    if (ttindex >= numtimtab || ttindex < 0)
        return; // ingang bestaat niet
    int eeindex = timtab[ttindex];
    Signaal_t bm = {EMPTY, EMPTY, EMPTY, EMPTY};
    bergopSignaal(eeindex, bm);
    for (index = ttindex; index < numtimtab - 1; index++)
    {
        timtab[index] = timtab[index + 1];
    }
    numtimtab--;
    if (nextindex > index)
    {
        nextindex--;
    }
    //printRooster();
}

bool Rooster::binairZoeken(Signaal_t &bm, int &piInsert)
{
    /*()
   Signaal_t &bm,  // gezochte Signaal
   int* piInsert // te retourneren invoegpositie
   )
   /* Deze methode gebruikt bisectie: deelt het te doorzoeken stuk
    * steeds in tweeën en beoordeelt steeds een zijde
    * van het nieuwe snijvlak. */
    int iL = 0;
    int iR = numtimtab;
    int nSpan = iR - iL;
    bool bSearch = (nSpan > 0);
    int iN = iL + nSpan / 2;
    bool bFound = false;
    while (bSearch)
    {
        Signaal_t sN;
        int eeindex = timtab[iN];
        laadSignaal(eeindex, sN);
        int nCmp = vergelijkSignaal(sN, bm);
        if (nCmp > 0)
        {
            /* gezochte string zit in of net rechts naast het rechter stuk */
            iL = iN;
        }
        else if (nCmp < 0)
        {
            /* gezochte string zit in of net links naast het linker stuk */
            iR = iN;
        }
        else
        {
            bFound = true;
            break;
        }
        if (nSpan == 1)
        {
            if (nCmp > 0)
            {
                iN = iR;
            }
            else
            {
                iN = iL;
            }
            break; // niet gevonden; invoegpositie wel
        }
        nSpan = iR - iL;
        iN = iL + nSpan / 2;
    }              // einde zoeklus
    piInsert = iN; // invoegpositie of positie van de gevonden string
    return bFound; // indien false: iN is de invoegpositie, anders de positie van de gevonden string
}

bool Rooster::simpelZoeken(Signaal_t &bm, int &matchpos) {
    // Zoek in timtab naar de eerste index van een signaal die gelijk of
    // groter is dan bm. Gelijk wil hier zeggen: signaal met zelfde hour, minute 
    // en dayOfWeek. Geeft true wanneer gelijk signaal is gevonden, en false
    // wanneer niet. Matchpos bevat index in timtab van gelijk signaal, of het
    // eerste signaal dat groter is. 
    // geeft 1 indien bm1 > bm2
    // geeft 0 indien bm1 == bm2 (d.w.z. hour, minute en dayOfWeek zijn gelijk)
    // geeft -1 indien bm1 < bm2
    Signaal_t sigcomp;
    bool gevonden = false;
    int t, result;
    for (t = 0; t < numtimtab; t++) {
        laadSignaal(timtab[t], sigcomp);
        result = vergelijkSignaal(bm, sigcomp);
        matchpos = t;
        if (result == 0) {
            return true;
        }
        if (result < 0) {
            return false;
        }
    }
    matchpos = numtimtab;
    return false;
}

int Rooster::signaalInvoegen(Signaal_t &bm)
{
    // strategie: zoek een gelijke in tabel (via binair zoeken)
    // wanneer gelijke gevonden, tel weekdag-bitmaskers bij elkaar 
    // op en gooi weg.
    // wanneer gelijke niet gevonden, voeg in tabel in op index
    // retourneer index in timtab

    //char buf[30];  Serial.print("> signaalInvoegen...");
    //printSignaal(buf, bm); Serial.println(buf);

    // speciaal: er zijn 0 records 
    if (numtimtab == 0)
    {
        int eeindex = vindVrijeEeindex(); // zoek eerste vrij record in EEPROM
        bergopSignaal(eeindex, bm);
        timtab[0] = eeindex;
        numtimtab = 1;
        //printRooster();
        return 0;
    }

    int matchpos;
    // Kies hier één van de twee mogelijke zoekfuncties
    //bool gevonden = binairZoeken(bm, matchpos);
    bool gevonden = simpelZoeken(bm, matchpos);

    // Hier weten op welke index het nieuwe signaal moet worden ingevoegd 
    if (gevonden)
    {
        // Er bestaat al een Signaal met zelfde tijd en kanaal. 
        // Voeg samen en bewaar.
        Signaal_t temp;
        laadSignaal(timtab[matchpos], temp);
        bm.dayOfWeek = bm.dayOfWeek | temp.dayOfWeek; // bitwise OR dayOfWeek
        bergopSignaal(timtab[matchpos], bm);
        return WARN_RECORD_MERGED;
    }
    // Signaal verschilt; moet toegevoegd worden.
    // is er nog ruimte in rooster (EEPROM) of timtab?
    if (numtimtab >= ROOSTERSIZE)
    {
        return ERROR_TIMTAB_FULL; // toevoegen mislukt; Timtab is vol
    }
    int eeindex = vindVrijeEeindex(); // zoek eerste vrij record in EEPROM
    if (eeindex == ERROR_EEPROM_FULL)
    {
        return ERROR_EEPROM_FULL; // tovoegen mislukt; EEPROM is vol
    }
    bergopSignaal(eeindex, bm);
    // We schuiven alles vanaf matchpos 1 plaats op  
    
    //Serial.print("  Zoeken..");
    //Serial.print("  gevonden:");  Serial.print(gevonden);
    //Serial.print("  matchpos:");  Serial.print(matchpos);
    //Serial.print("  numtimtab:"); Serial.println(numtimtab);  
    int t;
    t = numtimtab - 1;
    while (t >= matchpos) {
        timtab[t + 1] = timtab[t];
        t--;
    }
    timtab[matchpos] = eeindex;
    numtimtab++;

    //Serial.print("  Ingevoegd in timtab index: "); Serial.print(matchpos);
    //printRooster();
    return matchpos; // alles OK
}

int Rooster::signaalInvoegen(int hour, int minute, int dayOfWeek, int channel)
{
    Signaal_t bm;
    bm.hour = hour;
    bm.minute = minute;
    bm.dayOfWeek = dayOfWeek;
    bm.channel = channel;
    return signaalInvoegen(bm);
}

char *Rooster::printDayOfWeek2(char *buffer, int dayOfWeek)
{
    // print dag van de week voor elke bit in bitveld.
    // buffer moet minimaal 15 tekens lang zijn.
    const char dowNames[] = {"MaDiWoDoVrZaZo"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++)
    {
        if (dayOfWeek & masker)
        {
            buffer[index] = dowNames[index];
            index++;
            buffer[index] = dowNames[index];
            index++;
        }
        else
        {
            buffer[index++] = '_';
            buffer[index++] = '_';
        }
        masker >>= 1;
    }
    buffer[index] = 0; // string terminator
    return buffer;
}
char *Rooster::printDayOfWeek1(char *buffer, int dayOfWeek)
{
    // print dag van de week voor elke bit in bitveld.
    // buffer moet minimaal 15 tekens lang zijn.
    const char dowNames[] = {"MDWDVZZ"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++)
    {
        if (dayOfWeek & masker)
        {
            buffer[index] = dowNames[index];
            index++;
        }
        else
        {
            buffer[index++] = '_';
        }
        masker >>= 1;
    }
    buffer[index] = 0; // string terminator
    return buffer;
}

char *Rooster::print2decimals(char *buffer, int value)
{
    // print a number with 2 decimals to stringbuffer
    buffer[0] = '0' + ((value / 10) % 10);
    buffer[1] = '0' + (value % 10);
    buffer[2] = 0;
    if (value > 99 || value < 0)
    {
        buffer[0] = '!'; // dit beduidt under/overflow
    }
    return buffer;
}
char *Rooster::printndecimals(char *buffer, int value, int numberofchars)
{
    // print a number with 2 decimals to stringbuffer
    int t;
    for (t = numberofchars - 1; t >= 0; t--)
    {
        buffer[t] = '0' + (value % 10);
        value = value / 10;
    }
    if (value > 0)
    {
        buffer[0] = '!'; 
        // waarde is te hoog voor weergave in gegeven aantal tekens.
    }
    buffer[numberofchars] = 0;
    return buffer;
}

char *Rooster::printHex(char *buffer, int num) {
    char hex[] = "0123456789ABCDEF";
    buffer[1] = hex[num % 16];
    num >>= 4;
    buffer[0] = hex[num % 16];
    buffer[2] = 0;
    return buffer;
}

char *Rooster::printSignaal(char *buffer, Signaal_t &bm)
{
    // retourneer stringweergave van Signaal
    // buffer wijst naar string of char array van minstens .. tekens.
    print2decimals(&buffer[0], bm.hour); // 0 en 1
    buffer[2] = ':';
    print2decimals(&buffer[3], bm.minute); // 3 en 4
    buffer[5] = ':';
    printDayOfWeek1(&buffer[6], bm.dayOfWeek);
    int a = strlen(buffer);
    buffer[a] = '#';
    print2decimals(&buffer[a + 1], bm.channel); // 0 en 1
    // tekst is te lang voor LCD !
}

void Rooster::printRoosterRegel(int index) 
{
    char buf[25];
    Signaal_t bm;
    int adres, eeindex;    
    eeindex = timtab[index];
    adres = index2adres(eeindex);
    laadSignaal(eeindex, bm);

    // print timtab index en waarde en omgerekend adres
    sprintf(buf, "  t[%.3d]=E%.3d (@%.4d) ", index, eeindex,adres);
    Serial.print(buf);

    // print Signaal
    printSignaal(buf, bm);
    Serial.print(buf);

    // EEPROM dump
    Serial.print(" [");
    for (int t = 0; t < 4; t++)
    {
        printHex(buf, EEPROM[adres + t]);
        Serial.print(buf);
        Serial.print(" ");
    }
    Serial.println("]");

}

void Rooster::printRooster()
{
    char buf[25];
    Serial.print(F("  numtimtab:"));
    Serial.print(numtimtab);
    Serial.print(F("  nextindex:"));
    Serial.println(nextindex);
    for (int index = 0; index < numtimtab; index++)
    {
        printRoosterRegel(index);
    }
}

uint8_t Rooster::update(int hour, int minute, int dow)
{
    // update test of volgende Signaal actief is, en retourneert channel,
    // verhoogt nextindex
    Signaal_t bm;
    laadSignaal(timtab[nextindex], bm);
    if (hour == bm.hour && minute == bm.minute && ((1 << dow) & bm.dayOfWeek))
    {
        nextindex = (nextindex + 1) % numtimtab;
        return bm.channel;
    }
    else
    {
        return INFO_NONE_ACTIVE; // geen signalen actief
    }
}

int Rooster::aantalSignalen() {
    return numtimtab;
}

int Rooster::volgendSignaal(int tindex, int kanaalfilter) {
    int i = tindex + 1;
    Signaal_t sig;
    while (i < numtimtab) {
        laadSignaal(timtab[i], sig);
        if (sig.channel == kanaalfilter) {
            return i;
        }
        i++;
    }
    return WARN_NOMORESIGNALS;
}

int Rooster::vorigSignaal(int tindex, int kanaalfilter) {
    int i = tindex - 1;
    Signaal_t sig;
    while (i >= 0) {
        laadSignaal(timtab[i], sig);
        if (sig.channel == kanaalfilter) {
            return i;
        }
    }
}

void Rooster::printStatus(int status) {
    // geef status in tekst
    Serial.print("= Status: ");
    Serial.print(status);
    Serial.print("  ");
    switch (status) {
        case ERROR_TIMTAB_FULL:
            Serial.println("Error: Timtab full");
            break;
        case ERROR_EEPROM_FULL:
            Serial.println("Error: EEPROM full");
            break;
        case INFO_NONE_ACTIVE:
            Serial.println("Info: None active");
            break;
        case WARN_RECORD_MERGED:
            Serial.println("Warning: record merged");
            break;
        default:
            Serial.println("Success");
            break;
    }
}
/*
int Rooster::vergelijkSignaalEeprom(int eeidx1, int eeidx2) {
    // vergelijk 2 signalen aangeduid met index
    Signaal_t bm1, bm2;
    laadSignaal(eeidx1, bm1);
    laadSignaal(eeidx2, bm2);
    return vergelijkSignaal(bm1, bm2);
}
*/
