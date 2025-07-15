#include "RobotDisplay.h"

// Pin definities voor het display
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8

// Maak een RobotDisplay object aan
RobotDisplay display(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    Serial.begin(9600);
    
    // Initialiseer het display
    display.begin();
}

void loop() {
    // Update het display
    display.update();
}

void serialEvent() {
  // Lees alle beschikbare karakters van de seriële poort
  String command = "";
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      // Commando is compleet bij newline
      display.handleSerialInput(command);
      command = ""; // Reset voor volgende commando
    } else {
      command += inChar; // Voeg karakter toe aan commando
    }
  }
}