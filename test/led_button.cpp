#include <Arduino.h>

#define DATA_PIN 6  // DS
#define CLOCK_PIN 2 // SH_CP
#define LATCH_PIN 4 // ST_CP
#define BUTTON_PIN 8

int lastRawState = HIGH;
int debouncedState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // ms

void updateLEDs(byte pattern)
{
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, pattern);
  digitalWrite(LATCH_PIN, HIGH);
}

void knightRiderSweep()
{
  for (int i = 0; i < 8; i++)
  {
    updateLEDs(1 << i);
    delay(100);
  }
  for (int i = 6; i >= 1; i--)
  {
    updateLEDs(1 << i);
    delay(100);
  }
  updateLEDs(0); // all off when the sweep finishes
}

void setup()
{
  Serial.begin(115200);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop()
{
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastRawState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != debouncedState)
    {
      debouncedState = reading;

      if (debouncedState == LOW)
      {
        Serial.println("Button pressed - running sequence");
        knightRiderSweep();
      }
    }
  }

  lastRawState = reading;
}
