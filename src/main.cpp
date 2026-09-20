#include "FS.h"
#include <Arduino.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESP32Servo.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "config.h"

/**********************************************************************/
/* Defines */

// LED pins
#define LED_DATA_PIN 25
#define LED_CLOCK_PIN 32
#define LED_LATCH_PIN 33
// Button pins
#define NAV_BUTTON_PIN 5
#define SEL_BUTTON_PIN 18
// Joystick pins
#define VRX_PIN 36
#define VRY_PIN 39
#define SW_PIN 4
// RGB pins
#define RED_PIN 12
#define GREEN_PIN 14
#define BLUE_PIN 27
// Ultrasonic pins
#define TRIG_PIN 26
#define ECHO_PIN 15
// Servos
#define SERVO_L_PIN 19
#define SERVO_R_PIN 21
// Misc.
#define FORMAT_LITTLEFS_IF_FAILED true
#define STICK_DEADBAND 1200
#define WAVE_DIST_THRESHOLD_CM 15
#define WAVE_COOLDOWN_MS 1000
#define SPEED_OF_SOUND_CM_US 0.0343
// on/off LED
#define ON_OFF_PIN 13

/**********************************************************************/
/* Function prototypes */

void updateLEDs(byte pattern);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void handleNavButton();
void handleSelButton();
void handleJoyStick();
void handleSwButton();
void handleUltrasonic();
void update_app(JSONVar msg);
void celebrate();
void KnightRiderLEDs();
void sleepLEDs();
void wakeLEDs();
void celebrateArms();
void wakeArms();
void sleepArms();
void save_food(JSONVar msg);
int calibrateStick(int pin);
int find_dist_cm();

/**********************************************************************/
/* Global vars */

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

int nav_button_prev = HIGH;
int debounced_nav_curr = HIGH;
unsigned long last_nav_change = 0;

int sel_button_prev = HIGH;
int debounced_sel_curr = HIGH;
unsigned long last_sel_change = 0;

int sw_prev = HIGH;
int debounced_sw_curr = HIGH;
unsigned long last_sw_change = 0;
const unsigned short debounceDelay = 50;

int xCentre = 4000;
int yCentre = 4000;
String lastStickDirection = "none";

unsigned long lastWaveTime = 0;
bool wasClose = false;

bool isAsleep = true;

bool client_connected = false;

Servo leftArm;
Servo rightArm;

/**********************************************************************/

AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

void setup() {
  delay(2000);
  Serial.begin(115200);

  // setup filesystem
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  // connect to WiFi
  WiFi.disconnect(false, true);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // setup websocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // webpage server routing
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();

  // Pins
  pinMode(LED_DATA_PIN, OUTPUT);
  pinMode(LED_CLOCK_PIN, OUTPUT);
  pinMode(LED_LATCH_PIN, OUTPUT);
  pinMode(NAV_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(ON_OFF_PIN, OUTPUT);
  leftArm.attach(SERVO_L_PIN);
  rightArm.attach(SERVO_R_PIN);

  // Calibrate joystick
  delay(500);
  xCentre = calibrateStick(VRX_PIN);
  yCentre = calibrateStick(VRY_PIN);

  // Initial RGB state
  analogWrite(RED_PIN, 255);
  analogWrite(GREEN_PIN, 255);
  analogWrite(BLUE_PIN, 0);
}

void loop() {
  if (client_connected) {
    // Inputs
    handleNavButton();
    handleSelButton();
    handleSwButton();
    handleJoyStick();
    handleUltrasonic();
  }
}

/**********************************************************************/

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT: {
    JSONVar msg;
    msg["type"] = "reset";
    ws.textAll(JSON.stringify(msg));
    client_connected = true;
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                  client->remoteIP().toString().c_str());
    break;
  }
  case WS_EVT_DISCONNECT:
    client_connected = false;
    isAsleep = true;
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    // received msg through websocket
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

/**********************************************************************/

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len &&
      info->opcode == WS_TEXT) {
    data[len] = 0;
    String msg_str = (char *)data;
    JSONVar msg = JSON.parse(msg_str);
    String type = msg["type"];
    if (type == "food_chosen") {
      celebrate();
    } else if (type == "save_food") {
      save_food(msg);
    } else if (type == "update_app") {
      update_app(msg);
    } else if (type == "sleep") {
      Serial.println("Going to sleep");
      isAsleep = true;
      sleepLEDs();
      sleepArms();
    }
  }
}

/**********************************************************************/
/* Handle inputs */

void handleNavButton() {
  int nav_button_curr = digitalRead(NAV_BUTTON_PIN);
  if (nav_button_curr != nav_button_prev) {
    last_nav_change = millis();
  }

  if ((millis() - last_nav_change) > debounceDelay) {
    if (nav_button_curr != debounced_nav_curr) {
      debounced_nav_curr = nav_button_curr;
      if (debounced_nav_curr == LOW) {
        Serial.println("Nav button pressed - cycling app");
        JSONVar obj;
        obj["type"] = "nav";
        ws.textAll(JSON.stringify(obj));
        // Print IP
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
    }
  }
  nav_button_prev = nav_button_curr;
}

void handleSelButton() {
  int sel_button_curr = digitalRead(SEL_BUTTON_PIN);
  if (sel_button_curr != sel_button_prev) {
    last_sel_change = millis();
  }

  if ((millis() - last_sel_change) > debounceDelay) {
    if (sel_button_curr != debounced_sel_curr) {
      debounced_sel_curr = sel_button_curr;
      if (debounced_sel_curr == LOW) {
        Serial.println("sel button pressed");
        JSONVar obj;
        obj["type"] = "sel";
        ws.textAll(JSON.stringify(obj));
      }
    }
  }
  sel_button_prev = sel_button_curr;
}

void handleJoyStick() {
  int xVal = analogRead(VRX_PIN);
  int yVal = analogRead(VRY_PIN);
  int delta_x = xVal - xCentre;
  int delta_y = yVal - yCentre;

  String stickDirection = "none";

  if (abs(delta_x) > STICK_DEADBAND || abs(delta_y) > STICK_DEADBAND) {
    if (abs(delta_x) > abs(delta_y)) {
      stickDirection = delta_x > 0 ? "ArrowLeft" : "ArrowRight";
    } else {
      stickDirection = delta_y > 0 ? "ArrowDown" : "ArrowUp";
    }
  }

  if (stickDirection != "none" && stickDirection != lastStickDirection) {
    Serial.println("Joystick moved");
    JSONVar obj;
    obj["type"] = "stick";
    obj["direction"] = stickDirection;
    ws.textAll(JSON.stringify(obj));
  }
  lastStickDirection = stickDirection;
}

void handleSwButton() {
  int sw_curr = digitalRead(SW_PIN);
  if (sw_curr != sw_prev) {
    last_sw_change = millis();
  }

  if ((millis() - last_sw_change) > debounceDelay) {
    if (sw_curr != debounced_sw_curr) {
      debounced_sw_curr = sw_curr;
      if (debounced_sw_curr == LOW) {
        Serial.println("SW pressed - escape");
        JSONVar obj;
        obj["type"] = "sw";
        ws.textAll(JSON.stringify(obj));
      }
    }
  }
  sw_prev = sw_curr;
}

void handleUltrasonic() {
  int dist_cm = find_dist_cm();
  bool isClose = (dist_cm > 0 && dist_cm < WAVE_DIST_THRESHOLD_CM);
  if (isClose && !wasClose && (millis() - lastWaveTime > WAVE_COOLDOWN_MS)) {
    Serial.println("Hand wave detected - toggle sleep");
    if (isAsleep) {
      Serial.println("Hand wave detected - Waking up");
      wakeLEDs();
      wakeArms();
      isAsleep = false;
      JSONVar obj;
      obj["type"] = "sleep";
      ws.textAll(JSON.stringify(obj));
    }
    lastWaveTime = millis();
  }
  wasClose = isClose;
}

/**********************************************************************/

void celebrate() {
  // tweak out
  KnightRiderLEDs();
  celebrateArms();
}

void save_food(JSONVar msg) {}

void update_app(JSONVar msg) {
  String cur_app = msg["app"];
  if (cur_app == "clock-app") {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 255);
    analogWrite(BLUE_PIN, 0);
  }

  else if (cur_app == "weather-app") {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 255);
  }

  else if (cur_app == "food-finder-app") {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  }

  else if (cur_app == "game-app") {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 255);
  }
}

/**********************************************************************/
/* LED sequences */

void KnightRiderLEDs() {
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 8; i++) {
      byte pattern = 1 << i;
      updateLEDs(pattern);
      delay(50);
    }
    for (int i = 6; i >= 1; i--) {
      byte pattern = 1 << i;
      updateLEDs(pattern);
      delay(50);
    }
  }
  updateLEDs(1);
  delay(50);
  updateLEDs(0);
}

void sleepLEDs() {
  byte pattern = 0b11111111;
  updateLEDs(pattern);
  delay(150);
  digitalWrite(ON_OFF_PIN, LOW);

  // Converging inwards
  int pairs[4][2] = {{0, 7}, {1, 6}, {2, 5}, {3, 4}};
  int stepDelay = 150;
  for (int i = 0; i < 4; i++) {
    pattern &= ~(1 << pairs[i][0]);
    pattern &= ~(1 << pairs[i][1]);
    updateLEDs(pattern);
    delay(stepDelay);
    stepDelay += 100; // slow down each step
  }
}

void wakeLEDs() {
  byte pattern = 0b00000000; // all off
  updateLEDs(pattern);
  delay(300);
  digitalWrite(ON_OFF_PIN, HIGH);

  // Diverging Outwards
  int pairs[4][2] = {{3, 4}, {2, 5}, {1, 6}, {0, 7}};
  int stepDelay = 300;
  for (int i = 0; i < 4; i++) {
    pattern |= (1 << pairs[i][0]);
    pattern |= (1 << pairs[i][1]);
    updateLEDs(pattern);
    delay(stepDelay);
    stepDelay -= 70; // speed up each step
  }
}

/**********************************************************************/
/* Servo sequences */

void celebrateArms() {
  // pump arms
  for (int i = 0; i < 4; i++) {
    leftArm.write(180);
    rightArm.write(0);
    delay(300);
    leftArm.write(0);
    rightArm.write(180);
    delay(300);
  }
  leftArm.write(90);
  rightArm.write(90);
}

void sleepArms() {
  // Slow droop
  int stepDelay = 150;
  for (int angle = 180; angle >= 0; angle -= 30) {
    leftArm.write(angle);
    rightArm.write(angle);
    delay(stepDelay);
    stepDelay += 80;
  }
}

void wakeArms() {
  // stretch up
  for (int angle = 0; angle <= 180; angle += 20) {
    leftArm.write(angle);
    rightArm.write(angle);
    delay(150);
  }
  // bounce at the top
  for (int i = 0; i < 2; i++) {
    leftArm.write(160);
    rightArm.write(160);
    delay(100);
    leftArm.write(180);
    rightArm.write(180);
    delay(100);
  }

  leftArm.write(90);
  rightArm.write(90);
}

/**********************************************************************/

void updateLEDs(byte pattern) {
  digitalWrite(LED_LATCH_PIN, LOW);
  shiftOut(LED_DATA_PIN, LED_CLOCK_PIN, LSBFIRST, pattern);
  digitalWrite(LED_LATCH_PIN, HIGH);
}

int calibrateStick(int pin) {
  int sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(pin);
    delay(5);
  }

  return sum / 20;
}

int find_dist_cm() {
  digitalWrite(TRIG_PIN, LOW); // Ensure low first
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int time_elapsed_us = pulseIn(ECHO_PIN, HIGH, 11662); // timeout after 2m
  if (time_elapsed_us == 0) {
    return -1; // out of range
  }
  int dist_cm = time_elapsed_us * SPEED_OF_SOUND_CM_US / 2;
  return dist_cm;
}