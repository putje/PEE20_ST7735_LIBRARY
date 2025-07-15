#include "RobotDisplay.h"

// Constanten definities
const float RobotDisplay::tweePi = 6.2831853;
const uint16_t RobotDisplay::kleuren[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE};
const char* RobotDisplay::kleurNamen[] = {"Rood", "Groen", "Blauw"};
const int16_t RobotDisplay::centrumX = 64;
const int16_t RobotDisplay::centrumY = 84;
const int16_t RobotDisplay::klokX = 113;
const int16_t RobotDisplay::klokY = 12;
const uint8_t RobotDisplay::cirkelRadius = 50;

// Constructor
RobotDisplay::RobotDisplay(uint8_t cs, uint8_t dc, uint8_t rst) 
    : tft(cs, dc, rst) {
    // Initialiseer variabelen
    startTijd = 0;
    laatstGezienTijd = 0;
    richting = 1;
    laatstGezienRichting = 1;
    commandCompleet = false;
    systeemGestart = false;
    kanGOntvangen = true;
    robotKlaar = false;
    resetActief = false;
    resetTijd = 0;
    ontvangenCommando = "";
    laatstGezeenCommando = "";
    kleurTellers[0] = 0;
    kleurTellers[1] = 0;
    kleurTellers[2] = 0;
}

void RobotDisplay::begin() {
    tft.initR(INITR_GREENTAB);
    tft.fillScreen(ST77XX_BLACK);

    // Setup volgorde met compactere functies
    tekenTijdVak();
    updateTijdWeergave();
    delay(1000);

    toonSetupTekst();
    delay(1000);

    toonSetupTijd();
    delay(2000);

    initialiseerHoofdScherm();
    toonWachtBericht();
}

void RobotDisplay::update() {
    // EERST controleren op reset commando - dit heeft altijd prioriteit!
    if (commandCompleet && ontvangenCommando.equals("SR")) {
        resetSysteem();
        toonResetScherm();
        commandCompleet = false;
        ontvangenCommando = "";
        return; // Direct return na reset start
    }

    // Controleer of we in reset modus zijn
    if (resetActief) {
        behandelReset();
        return;
    }

    if (!robotKlaar) {
        if (!systeemGestart) {
            behandelStartCommando();
            return;
        }

        updateTijdWeergave();
        if (commandCompleet) {
            verwerkCommando();
            commandCompleet = false;
            ontvangenCommando = "";
        }
        updateRichtingPijl();
    } else {
        // Ook in eindscherm staat kunnen we nog resetten
        if (commandCompleet) {
            // Alleen SR wordt hier verwerkt, andere commando's worden genegeerd
            if (!ontvangenCommando.equals("SR")) {
                Serial.println("Robot is klaar - alleen reset (SR) mogelijk");
            }
            commandCompleet = false;
            ontvangenCommando = "";
        }
    }
}

void RobotDisplay::handleSerialInput(String command) {
    ontvangenCommando = command;
    commandCompleet = true;
}

void RobotDisplay::behandelReset() {
    // Controleer of reset scherm lang genoeg getoond is (3 seconden)
    if (millis() - resetTijd >= 3000) {
        // Reset is compleet, start systeem opnieuw op
        resetActief = false;
        
        // Toon setup sequentie opnieuw
        tft.fillScreen(ST77XX_BLACK);
        tekenTijdVak();
        updateTijdWeergave();
        delay(500);

        toonSetupTekst();
        delay(1000);

        toonSetupTijd();
        delay(1000);

        initialiseerHoofdScherm();
        toonWachtBericht();
        
        Serial.println("Systeem herstart voltooid!");
    }
}

void RobotDisplay::toonResetScherm() {
    // Maak scherm leeg en toon reset bericht
    tft.fillScreen(ST77XX_BLACK);
    
    // Teken een mooie rand
    tft.drawRoundRect(5, 5, 118, 150, 8, ST77XX_RED);
    tft.drawRoundRect(6, 6, 116, 148, 7, ST77XX_RED);
    
    // Titel
    tft.setCursor(25, 25);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("RESET");
    
    // Ondertitel
    tft.setCursor(15, 50);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.println("Systeem wordt");
    tft.setCursor(25, 65);
    tft.println("opnieuw");
    tft.setCursor(25, 80);
    tft.println("opgestart");
    
    // Progress indicatie
    tft.setCursor(20, 105);
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Wacht even...");
    
    // Animatie balkjes
    for (int i = 0; i < 5; i++) {
        tft.fillRect(20 + (i * 18), 125, 12, 8, ST77XX_CYAN);
        delay(200);
    }
    
    // Zet reset tijd
    resetTijd = millis();
    resetActief = true;
    
    Serial.println("=== SYSTEEM RESET GESTART ===");
}

void RobotDisplay::behandelStartCommando() {
    if (commandCompleet && ontvangenCommando.equals("G") && kanGOntvangen) {
        systeemGestart = true;
        kanGOntvangen = false;
        startTijd = millis();
        laatstGezienTijd = 0;

        initialiseerHoofdScherm();
        commandCompleet = false;
        ontvangenCommando = "";
        Serial.println("System started!");
    } else if (commandCompleet) {
        if (ontvangenCommando.equals("G") && !kanGOntvangen) {
            Serial.println("G ignored - system already started");
        }
        commandCompleet = false;
        ontvangenCommando = "";
    }
}

void RobotDisplay::initialiseerHoofdScherm() {
    // Maak het scherm leeg en teken de basis elementen
    tft.fillScreen(ST77XX_BLACK);
    tekenTijdVak();
    updateTijdWeergave();
    tekenCirkel();
    tekenRichtingPijl(richting);
}

void RobotDisplay::tekenTijdVak() {
    // Teken een mooi afgerond vak bovenin voor tijd informatie
    tft.drawRoundRect(0, 0, tft.width(), 25, 2, ST77XX_WHITE);
    tft.fillRoundRect(1, 1, tft.width()-2, 23, 2, ST77XX_BLACK);

    // Teken het klokje frame rechtsboven
    tft.drawCircle(klokX, klokY, 8, ST77XX_WHITE);
}

void RobotDisplay::updateTijdWeergave() {
    if (!systeemGestart) return; // Alleen updaten als robot actief is

    unsigned long verstrekenTijd = (millis() - startTijd) / 1000;
    if (verstrekenTijd == laatstGezienTijd) return; // Geen update nodig

    laatstGezienTijd = verstrekenTijd;
    float afstand = tweePi * verstrekenTijd;

    // Wis het tekstgebied efficient in een keer
    tft.fillRect(1, 4, tft.width()-25, 17, ST77XX_BLACK);

    // Toon de verstreken tijd linksboven
    tft.setCursor(2, 4);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.print("Tijd:");
    tft.print(verstrekenTijd);
    tft.print("s");

    // Toon de afgelegde afstand eronder
    tft.setCursor(2, 13);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("Afstand:");
    tft.print(afstand, 1);
    tft.print("cm");

    updateKlokWijzers(verstrekenTijd);
}

void RobotDisplay::updateKlokWijzers(unsigned long seconden) {
    // Maak de klok van binnen leeg maar laat de rand intact
    tft.fillCircle(klokX, klokY, 7, ST77XX_BLACK);

    // Bereken de hoeken voor de wijzers (gebruik radialen direct)
    float minuutHoek = (seconden % 60) * 0.10472; // 6 graden per seconde in radialen
    float uurHoek = (seconden % 720) * 0.00873;   // 0.5 graden per seconde in radialen

    // Teken de lange minuutwijzer
    tft.drawLine(klokX, klokY,
                 klokX + 5 * sin(minuutHoek),
                 klokY - 5 * cos(minuutHoek), ST77XX_WHITE);

    // Teken de korte uurwijzer
    tft.drawLine(klokX, klokY,
                 klokX + 3 * sin(uurHoek),
                 klokY - 3 * cos(uurHoek), ST77XX_WHITE);

    // Teken het middelpuntje van de klok
    tft.drawPixel(klokX, klokY, ST77XX_WHITE);
}

void RobotDisplay::updateRichtingPijl() {
    // Alleen updaten als de richting daadwerkelijk veranderd is
    if (richting != laatstGezienRichting) {
        laatstGezienRichting = richting;
        // Maak de cirkel van binnen leeg voor nieuwe pijl
        tft.fillCircle(centrumX, centrumY, cirkelRadius-3, ST77XX_BLACK);
        tekenRichtingPijl(richting);
    }
}

void RobotDisplay::tekenRichtingPijl(int dir) {
    // Bepaal de kleur op basis van richting: rood=links, groen=rechtdoor, blauw=rechts
    uint16_t kleur = (dir == 0) ? ST77XX_RED : (dir == 1) ? ST77XX_GREEN : ST77XX_BLUE;

    if (dir == 0) { // Linkse pijl tekenen
        tft.fillRect(centrumX-8, centrumY-6, 30, 12, kleur);
        tft.fillTriangle(centrumX-25, centrumY, centrumX-8, centrumY-18, centrumX-8, centrumY+18, kleur);
        // Extra dikke lijn voor betere zichtbaarheid
        tft.fillRect(centrumX-9, centrumY-7, 32, 14, kleur);
        tft.fillTriangle(centrumX-27, centrumY, centrumX-9, centrumY-20, centrumX-9, centrumY+20, kleur);
    } else if (dir == 1) { // Rechtdoor pijl tekenen
        tft.fillRect(centrumX-6, centrumY-8, 12, 30, kleur);
        tft.fillTriangle(centrumX, centrumY-25, centrumX-18, centrumY-8, centrumX+18, centrumY-8, kleur);
        // Extra dikke lijn voor betere zichtbaarheid
        tft.fillRect(centrumX-7, centrumY-9, 14, 32, kleur);
        tft.fillTriangle(centrumX, centrumY-27, centrumX-20, centrumY-9, centrumX+20, centrumY-9, kleur);
    } else { // Rechts pijl
        tft.fillRect(centrumX-22, centrumY-6, 30, 12, kleur);
        tft.fillTriangle(centrumX+25, centrumY, centrumX+8, centrumY-18, centrumX+8, centrumY+18, kleur);
        // Extra dikke lijn voor betere zichtbaarheid
        tft.fillRect(centrumX-23, centrumY-7, 32, 14, kleur);
        tft.fillTriangle(centrumX+27, centrumY, centrumX+9, centrumY-20, centrumX+9, centrumY+20, kleur);
    }
}

void RobotDisplay::tekenCirkel() {
    // Teken een dikke witte cirkel door meerdere cirkels over elkaar
    for (int i = 0; i < 3; i++) {
        tft.drawCircle(centrumX, centrumY, cirkelRadius-i, ST77XX_WHITE);
    }
}

void RobotDisplay::toonWachtBericht() {
    // Maak de cirkel van binnen leeg voor het wachtbericht
    tft.fillCircle(centrumX, centrumY, cirkelRadius-5, ST77XX_BLACK);

    // Toon gecentreerde tekst binnen de cirkel
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);

    // "Wacht op" - eerste regel
    String lijn1 = "Wacht op";
    int tekstBreedte1 = lijn1.length() * 6; // 6 pixels per karakter voor textSize 1
    tft.setCursor(centrumX - tekstBreedte1/2, centrumY - 15);
    tft.print(lijn1);

    // "start" - tweede regel
    String lijn2 = "start";
    int tekstBreedte2 = lijn2.length() * 6;
    tft.setCursor(centrumX - tekstBreedte2/2, centrumY - 5);
    tft.print(lijn2);

    // "signaal" - derde regel
    String lijn3 = "signaal";
    int tekstBreedte3 = lijn3.length() * 6;
    tft.setCursor(centrumX - tekstBreedte3/2, centrumY + 5);
    tft.print(lijn3);
}

void RobotDisplay::toonSetupTekst() {
    // Toon de lange test tekst tijdens setup
    tft.fillScreen(ST77XX_BLACK);
    tekenTijdVak();

    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextWrap(true); // Automatische tekstomloop aan
    tft.print("Dit is de test tekst van de dreamteam robot, deze test is alleen een illustratie van het reken en schrijf werk van het scherm in verband met de Arduino Nano. Dit project wordt gezien als het grote drama van deze periode hoewel het project leuk is zorgt het voor stress en veel problemen");
}

void RobotDisplay::toonSetupTijd() {
    // Wis het gebied onder het tijdvak en toon setup duur
    tft.fillRect(0, 27, tft.width(), tft.height()-27, ST77XX_BLACK);

    tft.setCursor(0, 60);
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Setup duurde");
    tft.setTextColor(ST77XX_MAGENTA);
    tft.print(millis() / 1000);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" seconden.");
}

void RobotDisplay::verwerkCommando() {
    ontvangenCommando.trim(); // Verwijder eventuele spaties

    // Verwerk alleen als het commando anders is dan het vorige EN niet een geblokkeerde G
    if (ontvangenCommando != laatstGezeenCommando && !(ontvangenCommando.equals("G") && !kanGOntvangen)) {

        if (ontvangenCommando.equals("L")) {
            richting = 0; // links
            laatstGezeenCommando = ontvangenCommando;
        }
        else if (ontvangenCommando.equals("D")) {
            richting = 1; // straight/rechtdoor
            laatstGezeenCommando = ontvangenCommando;
        } 
        else if (ontvangenCommando.equals("R")) {
            richting = 2; // rechts
            laatstGezeenCommando = ontvangenCommando;
        }
        else if (ontvangenCommando.equals("LR")) {
            kleurTellers[0]++; // rood kleurtje gevonden
            Serial.print("Rood NFC gevonden! Totaal: ");
            Serial.println(kleurTellers[0]);
        }
        else if (ontvangenCommando.equals("LG")) {
            kleurTellers[1]++; // groen kleurtje
            Serial.print("Groen NFC gevonden! Totaal: ");
            Serial.println(kleurTellers[1]);
        }
        else if (ontvangenCommando.equals("LB")) {
            kleurTellers[2]++; // blauw kleurtje  
            Serial.print("Blauw NFC gevonden! Totaal: ");
            Serial.println(kleurTellers[2]);
        }
        else if (ontvangenCommando.equals("F")){
            robotKlaar = true; // robot is klaar!
            toonEindScherm();
        }
        else {
            Serial.print("Onbekend commando: ");
            Serial.println(ontvangenCommando);
        }
    }
}

void RobotDisplay::resetSysteem() {
    // Reset alle variabelen naar beginwaarden
    kleurTellers[0] = 0; // Rood teller reset
    kleurTellers[1] = 0; // Groen teller reset  
    kleurTellers[2] = 0; // Blauw teller reset
    robotKlaar = false;
    systeemGestart = false;
    kanGOntvangen = true;
    commandCompleet = false;
    richting = 1;
    laatstGezienRichting = 1;
    laatstGezeenCommando = "";
    startTijd = 0;
    laatstGezienTijd = 0;
    
    Serial.println("=== SYSTEEM VARIABELEN GERESET ===");
}

void RobotDisplay::toonEindScherm() {
    // Maak een mooi eindscherm met alle resultaten
    tft.fillScreen(ST77XX_BLACK); // scherm leegmaken

    // mooie rand rond scherm
    tft.drawRoundRect(1, 1, 126, 158, 5, ST77XX_WHITE);

    // Bereken de eindstatistieken
    unsigned long eindTijd = (millis() - startTijd) / 1000;
    float eindAfstand = tweePi * eindTijd;

    // titel sectie
    tft.setCursor(10, 10);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.println("TRAJECT RESULTATEN");

    // scheidingslijn onder titel
    tft.drawLine(10, 25, 118, 25, ST77XX_WHITE);

    // tijd resultaat
    tft.setCursor(10, 35);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Totale tijd:");
    tft.setCursor(10, 45);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(eindTijd);
    tft.print(" seconden");

    // afstand resultaat
    tft.setCursor(10, 60);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("Totale afstand:");
    tft.setCursor(10, 70);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(eindAfstand, 1);
    tft.print(" cm");

    // scheidingslijn tussen metingen en kleuren
    tft.drawLine(10, 85, 118, 85, ST77XX_WHITE);

    // NFC kleuren sectie
    tft.setCursor(25, 95);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("NFC KLEUREN:");

    // rood teller
    tft.setCursor(10, 110);
    tft.setTextColor(ST77XX_RED);
    tft.print("Rood: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(kleurTellers[0]);

    // groen teller
    tft.setCursor(10, 122);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Groen: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(kleurTellers[1]);

    // blauw teller
    tft.setCursor(10, 134);
    tft.setTextColor(ST77XX_BLUE);
    tft.print("Blauw: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(kleurTellers[2]);

    // totaal aantal kleuren
    int totaalKleuren = kleurTellers[0] + kleurTellers[1] + kleurTellers[2];
    tft.setCursor(10, 148);
    tft.setTextColor(ST77XX_MAGENTA);
    tft.print("Totaal: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(totaalKleuren);

    // debug info naar serial
    Serial.println("=== ROBOT TRAJECT VOLTOOID ===");
    Serial.print("Eindtijd: "); Serial.print(eindTijd); Serial.println(" seconden");
    Serial.print("Eindafstand: "); Serial.print(eindAfstand); Serial.println(" cm");
    Serial.print("Rood NFC's: "); Serial.println(kleurTellers[0]);
    Serial.print("Groen NFC's: "); Serial.println(kleurTellers[1]);
    Serial.print("Blauw NFC's: "); Serial.println(kleurTellers[2]);
    Serial.print("Totaal NFC's: "); Serial.println(totaalKleuren);
}