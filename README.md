# ESP32 Mini Arcade

A pocket-sized arcade cabinet built around an ESP32, made by **Based Squad** at a hackathon. The ESP32 hosts its own WiFi network and serves a web app straight from its onboard flash — mounted inside the cabinet is an iPhone running that page in Safari, which acts as the machine's actual screen. A WebSocket connection between the firmware and the page keeps the two in sync in real time: the joystick, two physical buttons, and an ultrasonic wave sensor drive the on-screen UI instantly, while the ESP32 reacts back over that same socket — flashing an 8-LED array, waving a pair of servo "arms", or shifting the RGB status LED — depending on what's happening on screen (games, a clock, a weather widget, and a restaurant picker).

![Mini Arcade](assets/Mini%20Arcade.jpg)

## Features

- **Brick Breaker** and **Alien Invasion** — two arcade games, playable with the joystick
- **Food Finder** — a quiz-driven restaurant picker (cuisine, budget, distance) that searches nearby places via the Google Places API, runs a radar-style expanding search, and lets you save and rate favourites
- **Weather** — current conditions plus a 7-day forecast
- **Clock** — the default idle screen
- Physical feedback: an 8-LED array and two servo "arms" animate on events (winning, waking up, going to sleep)
- Wave-to-wake: an ultrasonic sensor detects a hand wave to toggle the machine in and out of sleep
- Everything talks over a WebSocket between the firmware and the on-device web app, so the physical controls (joystick, buttons) drive the UI in real time

## Hardware

Built on a **Lolin32 Lite** (ESP32) dev board.

| Component | Pin(s) |
|---|---|
| LED array (shift register: data / clock / latch) | GPIO 25 / 32 / 33 |
| LED array power enable | GPIO 13 |
| Nav button | GPIO 5 |
| Select button | GPIO 18 |
| Joystick (X / Y / press) | GPIO 36 / 39 / 4 |
| RGB status LED (R / G / B) | GPIO 12 / 14 / 27 |
| Ultrasonic sensor (trig / echo) | GPIO 26 / 15 |
| Left / right arm servos | GPIO 19 / 21 |

## Software stack

- **Firmware:** C++ (Arduino framework) on PlatformIO, using `ESPAsyncWebServer` + `AsyncTCP` to serve the frontend and handle the WebSocket, `LittleFS` as the on-chip filesystem, and `ESP32Servo` for the arms
- **Frontend:** vanilla JS/HTML/CSS served straight from the ESP32's flash, no build step, communicating with the firmware over a WebSocket

## Setup

1. **Clone the repo** and open it in VS Code with the [PlatformIO](https://platformio.org/) extension installed.

2. **Add your WiFi credentials.** Create `include/config.h` (this is gitignored, so it won't already exist after cloning):
   ```cpp
   #define WIFI_SSID "your-network-name"
   #define WIFI_PASSWORD "your-network-password"
   ```

3. **Food Finder needs a Google Maps/Places API key**, set in `data/index.html`. If you're re-using this repo, generate your own key and restrict it (HTTP referrer / API restrictions) rather than reusing whatever key is already committed.

4. **Flash the board**, with the ESP32 connected over USB:
   - Upload the filesystem image first (PlatformIO: "Upload Filesystem Image") — this pushes everything in `data/` onto the ESP32's LittleFS partition
   - Then upload the firmware itself (PlatformIO: "Upload")

5. **Open the serial monitor** (115200 baud) to watch it connect to WiFi and print its IP address, then open that IP in a browser on the same network — that page is the on-device UI, and the physical controls only respond once it's open and connected over the WebSocket.

## Usage

- **Nav button** — cycle between apps (Clock / Weather / Food Finder / Game)
- **Select button** — confirm / launch
- **Joystick** — navigate menus and control gameplay
- **SW (joystick press)** — back / escape
- **Wave a hand** near the ultrasonic sensor to wake the machine from sleep

## Project structure

```
├── src/main.cpp          # firmware: WiFi, WebSocket server, inputs, LEDs, servos
├── include/config.h      # WiFi credentials (gitignored)
├── data/                 # frontend served from LittleFS (HTML/CSS/JS, games, fonts, media)
├── stl/                  # 3D-printable enclosure
├── assets/                # renders / photos of the finished build
└── test/                 # standalone hardware test sketches
```

## Team — Based Squad

Axel Woods · Ashley Hur · Sam Korania · Blake Hybarraclough · Eva Foo
