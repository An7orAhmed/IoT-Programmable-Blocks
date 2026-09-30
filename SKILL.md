# IoT Programmable Blocks — Project Quick-Start Skill

## Overview
Modular IoT automation platform for final year thesis (G-30-CSE-19B). ESP8266-based hardware "blocks" (sensors/actuators) expose uniform REST APIs, orchestrated by a Python OOP library, with a web portal and DeepSeek agentic chatbot for autonomous script generation.

## Architecture (3-Tier)

```
Presentation:  React Web Portal + DeepSeek Chatbot + React Flow diagrams
Control:       Python 3.13 (blocks.py, blocks_lib.py) — OOP orchestration
Physical:      ESP8266 (Wemos D1 Mini Lite) — C++/Arduino firmware
```

Communication: HTTP REST + mDNS discovery. Optional cloud relay via PHP backend.

---

## Key Files & Locations

### Firmware (C++ / Arduino / PlatformIO)
| File | Purpose |
|---|---|
| `Blocks/led-button-buzz/src/main.cpp` | **v2 multi-block firmware** — LED, Button, Buzzer, DS18B20 on single ESP8266 |
| `Blocks/led-button-buzz/platformio.ini` | PlatformIO config: `d1_mini_lite`, `espressif8266`, DallasTemperature dep |
| `Blocks/Button/Button.ino` | v1 single-purpose button firmware (sends state on change) |
| `Blocks/LED/LED.ino` | v1 LED firmware (polls server for commands) |
| `Blocks/Buzzer/Buzzer.ino` | v1 buzzer firmware (polls server) |
| `Blocks/Relay/Relay.ino` | v1 relay firmware (polls server) |
| `Blocks/Temp/Temp.ino` | v1 DHT11 temperature firmware (sends on change) |

### Python Control Library
| File | Purpose |
|---|---|
| `Blocks/led-button-buzz/iot-block-v2/blocks.py` | **High-level OOP API**: `LED`, `Buzzer`, `Button` classes, singleton client |
| `Blocks/led-button-buzz/iot-block-v2/blocks_lib.py` | **Low-level HTTP client**: `IoTBlocksClient`, mDNS discovery, retry logic |
| `Blocks/led-button-buzz/iot-block-v2/app.py` | Example launcher (importlib dynamic loading) |
| `Blocks/led-button-buzz/iot-block-v2/example_01.py` | Button → LED relay |
| `Blocks/led-button-buzz/iot-block-v2/example_02.py` | Button → LED + Buzzer |
| `Blocks/led-button-buzz/iot-block-v2/example_03.py` | Email alert on button press (SMTP) |
| `Blocks/led-button-buzz/iot-block-v2/example_04.py` | Blink LED while button held |

### Server / Web API
| File | Purpose |
|---|---|
| `Blocks/led-button-buzz/server-api/index.php` | PHP state management — JSON files per device, RESTful CRUD |
| `Blocks/led-button-buzz/server-api/app_script.py` | Auto-generated automation script (poll-based) |
| `Blocks/led-button-buzz/server-api/passenger_wsgi.py` | WSGI runtime admin: `/start`, `/stop`, `/state`, `/update_script`, `/get_script` |

### Project Prototypes
| File | Purpose |
|---|---|
| `project/Blocks.py` | Earlier Python block abstractions |
| `project/Example 01.py` through `Example 05.py` | Earlier automation examples |

### Documentation
| File | Purpose |
|---|---|
| `README.md` | Project description, pin mapping, setup |
| `slide.md` | **Master's-level thesis presentation** (21 slides, academic framing) |

---

## Hardware Pin Mapping (v2: led-button-buzz)

| Component | GPIO Pin | Notes |
|---|---|---|
| LED | D6 (GPIO12) | Output, active HIGH |
| Button | D1 (GPIO5) | Input, pull-up, active LOW |
| Buzzer | D7 (GPIO13) | Output |
| DS18B20 Temp | D5 (GPIO14) | OneWire, 4.7kΩ pull-up to 3.3V, DallasTemperature lib |
| Board LED | LED_BUILTIN (GPIO2) | Status: blinks 700ms when STA connected |

Device ID: `IoT-Block-002`
AP SSID: `IoT-Block-002`, Password: `12345678`
mDNS: `iot-block-002.local`

---

## Firmware Key Details (main.cpp)

- **WiFi Mode**: `WIFI_AP_STA` — simultaneous AP (192.168.4.1) + Station (router)
- **Captive Portal**: `GET /` → HTML form; `POST /save` → EEPROM persist
- **EEPROM Config**: 256 bytes, magic `0xB10C2026`, struct: `{ssid[33], password[65], serverUrl[129]}`
- **Local HTTP API** (port 80 on AP):
  - `GET /api/discovery` → device identity + health
  - `GET /api/io` → `{led, buzzer, button, tempC}`
  - `GET /api/io/set?led=1&buzz=0` or `POST /api/io/set` with JSON body
- **Remote Sync**:
  - Pull every 500ms: `GET {serverUrl}/api/{deviceId}`
  - Push on change (min 200ms): `POST` first, `GET` fallback
- **Temperature**: Async DallasTemperature, validates range [-100, 150], reports `null` if invalid
- **JSON Parsing**: Manual lightweight parser — NO ArduinoJson dependency
- **Serial Baud**: 115200

---

## Python API Reference

### `blocks_lib.IoTBlocksClient`
```python
client = IoTBlocksClient(known_hosts={}, timeout_seconds=0.6)
client.scan_available_devices(start_id=1, end_id=255, log=False, refresh=False) → list[DeviceInfo]
client.start_background_scan(log=False, refresh=False)          # daemon thread, 24 workers
client.wait_for_background_scan(timeout_seconds=5.0)
client.get_status(device_id, refresh=False) → dict              # {led, buzzer, button, tempC}
client.set_outputs(device_id, led=None, buzz=None) → dict
```

### `blocks` (High-Level OOP)
```python
from blocks import LED, Buzzer, Button, get_default_client

led = LED("IoT-Block-002")        # .on(), .off(), .state (property)
buzzer = Buzzer("IoT-Block-002")  # .on(), .off(), .state (property)
button = Button("IoT-Block-001")  # .state (property, read-only)
client = get_default_client()     # singleton, auto-starts background scan
```

### Exception Hierarchy
```
IoTBlocksError                    # Base
├── DeviceNotFoundError           # mDNS resolution failure
├── NetworkError                  # Timeout / DNS / connection
└── ApiError                      # HTTP 4xx/5xx
    └── StationNotConnectedError  # 503: STA not connected
```

### Design Patterns
- **Singleton client**: Module-level `_DEFAULT_CLIENT` shared across all block objects
- **Deduplication**: `_set_if_changed()` only sends HTTP when state actually differs
- **Lazy resolution**: `_resolve_host()` cascade: cache → mDNS derive → known_hosts → background scan → error
- **Retry with re-resolution**: Single retry on NetworkError with host cache invalidation
- **Background discovery**: `ThreadPoolExecutor(24)` scans `iot-block-001.local` through `iot-block-255.local`

---

## PHP Backend (index.php)

- **Storage**: JSON files in `devices/` directory per device ID
- **State schema**: `{deviceId, led, buzzer, button, tempC, updatedAt}`
- **Atomic writes**: temp file + rename
- **Input coercion**: accepts `"1"/"0"`, `"true"/"false"`, `"on"/"off"`, `"HIGH"/"LOW"`
- **CORS**: `Access-Control-Allow-Origin: *`
- **Routes**: Parsed from `REQUEST_URI` — `/api/devices`, `/api/{deviceId}`, `/api/{deviceId}/set`

---

## DeepSeek Agentic Chatbot (Thesis Novelty)

Architecture:
```
Natural Language → DeepSeek Agent → Python Script → Flow Diagram
                                    (tool-calling)  (React Flow)
```

Agent Tools:
- `discover_devices()` — scan mDNS for active blocks
- `get_device_state(device_id)` — read current IO
- `set_device_output(device_id, led, buzzer)` — set actuators
- `execute_script(code)` — run in sandboxed subprocess
- `update_flow_diagram(nodes, edges)` — push to React Flow

Safety: Sandboxed subprocess, WSGI lifecycle management, no modification of core libs.

---

## Presentation (slide.md)

21-slide academic thesis presentation covering:
1. Problem statement & research question
2. Literature review (Node-RED, IFTTT, Home Assistant, LLM code-gen)
3. 3-tier architecture diagram (ASCII)
4. Hardware block taxonomy
5. Firmware design (AP+STA, mDNS, EEPROM)
6. RESTful API specification
7. Python OOP control library
8. Web portal & WSGI admin
9. **DeepSeek agentic chatbot** (architecture, tool-calling, sandboxing)
10. Bidirectional flow diagram sync
11. Implementation details & metrics
12. Experimental results (8 test scenarios, comparative analysis)
13. Strengths, limitations, future work

---

## Common Commands

```bash
# Python environment (venv in iot-block-v2/)
cd /Users/an7or/MyWork/Embedded/IoT-Programmable-Blocks/Blocks/led-button-buzz/iot-block-v2
source bin/activate

# Run an example
python app.py  # Set EXAMPLE_TO_RUN in app.py

# PlatformIO build/upload
cd /Users/an7or/MyWork/Embedded/IoT-Programmable-Blocks/Blocks/led-button-buzz
pio run
pio run --target upload
```

---

## Key Design Decisions

1. **No ArduinoJson** — manual JSON parser to minimize firmware size
2. **AP+STA mode** — device always reachable for config even without router
3. **Pull + Push sync** — bidirectional with fallback (POST → GET)
4. **Standard library only** for Python core — no pip deps for `blocks_lib.py` (uses `urllib`, `threading`, `json`)
5. **Deduplication at every layer** — firmware tracks `lastObserved*`, Python tracks `_last_state`, PHP uses atomic writes
6. **Progressive abstraction** — raw HTTP → Python API → natural language agent
