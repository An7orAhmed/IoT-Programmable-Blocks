# LED + Button + Buzzer + Sensor IoT Block (ESP8266)

Firmware for a Wemos D1 Mini Lite (ESP8266) that provides:
- Local AP + web setup page for Wi-Fi and server URL
- Local HTTP APIs to read and control IO state
- Optional remote sync with a central host
- Onboard LED status indication
- EEPROM-based config persistence

## Features

- Runs in AP + STA mode (`WIFI_AP_STA`)
- Captive-style setup page at the device AP address (`192.168.4.1`)
- Saves config in EEPROM:
  - Router SSID
  - Router password
  - Server host URL (optional)
- mDNS advertisement when STA is connected
- Local IO API:
  - Read button/LED/buzzer/motion/temperature/humidity state
  - Set LED and buzzer state
- Remote sync (when `serverUrl` is configured):
  - Pull desired state from `host_url/api/<deviceId>`
  - Push current state to `host_url/api/<deviceId>/set` (POST first, GET fallback), including sensor values
- Onboard LED (`LED_BUILTIN`) behavior:
  - Blinks slowly when Wi-Fi station is connected
  - Off when not connected

## Hardware

Pin mapping in firmware (ESP8266 GPIO numbers):
- LED output: GPIO14
- Button input: GPIO12 (`INPUT_PULLUP`, pressed = LOW)
- Buzzer output: GPIO13
- DHT11 data: GPIO4
- PIR motion output: GPIO5
- Board LED: `LED_BUILTIN`

These are GPIO numbers, not Wemos D1 mini `D#` silkscreen aliases.

Board target from PlatformIO:
- Environment: `d1_mini_lite`
- Platform: `espressif8266`
- Framework: `arduino`

## Project Structure

- `src/main.cpp`: firmware source
- `platformio.ini`: build/upload config
- `include/`, `lib/`, `test/`: standard PlatformIO folders

## Build and Flash

### Prerequisites

- VS Code + PlatformIO extension, or PlatformIO Core CLI
- USB connection to D1 Mini Lite

### Build

```bash
platformio run
```

### Upload

```bash
platformio run -t upload
```

### Serial Monitor

```bash
platformio device monitor -b 115200
```

## First-Time Setup Flow

1. Power the board.
2. Connect to AP SSID: `IoT-Block-002`
3. AP password: `12345678`
4. Open `http://192.168.4.1`
5. Fill:
   - Router SSID (required)
   - Router password
   - Server Host URL (optional)
6. Save settings.
7. Reboot device to apply Wi-Fi station changes.

Notes:
- Firmware prints restored config and connection logs to serial.
- Firmware prints PIR, motion, DHT11, button, LED, and buzzer status once per second.
- If no valid config exists in EEPROM, defaults are used.

## Runtime Behavior

- Device always starts AP mode with SSID = `DEVICE_ID`.
- If saved router SSID exists, it attempts STA connection.
- If STA connects, mDNS is started:
  - `http://iot-block-002.local` (derived from `DEVICE_ID`)
- In `loop()`:
  - Web server handles requests
  - mDNS is updated
  - Remote pull/push sync runs when enabled
  - Outputs are driven from `ledState` and `buzzerState`

## Local API Reference

Base URL (AP mode):
- `http://192.168.4.1`

When STA is connected, use station IP instead.

### 1) GET /

Returns setup HTML page.

Response:
- `200 text/html`

### 2) POST /save

Saves config to EEPROM.

Form fields:
- `ssid` (required)
- `password`
- `serverUrl`

Responses:
- `200 text/html` on success
- `400 text/html` if SSID missing
- `500 text/html` if EEPROM commit fails

### 3) GET /api/discovery

Returns discovery and remote sync status.

Example response:

```json
{
  "deviceId": "IoT-Block-002",
  "hostname": "iot-block-002",
  "mdns": "iot-block-002.local",
  "stationConnected": true,
  "staIp": "192.168.1.50",
  "remoteSyncEnabled": true,
  "remotePullOk": true,
  "remotePullCode": 200,
  "remotePushOk": true,
  "remotePushCode": 200
}
```

Field notes:
- `remotePullOk` / `remotePushOk` can be `null` before first attempt.
- `remotePullCode` / `remotePushCode` are HTTP status or `-1` if request could not start.

### 4) GET /api/io

Returns current local IO status.

Responses:
- `200 application/json` when STA connected
- `503 application/json` when station not connected

Success example:

```json
{
  "stationConnected": true,
  "staIp": "192.168.1.50",
  "led": false,
  "buzzer": false,
  "button": true,
  "motion": false,
  "tempC": 27.31,
  "humidity": 54.0
}
```

Error example:

```json
{"error":"station_not_connected"}
```

### 5) GET or POST /api/io/set

Sets local output state.

Accepted parameters:
- `led`
- `buzz`

Accepted values:
- `1`, `0`, `true`, `false`, `on`, `off`, `HIGH`, `LOW`

Responses:
- `200 application/json` on success
- `400 application/json` for missing/invalid args
- `503 application/json` when station not connected

Success example:

```json
{"ok":true,"led":true,"buzzer":false}
```

Error examples:

```json
{"error":"missing_args","hint":"use led and/or buzz"}
```

```json
{"error":"invalid_led","accepted":["1","0","true","false"]}
```

## Remote Sync Contract

Remote sync is enabled only when:
- STA is connected, and
- `serverUrl` is non-empty

Firmware uses these endpoints:

### Pull Desired State

- Method: `GET`
- URL: `host_url/api/<deviceId>`
- Expected response: JSON containing at least `led` and/or `buzzer` (or `buzz`)

Example:

```json
{
  "led": true,
  "buzzer": false,
  "tempC": 27.31
}
```

### Push Current State

Primary:
- Method: `POST`
- URL: `host_url/api/<deviceId>/set`
- Content-Type: `application/json`
- Body:

```json
{
  "led": true,
  "buzzer": false,
  "button": true,
  "tempC": 27.31
}
```

Fallback if POST fails:
- Method: `GET`
- URL: `host_url/api/<deviceId>/set?led=1&buzz=0&button=1&tempC=27.31`

### Sync Timing

Current firmware constants:
- Pull interval: `500 ms`
- Minimum push interval: `200 ms`

## Optional PHP Remote API (No DB)

This repo now includes a simple PHP file-based backend:
- [server-api/index.php](server-api/index.php)
- [devices](devices) (per-device JSON state files)
- [server-api/.htaccess](server-api/.htaccess) (route rewriting to `index.php`)

Supported routes:
- `GET /api/<deviceId>`: read state
- `GET /api/<deviceId>/set?...`: update state
- `POST /api/<deviceId>/set`: update state (`application/json` or form data)

Examples:

```bash
curl "http://your-host/api/iot-block-001"
curl "http://your-host/api/iot-block-001/set?led=1&buzz=0&button=1&tempC=27.31"
curl -X POST "http://your-host/api/iot-block-001/set" \
  -H "Content-Type: application/json" \
  -d '{"led":true,"buzzer":false,"button":true,"tempC":27.31}'
```

Notes:
- Only routes with deviceId are accepted.
- Device IDs are validated (`[A-Za-z0-9-]`, max 64 chars).
- State is persisted in `devices/<deviceId>.json`.

## cURL Examples

### Local state read

```bash
curl "http://192.168.4.1/api/io"
```

### Local state set by GET

```bash
curl "http://192.168.4.1/api/io/set?led=1&buzz=0"
```

### Local state set by POST (form)

```bash
curl -X POST "http://192.168.4.1/api/io/set" \
  -d "led=true" \
  -d "buzz=false"
```

### Discovery

```bash
curl "http://192.168.4.1/api/discovery"
```

## EEPROM Config Schema

Stored structure:
- `magic` (validation marker)
- `ssid[33]`
- `password[65]`
- `serverUrl[129]`

Notes:
- Null-termination is enforced after EEPROM load.
- `magic` mismatch resets config to defaults.

## Security Notes

- AP password is hardcoded (`12345678`) and should be changed for production.
- Setup/API traffic is plain HTTP.
- If using remote sync over the internet, prefer HTTPS endpoint and network protections.

## Troubleshooting

- Cannot connect to router:
  - Verify SSID/password in setup page
  - Reboot after saving settings
  - Check serial logs at 115200 baud
- `/api/io` returns station_not_connected:
  - STA has not connected yet; verify Wi-Fi credentials
- Remote sync not working:
  - Ensure `serverUrl` is non-empty and reachable
  - Check `/api/discovery` remote fields (`remotePullOk`, `remotePushOk`, codes)
  - Ensure remote service supports both pull and set routes

## Known Limitations

- JSON parsing for remote pull is lightweight string-based parsing.
- Local `/api/io` and `/api/io/set` currently require station connection.
- No authentication on local APIs.
