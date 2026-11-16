# GroupB1Enge1414
Website to control our alarm clock.

The ESP32 joins your WiFi, gets the time from NTP, and hosts a small web page. Open the page from any phone or laptop on the same network to set the alarm, snooze, or stop it. Alarm settings are saved in flash, so they survive a power cycle.

## Setup
1. Install the Arduino IDE and add ESP32 board support (Boards Manager -> "esp32" by Espressif). No extra libraries are needed.
2. In `AlarmClock/`, copy `secrets.h.example` to `secrets.h` and enter your WiFi name and password (the file is gitignored).
3. Open `AlarmClock/AlarmClock.ino`, select your ESP32 board, and upload.
4. Open the Serial Monitor at 115200 baud, then browse to the IP address it prints.

## Wiring
| Part | Pin |
|------|-----|
| Active buzzer (+) | GPIO 25 (other side to GND) |
| Status LED | GPIO 2 (onboard) |

Change the pin constants at the top of `AlarmClock.ino` if your wiring differs. The WiFi must be 2.4 GHz (the ESP32 can't join 5 GHz).

## Timezone
Set to US Eastern by default; edit `TZ_INFO` in `AlarmClock.ino` for another zone.

## API
`GET /api/status`, `POST /api/alarm` (`time=HH:MM&enabled=1&snooze=5`), `POST /api/snooze`, `POST /api/stop`, `POST /api/test`.
