#include <Arduino.h>

#define DATA_PIN 6
#define CLOCK_PIN 2
#define LATCH_PIN 4

void updateLEDs(byte pattern)
{
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, pattern);
  digitalWrite(LATCH_PIN, HIGH);
}

void setup()
{
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
}

void loop()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    byte pattern = (byte)strtol(input.c_str(), NULL, 2); // base 2 = binary
    updateLEDs(pattern);
    Serial.print("Set LED pattern to ");
    Serial.println(pattern, BIN);
  }
}
