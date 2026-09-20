#include <Arduino.h>

#define VRX_PIN 36
#define VRY_PIN 39
#define SW_PIN 4

int sw_prev = HIGH;
int debounced_sw_curr = HIGH;
unsigned long last_sw_change = 0;
const unsigned short debounceDelay = 50;

void handleSwButton()
{
    int sw_curr = digitalRead(SW_PIN);
    if (sw_curr != sw_prev)
    {
        last_sw_change = millis();
    }

    if ((millis() - last_sw_change) > debounceDelay)
    {
        if (sw_curr != debounced_sw_curr)
        {
            debounced_sw_curr = sw_curr;
            if (debounced_sw_curr == LOW)
            {
                Serial.println("SW pressed - escape");
            }
        }
    }
    sw_prev = sw_curr;
}

void setup()
{
    Serial.begin(115200);
    pinMode(SW_PIN, INPUT_PULLUP);
}

void loop()
{
    int xVal = analogRead(VRX_PIN);
    int yVal = analogRead(VRY_PIN);
    int swVal = digitalRead(SW_PIN);

    Serial.print("X: ");
    Serial.printf("%6d", xVal);
    Serial.print("  Y: ");
    Serial.printf("%6d", yVal);
    Serial.print("  SW: ");
    Serial.println(swVal);

    delay(100);

    handleSwButton();
}
