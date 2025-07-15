#ifndef ROBOTDISPLAY_H
#define ROBOTDISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

class RobotDisplay {
private:
    // Display object
    Adafruit_ST7735 tft;
    
    // Constanten voor berekeningen en kleuren
    static const float tweePi;
    static const uint16_t kleuren[];
    static const char* kleurNamen[];
    
    // Status variabelen voor robot en systeem
    unsigned long startTijd, laatstGezienTijd;
    int richting, laatstGezienRichting;
    bool commandCompleet, systeemGestart, kanGOntvangen, robotKlaar;
    bool resetActief;
    unsigned long resetTijd;
    String ontvangenCommando, laatstGezeenCommando;
    int kleurTellers[3]; // Rood, Groen, Blauw
    
    // Display posities en afmetingen
    static const int16_t centrumX, centrumY;
    static const int16_t klokX, klokY;
    static const uint8_t cirkelRadius;
    
    // Private methoden
    void behandelReset();
    void toonResetScherm();
    void behandelStartCommando();
    void initialiseerHoofdScherm();
    void tekenTijdVak();
    void updateTijdWeergave();
    void updateKlokWijzers(unsigned long seconden);
    void updateRichtingPijl();
    void tekenRichtingPijl(int dir);
    void tekenCirkel();
    void toonWachtBericht();
    void toonSetupTekst();
    void toonSetupTijd();
    void verwerkCommando();
    void resetSysteem();
    void toonEindScherm();
    
public:
    // Constructor
    RobotDisplay(uint8_t cs, uint8_t dc, uint8_t rst);
    
    // Publieke methoden
    void begin();
    void update();
    void handleSerialInput(String command);
};

#endif