/*
belrooster.cpp 
20180819 
Paul Wiegmans <p.wiegmans@svok.nl>

Belrooster is een bibliotheek voor het beheer van roosters en 
signaaltijden in de EEPROM van de Arduino.

De tabel met signalen bestaat uit een
* rooster: array van Signaal_t (4 bytes) in EEPROM
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
beheerders in deze applicatie wordt gekozen. Voor nu zijn standaard alle roosters
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

EEPROM geheugen indeling ziet er dan als volgt uit: 
    0..14:      unieke marker "Belautomaat v1"  
                (15 bytes lang inclusief afsluitende nul) 
    32..33:     mp3volume  (gedefinieerd in belautomaat_driver)
    34..35:     signaallengte  (gedefinieerd in belautomaat_driver)
    64..EIND:   roostertabel met plaats voor 512 signalen van elk 4
                bytes groot. 

Dat is toen. Dit is nu 20190427. Extra eisen:
* een array met schakelaars voor elk rooster, op te slaan in EEPROM.
* laten we zeggen dat we een maximum van 10 roosters hebben.
* een array met labels: één voor elk rooster, met ruimte voor 8 
alfanumeriek tekens, op te slaan in EEPROM.
* tabel met mappings kanaal->uitgang, op te slaan in EEPROM.
* Elk rooster wordt gekoppeld aan een uitgang: uitgangen 0..3 zijn relais,
4 of hoger is een specifiek geluid voor de MP3-speler.
* tabel met mappings uitgang->geluid
Later: 
* editfunctie voor het inschakelen van elk rooster afzonderlijk.
* editfunctie voor het koppelen van elk rooster aan een kanaal. 
* eventueel dit combineren tot editfunctie voor elk rooster schakelaar en uitgang.

Indeling EEPROM geheugen:
0..31:      ruimte voor markerstring
32..41:     Ruimte voor 6 variabelen: mp3volume, signaallengte, masterswitch
38..117:    MAXROOSTER*8 = 80 bytes, roosterlabels
118..127:   MAXROOSTER*1 byte = 10 bytes; uitgangen, array 
128..EOM:   n*4 bytes; rooster, array met signaaltijden. 

Bereken de hoeveelheid signaaltijden die in EEPROM kunnen worden 
opgeslagen als volgt: 
nSignaaltijden = (EEPROM-grootte - 124) / 4
Capaciteit voor AVR Mega 2560: (4096-124)/4=
*/

#include "belrooster.h"
#include "arduino.h"

const char markerString[] = MARKER_STRING; // 20 bytes incl eindnul
// als de marker niet in EEPROM wordt gevonden,
// wordt de EEPROM geinitialiseerd en dus gewist. 

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
    lastFreeEeprom = EEADR_ROOSTER;
    hoofdschakelaar = EEVarLoad(EEVAR_HOOFDS);
}
void Rooster::schoonRooster(void)
{
    // initialiseer variabelen, vul EEPROM met markerstring en vul verder met
    // EMPTY waarden.
    int adres, index;

    // kopieer marker naar EEPROM, inclusief nullchar
    for (adres = 0, index = 0; markerString[index] != 0; adres++, index++)
    {
        EEPROM[adres] = markerString[index]; 
    }
    // wis vars, namen en uitgangen 
    for (adres = EEADR_EEVAR; adres < EEADR_ROOSTER; adres++) {
        EEPROM[adres] = 0;
    }
    // wis rooster
    for (adres = EEADR_ROOSTER; adres < EEPROMSIZE; adres++)
    {
        EEPROM[adres] = EMPTY; // vul de rest met EMPTY
    }
    numtimtab = 0; // nul indexen in timtab
    nextindex;
}
 
int Rooster::vergelijkSignaal(Signaal_t &sa, Signaal_t &sb)
{
    // vergelijk twee signalen. Alleen uur, minuut en kanaal worden vergeleken.
    // geeft 1 indien sa > sb
    // geeft 0 indien sa == sb (d.w.z. hour, minute en dayOfWeek zijn gelijk)
    // geeft -1 indien sa < sb
    // http://www.cplusplus.com/reference/cstdlib/qsort/
    if (sa.hour > sb.hour)
        return 1;
    if (sa.hour < sb.hour)
        return -1;
    if (sa.minute > sb.minute)
        return 1;
    if (sa.minute < sb.minute)
        return -1;
    if (sa.dayOfWeek > sb.dayOfWeek)
        return 1;
    if (sa.dayOfWeek < sb.dayOfWeek)
        return -1;
    if (sa.channel > sb.channel)
        return 1;
    if (sa.channel < sb.channel)
        return -1;
    return 0;
}

int Rooster::vergelijkSignaalEeprom(int eeidx1, int eeidx2)
{
    // vergelijk 2 signalen, gegeven door indexen in timtab.
    Signaal_t st1, st2;
    leesSignaalEEPROM(eeidx1, st1);
    leesSignaalEEPROM(eeidx2, st2);
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
    int tijd = 0;
    Signaal_t bm;
    for (int eeindex = 0; eeindex < ROOSTERSIZE; eeindex++)
    {
        leesSignaalEEPROM(eeindex, bm);
        if (bm.hour != EMPTY)
        {
            timtab[tijd++] = eeindex;
        }
    }
    numtimtab = tijd;
    sorteerTimtab();
}
bool Rooster::testEepromMarker(void)
{
    // geef true terug, wanneer de markerstring is gevonden vanaf EEPROM[0]
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
    return EEADR_ROOSTER + sizeof(Signaal_t) * eeindex;
}
int Rooster::adres2index(int adres)
{
    // reken adres om naar index in EEPROM
    return (adres - EEADR_ROOSTER) / sizeof(Signaal_t);
}
void Rooster::leesSignaalEEPROM(int eeindex, Signaal_t &sig)
{
    // primitieve functie lees Signaal met index in EEPROM
    EEPROM.get(index2adres(eeindex), sig);
}
void Rooster::schrijfSignaalEEPROM(int eeindex, Signaal_t &sig)
{
    // primitieve functie schrijf Signaal met index in EEPROM
    int adres = index2adres(eeindex);
    EEPROM.put(adres, sig);
}
void Rooster::randomSignaal(Signaal_t &sig)
{
    sig.hour = random(0, 24);
    sig.minute = random(0, 60);
    sig.dayOfWeek = random(0, 128);
    sig.channel = random(0, 8);
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
            adres = EEADR_ROOSTER;
        }
        if (adres == startadres) {      // zijn we al rond geweest?
            return ERROR_EEPROM_FULL;   // stop met foutmelding
        }
        value = EEPROM.read(adres);
    }
    lastFreeEeprom = adres;
    int eeindex = adres2index(adres);
    //Serial.print("  vindVrijeEeindex:");  Serial.println(eeindex);
    return eeindex;
}

bool Rooster::binairZoeken(Signaal_t &sig, int &piInsert)
{
/* Deze functie zoekt de positie in de tabel met indexen waarop signaal {sig} 
moet worden ingevoegd om de tabel gesorteerde te houden.
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
        leesSignaalEEPROM(eeindex, sN);
        int nCmp = vergelijkSignaal(sN, sig);
        if (nCmp > 0) {
            /* gezochte string zit in of net rechts naast het rechter stuk */
            iL = iN;
        } else if (nCmp < 0) {
            /* gezochte string zit in of net links naast het linker stuk */
            iR = iN;
        } else {
            bFound = true;
            break;
        }
        if (nSpan == 1) {
            if (nCmp > 0) {
                iN = iR;
            } else {
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

void Rooster::signaalWissen(int tijd)
{
    // verwijder Signaal uit timtab, behoud sortering
    // wis ingang in timtab, wis record in eeprom, tellers bijwerken
    int index;
    if (numtimtab <= 0)
        return; // geen signalen
    if (tijd >= numtimtab || tijd < 0)
        return; // ingang bestaat niet
    int eeindex = timtab[tijd];
    Signaal_t bm = {EMPTY, EMPTY, EMPTY, EMPTY};
    schrijfSignaalEEPROM(eeindex, bm);
    for (index = tijd; index < numtimtab - 1; index++)
    {
        timtab[index] = timtab[index + 1];
    }
    numtimtab--;
    if (nextindex > index)
    {
        nextindex--;
    }
}

bool Rooster::simpelZoeken(Signaal_t &sig, int &matchpos) {
    // Zoek in timtab naar de eerste index van een signaal die gelijk of
    // groter is dan sig. Gelijk wil hier zeggen: signaal met zelfde hour, minute 
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
        leesSignaalEEPROM(timtab[t], sigcomp);
        result = vergelijkSignaal(sig, sigcomp);
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

int Rooster::signaalInvoegen(Signaal_t &sig)
{
    // signaal invoegen; retourneer foutmelding of 0 bij succes
    // strategie: zoek een gelijke in tabel (via binair zoeken)
    // wanneer gelijke gevonden, tel weekdag-bitmaskers bij elkaar 
    // op en gooi weg.
    // wanneer gelijke niet gevonden, voeg in tabel in op index
    // retourneer index in timtab

    // speciaal geval: er zijn 0 records 
    if (numtimtab == 0)
    {
        int eeindex = vindVrijeEeindex(); // zoek eerste vrij record in EEPROM
        schrijfSignaalEEPROM(eeindex, sig);
        timtab[0] = eeindex;
        numtimtab = 1;
        //printRooster();
        return 0;
    }

    int matchpos;
    // Kies hier één van de twee mogelijke zoekfuncties
    //bool gevonden = binairZoeken(bm, matchpos);
    bool gevonden = simpelZoeken(sig, matchpos);

    // Hier weten we op welke index het nieuwe signaal moet worden ingevoegd 
    if (gevonden)
    {
        // Er bestaat al een Signaal met zelfde tijd en kanaal. 
        // Voeg samen en bewaar.
        Signaal_t temp;
        leesSignaalEEPROM(timtab[matchpos], temp);
        sig.dayOfWeek = sig.dayOfWeek | temp.dayOfWeek; // bitwise OR dayOfWeek
        schrijfSignaalEEPROM(timtab[matchpos], sig);
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
    schrijfSignaalEEPROM(eeindex, sig);
    // We schuiven alles vanaf matchpos 1 plaats op  

    int t;
    t = numtimtab - 1;
    while (t >= matchpos) {
        timtab[t + 1] = timtab[t];
        t--;
    }
    timtab[matchpos] = eeindex;
    numtimtab++;
    nextindex = 0;
    return 0; // alles OK
}

int Rooster::signaalInvoegen(int hour, int minute, int dayOfWeek, int rooster)
{
    Signaal_t bm;
    bm.hour = hour;
    bm.minute = minute;
    bm.dayOfWeek = dayOfWeek;
    bm.channel = rooster;
    return signaalInvoegen(bm);
}

void Rooster::signaalLezen(int tijd, Signaal_t &sig) {
    // Lees signaal met ingang in timtab
    leesSignaalEEPROM(timtab[tijd], sig);
}
/*
void Rooster::haalSignaaltijd(int index, Signaal_t &sig) // print signaal
{
    leesSignaalEEPROM(timtab[index], sig);
}
*/
bool Rooster::isDagActief(int dow, int dowbitmap) 
{
    return (((0x81 >> dow) & dowbitmap) & 0x7F) != 0;
    //return (1 << dow) & dowbitmap;   // oude test werkt niet
}

int Rooster::update(int hour, int minute, int second, int dow)
{
    // test of een signaal actief is op de huidige tijd.
    // huidige tijd is in parameters hour, minute, second, dow gegeven.

    // update test of volgende Signaal actief is, en retourneert rooster,
    // verhoogt nextindex
    static int oldsecond = 0;
    if ((hour == 0) and (minute == 0) and (second == 0) and (oldsecond == 59)) {  // 24 uur RESET !
        nextindex = 0;
    }
    oldsecond = second;

    if (nextindex >= numtimtab) {  // einde van timtab bereikt? skip
        return INFO_NONE_ACTIVE;
    }

    Signaal_t sig;
    leesSignaalEEPROM(timtab[nextindex], sig);  // lees eerstvolgende signaal

    if (hour * 60 + minute > sig.hour * 60 + sig.minute) {  // tijd nog niet bereikt? test volgende signaal
        nextindex ++;
        return INFO_NONE_ACTIVE; // geen signalen actief
    }

    // Test of tijd gelijk is aan eerstvolgende signaal. 
    // Test alleen op seconde 0; dit verkleint kans op dubbele trigger
    // wanneer belautomaat koud start in de minuut van het signaal. 
    // Er worden zoveel signaaltijden getest als er perioden TASK_INTERVAL_MS  
    // (zie belautomaat_lib.h) passen in 1 seconde, 
    // Geef alleen actief signaal als roosterschakelaar AAN is.
    if (hour == sig.hour && minute == sig.minute && second < 1)
    {
        DPRINT(" idx ");
        DPRINT(nextindex);
        DPRINT(" ");
        DPRINT(hour);
        DPRINT(":");
        DPRINT(minute);
        if (isDagActief(dow, sig.dayOfWeek)) {
            // tijd klopt. is actief rooster?
            //nextindex = (nextindex + 1) % numtimtab;
            DPRINT(" Dow ");
            DPRINT(dow);
            if (isactiefrooster(sig.channel)) {
                DPRINT(" rooster ");
                DPRINTLN(sig.channel);
                nextindex ++;
                return sig.channel;
            }
        }
        DPRINTLN();
        nextindex ++;
    }
    return INFO_NONE_ACTIVE; // geen signalen actief
}

int Rooster::getNextIndex(void) {
    return nextindex;
}
/*
https://en.wikipedia.org/wiki/Binary_search_algorithm
Zoeken in array met dubbele elementen. Dat is hier van toepassing!

function binary_search_leftmost(A, n, T):
    L := 0
    R := n
    while L < R:
        m := floor((L + R) / 2)
        if A[m] < T:
            L := m + 1
        else:
            R := m
    return L
*/

void Rooster::findNextIndex(int hour, int minute) {
    // Zoek index van eerstvolgende signaal gegeven het huidige uur en minuut
 
    int left = 0, right = numtimtab, middle;
    Signaal_t sig, nu;
    int compvalue = -1;
    nu.hour = hour; 
    nu.minute = minute;
    while ((left < right) && (compvalue != 0)) {
        middle = (left + right) / 2;
        signaalLezen(middle, sig);
        compvalue = vergelijkSignaal(sig, nu);
        if (compvalue <= 0) 
            left = middle + 1;
        else
            right = middle - 1;
        DPRINT("findNxt ");
        DPRINT(left);
        DPRINT(" ");
        DPRINTLN(right);
    }
    if (compvalue == 0) {
        nextindex = middle;
    } else {
        nextindex = left;
    }
    DPRINTLN("findNxt done");
}

int Rooster::aantalSignalen() {
    // geef huidig aantal signalen in rooster
    return numtimtab;
}

int Rooster::haalVolgend(int index, int filter) {
    // blader naar volgend signaal in gegeven kanaal
    // om eerste te halen, roep aan met index=-1
    Signaal_t sig;
    while (true) {
        index++;
        if (index >= numtimtab) {
            return WARN_NOMORESIGNALS;
        }
        leesSignaalEEPROM(index, sig);
        if (sig.channel == filter) {
            return index;
        }
    }
}

int Rooster::haalVorig(int index, int filter) {
    // blader naar vorig signaal in gegeven kanaal
    Signaal_t sig;
    while (true) {
        index--;
        if (index < 0) {
            return WARN_NOMORESIGNALS;
        }
        leesSignaalEEPROM(index, sig);
        if (sig.channel == filter) {
            return index;
        }
    }
}

/* Hulpfuncties voor het laden en bewaren van variabelen in EEPROM */
int EEVarLoad(int index) {
  if (index >= 0 && index < MAX_EEVAR) {
    int value;
    EEPROM.get(EEADR_EEVAR + sizeof(int)*index, value);
    return value;
  }
}
void EEVarStore(int index, int value) {
  if (index >= 0 && index < MAX_EEVAR) {
    EEPROM.put(EEADR_EEVAR + sizeof(int)*index, value);
  }
}

void Rooster::zetactiefrooster(int index, bool isactief) {
    if (isactief) {
        hoofdschakelaar |= (1 << index);
    } else {
        hoofdschakelaar &= 0xffff ^(1 << index);
    }
    EEVarStore(EEVAR_HOOFDS, hoofdschakelaar);  // wijzigingen direct naar EEPROM
}
// opvragen van roosterschakelaar
bool Rooster::isactiefrooster(int index) {
    // hoofdschakelaar is geladen in constructor. 
    return 0 != (hoofdschakelaar & (1 << index));
}

void Rooster::zetnaam(int nummer, char *naam) {
    int eeadres = EEADR_NAAM + MAX_NAAMLEN * nummer;
    int index;
    if (nummer < 0 || nummer >= MAXROOSTER) return;
    for (index = 0; naam[index] != 0 && index < MAX_NAAMLEN; index++) {
        EEPROM.put(eeadres + index, naam[index]);
    }
    for (; index < MAX_NAAMLEN; index++) {
        EEPROM.put(eeadres + index, 0);
    }
}

char* Rooster::haalnaam(int nummer){
    int eeadres = EEADR_NAAM + MAX_NAAMLEN * nummer;
    static char naam[MAX_NAAMLEN + 2];
    int index;
    if (nummer < 0 || nummer >= MAXROOSTER) return 0;
    for (index = 0; index < MAX_NAAMLEN; index++) {
        EEPROM.get(eeadres + index, naam[index]);
    }
    naam[index] = 0;
    return naam;
}

void Rooster::zetuitgang(int nummer, int uitgang) {
    EEPROM.put(EEADR_UITGANG + (nummer * INT_WIDTH), uitgang);
}

int Rooster::haaluitgang(int nummer) {
    int value;
    EEPROM.get(EEADR_UITGANG + (nummer * INT_WIDTH), value);
    return value;
}

#ifdef DEBUG
char *Rooster::printDayOfWeek1(char *buffer, int dayOfWeek)
{
    // print dag van de week voor elke bit in bitveld.
    // buffer moet minimaal 15 tekens lang zijn.
    const char dowNames[] PROGMEM = {"MDWDVZZ"};
    uint8_t masker = 64;
    int index = 0;
    for (int dag = 0; dag < 7; dag++)
    {
        if (dayOfWeek & masker) {
            buffer[index] = dowNames[index];
        } else {
            buffer[index] = (char)0xB0;
        }
        index++;
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
    const char hex[] PROGMEM = "0123456789ABCDEF";
    buffer[1] = hex[num % 16];
    num >>= 4;
    buffer[0] = hex[num % 16];
    buffer[2] = 0;
    return buffer;
}

char *Rooster::printSignaal(char *buffer, Signaal_t &sig)
{
    // retourneer stringweergave van Signaal
    // buffer wijst naar string of char array van minstens .. tekens.
    print2decimals(&buffer[0], sig.hour); // 0 en 1
    buffer[2] = ':';
    print2decimals(&buffer[3], sig.minute); // 3 en 4
    buffer[5] = ' ';
    printDayOfWeek1(&buffer[6], sig.dayOfWeek);
    //int a = strlen(buffer);
    //buffer[a] = 'k';
    //print2decimals(&buffer[a + 1], bm.channel); // 0 en 1
    // tekst is te lang voor LCD !
}

char* Rooster::printRoosterRegel(int index) // print signaal
{
    static char buf[25];
    Signaal_t sig;
    int eeindex = timtab[index];
    leesSignaalEEPROM(eeindex, sig);    
    printSignaal(buf, sig);
    return buf;
}

void Rooster::printRooster()
{
    DPRINT(F("  numtimtab:"));
    DPRINT(numtimtab);
    DPRINT(F("  nextindex:"));
    DPRINTLN(nextindex);
    for (int index = 0; index < numtimtab; index++)
    {
        DPRINT(printRoosterRegel(index));
    }
}

void Rooster::printStatus(int status) 
{
    // geef tekst van status gegeven door signaalInvoegen
    DPRINT(F("= Status: "));
    DPRINT(status);
    DPRINT(F("  "));
    switch (status) {
        case ERROR_TIMTAB_FULL:
            DPRINTLN(F("Error: Timtab full"));
            break;
        case ERROR_EEPROM_FULL:
            DPRINTLN(F("Error: EEPROM full"));
            break;
        case INFO_NONE_ACTIVE:
            DPRINTLN(F("Info: None active"));
            break;
        case WARN_RECORD_MERGED:
            DPRINTLN(F("Warning: record merged"));
            break;
        default:
            DPRINTLN(F("Success"));
            break;
    }
}
#endif 

