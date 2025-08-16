#include <Arduino.h>
#include <BleSpam.h>

BleSpam bleSpammer;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial to be ready
  Serial.println("\nStarting BLE Spam Example...");
  // Initialize random seed
  randomSeed(analogRead(0));
}

void loop() {
  Serial.println("Spamming AppleJuice for 5 seconds...");
  bleSpammer.spam(BleSpam::AppleJuice, 5000);
  delay(1000);

  Serial.println("Spamming SourApple for 5 seconds...");
  bleSpammer.spam(BleSpam::SourApple, 5000);
  delay(1000);

  Serial.println("Spamming Microsoft SwiftPair for 5 seconds...");
  bleSpammer.spam(BleSpam::Microsoft, 5000);
  delay(1000);
  
  Serial.println("Spamming all types for 10 seconds...");
  bleSpammer.spamAll(10000);
  delay(1000);

  Serial.println("Spamming custom name 'FreeWiFi' for 5 seconds...");
  bleSpammer.spamCustom("FreeWiFi", 5000);
  delay(1000);

  Serial.println("\nLoop finished. Restarting in 5 seconds...");
  delay(5000);
}