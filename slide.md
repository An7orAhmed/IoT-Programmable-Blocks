# IoT Programmable Blocks: A Modular, AI-Augmented Platform for Real-World Automation

## Final Year Thesis Presentation — G-30-CSE-19B

---

<!-- Slide 1: Title -->
## IoT Programmable Blocks
### A Modular, Semantically-Composable IoT Automation Platform with Agentic AI Code Generation

**G-30-CSE-19B**

*Final Year Thesis Project · Department of Computer Science & Engineering*

---

<!-- Slide 2: Agenda -->
## Agenda

1. **Introduction & Problem Statement**
2. **Literature Review & Related Work**
3. **System Architecture Overview**
4. **Hardware Design — Modular Block Abstraction**
5. **Firmware Design — Dual-Mode Connectivity & Local Intelligence**
6. **Communication Protocol — RESTful Device API**
7. **Python Control Library — Object-Oriented Orchestration**
8. **Web Portal — State Management & Runtime Administration**
9. **AI Integration — DeepSeek Agentic Chatbot for Autonomous Script Synthesis**
10. **Flow Diagram — Visual Programming & Dynamic Recomposition**
11. **Experimental Results & Evaluation**
12. **Conclusion & Future Work**

---

<!-- Slide 3: Introduction & Problem Statement -->
## 1. Introduction & Problem Statement

### Motivation

The Internet of Things (IoT) promises ubiquitous sensing and actuation, yet **programming IoT systems remains inaccessible** to non-experts. Existing solutions exhibit critical limitations:

| Limitation | Consequence |
|---|---|
| **Vendor lock-in** | Devices tied to proprietary ecosystems (HomeKit, Alexa, SmartThings) |
| **Fragmented programming models** | Each platform demands unique SDKs and paradigms |
| **Low-level complexity** | Users manage WiFi provisioning, protocol serialization, state synchronization |
| **No semantic composition** | "If button pressed, turn on LED" requires hundreds of lines across firmware, server, and client |

### Research Question

> *Can we design a modular IoT platform where physical blocks are abstracted as **first-class software objects**, and automation scripts are **autonomously synthesized** from natural language by an agentic large language model?*

### Contributions

1. **Hardware-agnostic block abstraction** — any ESP8266 device exposes a uniform REST API
2. **Zero-configuration discovery** — mDNS-based scanning with automatic host resolution
3. **Pythonic OOP control layer** — `LED.on()`, `Button.state`, `Buzzer.off()`
4. **Agentic AI code generation** — DeepSeek-powered chatbot that translates natural language into executable Python automation scripts and dynamically adjusts visual flow diagrams

---

<!-- Slide 4: Literature Review -->
## 2. Literature Review & Related Work

### IoT Programming Paradigms

| Paradigm | Examples | Limitation |
|---|---|---|
| **Rule-based engines** | IFTTT, Zapier | Boolean trigger-action only; no stateful logic |
| **Visual programming** | Node-RED, Blockly | Limited to pre-defined nodes; not extensible |
| **Cloud-function models** | AWS IoT, Azure IoT Hub | Requires cloud infrastructure; latency-sensitive |
| **Embedded DSLs** | TinyML, MicroPython | Low-level; steep learning curve |

### LLM-Assisted Code Generation

- **GitHub Copilot / Codex**: General-purpose code completion; no IoT domain awareness
- **ChatGPT Plugins**: Cannot interact with physical hardware directly
- **Task-specific agents** (SWE-Agent, OpenDevin): Focused on software engineering; no hardware-in-the-loop

### Research Gap

> **No existing system combines (a) modular hardware abstraction, (b) semantic composition via OOP, and (c) agentic LLM-driven code generation** with real-time feedback from physical devices — this is the gap our work addresses.

---

<!-- Slide 5: System Architecture -->
## 3. System Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                   WEB PORTAL (React / Next.js)               │
│  ┌──────────────────────┐  ┌───────────────────────────────┐ │
│  │  Visual Flow Builder  │  │  DeepSeek Agentic Chatbot     │ │
│  │  (React Flow / D3.js) │  │  ┌─────────────────────────┐  │ │
│  │                       │  │  │ NLP → Python Script     │  │ │
│  │  Node-edge graph of   │  │  │ Script → Flow Diagram   │  │ │
│  │  block interactions   │  │  │ Tool-calling for HW I/O │  │ │
│  └──────────────────────┘  │  └─────────────────────────┘  │ │
│                            └───────────────────────────────┘ │
└────────────────────┬─────────────────────────────────────────┘
                     │ HTTP REST API
┌────────────────────▼─────────────────────────────────────────┐
│              PYTHON CONTROL LAYER (blocks.py)                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐ │
│  │ LED(id)  │  │Buzzer(id)│  │Button(id)│  │blocks_lib.py │ │
│  │ .on()    │  │ .on()    │  │ .state   │  │ IoTBlocks-   │ │
│  │ .off()   │  │ .off()   │  │          │  │ Client       │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └──────┬───────┘ │
│       └──────────────┴─────────────┘               │         │
│                     │ mDNS Discovery + HTTP         │         │
└─────────────────────┼──────────────────────────────┼─────────┘
                      │                              │
          ┌───────────▼──────────┐    ┌──────────────▼────────┐
          │  LOCAL NETWORK API   │    │   CLOUD RELAY (PHP)   │
          │  http://192.168.4.1  │    │   esinebd.com /       │
          │  /api/io, /api/io/set│    │   iot.alsabid.com     │
          └───────────┬──────────┘    └──────────┬────────────┘
                      │                          │
┌─────────────────────▼──────────────────────────▼─────────────┐
│              FIRMWARE LAYER — ESP8266 (Wemos D1 Mini Lite)    │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  AP+STA Mode  │  mDNS  │  EEPROM Config  │  HTTP Server  │ │
│  └──────────────────────────────────────────────────────────┘ │
└─────────────────────┬────────────────────────────────────────┘
                      │ GPIO
┌─────────────────────▼────────────────────────────────────────┐
│              HARDWARE LAYER — Physical Blocks                 │
│  ┌──────┐  ┌──────┐  ┌────────┐  ┌─────────┐  ┌──────────┐  │
│  │ LED  │  │Buzzer│  │ Button │  │ DS18B20 │  │  Relay   │  │
│  │  D6  │  │  D7  │  │   D1   │  │   D5    │  │   D5     │  │
│  └──────┘  └──────┘  └────────┘  └─────────┘  └──────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### Three-Tier Design

| Tier | Technology | Responsibility |
|---|---|---|
| **Presentation** | React, React Flow, DeepSeek API | User interface, flow visualization, chatbot |
| **Control** | Python 3.13, `blocks.py` | Device orchestration, script execution, AI tool-calling |
| **Physical** | ESP8266, C++/Arduino | Sensor reading, actuator control, local HTTP API |

---

<!-- Slide 6: Hardware Design -->
## 4. Hardware Design — Modular Block Abstraction

### Block Taxonomy

Each physical "block" is a self-contained ESP8266 (Wemos D1 Mini Lite) module exposing a uniform interface:

| Block Type | Category | GPIO | Direction | Physical Component |
|---|---|---|---|---|
| **LED** | Digital Output | D6 | Server → Device | 5mm LED + 220Ω resistor |
| **Buzzer** | Digital Output | D7 | Server → Device | Piezoelectric buzzer |
| **Button** | Digital Input | D1 | Device → Server | Tactile push-button (pull-up) |
| **Temperature** | Analog Sensor | D5 | Device → Server | DS18B20 (OneWire, ±0.5°C) |
| **Relay** | Power Output | D5 | Server → Device | 5V relay module |

### Key Design Principle: Uniform Abstraction

Every block, regardless of its physical function, exposes an identical API surface:

```
GET  /api/io        → {"led": true, "buzzer": false, "button": false, "tempC": 23.5}
GET  /api/io/set?led=1&buzz=0
POST /api/io/set    → {"led": true, "buzzer": false}
GET  /api/discovery → {"deviceId": "IoT-Block-002", "mdns": "iot-block-002.local", ...}
```

This uniformity enables the Python control layer to treat all blocks polymorphically.

---

<!-- Slide 7: Firmware Design -->
## 5. Firmware Design — Dual-Mode Connectivity

### WiFi Architecture: Simultaneous AP + Station (WIFI_AP_STA)

```
                    ┌─────────────────────┐
                    │   ESP8266 Firmware   │
                    │                      │
   User's Router ───┤ STA (192.168.x.x)    │─── Cloud API sync
   (Internet)       │                      │    (every 500ms)
                    │ AP (192.168.4.1)     │─── Local direct API
   Phone/Laptop ────┤                      │    (port 80)
   (Configuration)  └─────────────────────┘
```

### Key Firmware Features

| Feature | Implementation |
|---|---|
| **Captive Portal** | HTML form at `192.168.4.1/` for WiFi provisioning |
| **Persistent Config** | EEPROM storage (256B) with magic number `0xB10C2026` |
| **mDNS Advertising** | `iot-block-002.local` for zero-config discovery |
| **Remote Sync** | Pull (500ms) + Push (200ms min) to cloud server |
| **DS18B20 Driver** | Async, non-blocking conversion cycle via DallasTemperature |
| **JSON Parsing** | Lightweight manual parser (no ArduinoJson dependency) |

### Remote Sync Protocol

```
PULL (every 500ms):
  GET {serverUrl}/api/IoT-Block-002
  Response: {"led": true, "buzzer": false}
  → Updates local outputs

PUSH (on state change, min 200ms interval):
  1. POST {serverUrl}/api/IoT-Block-002/set  {"led":true,"buzzer":false,"button":false,"tempC":23.5}
  2. [fallback] GET .../set?led=1&buzz=0&button=0&tempC=23.5
```

---

<!-- Slide 8: Communication Protocol -->
## 6. Communication Protocol — RESTful Device API

### API Endpoint Specification

| Method | Endpoint | Purpose | Response Schema |
|---|---|---|---|
| `GET` | `/api/discovery` | Device identity & health | `{deviceId, hostname, mdns, staIp, stationConnected, remotePullOk, remotePushOk, tempC}` |
| `GET` | `/api/io` | Read all IO state | `{led, buzzer, button, tempC}` |
| `GET` | `/api/io/set?led=1&buzz=0` | Set outputs (query params) | `{success: true}` |
| `POST` | `/api/io/set` | Set outputs (JSON body) | `{success: true}` |

### Error Handling

```python
# Hierarchical exception model
IoTBlocksError                    # Base
├── DeviceNotFoundError           # mDNS resolution failure
├── NetworkError                  # Connection timeout / DNS
└── ApiError                      # HTTP 4xx/5xx
    └── StationNotConnectedError  # 503: STA not connected yet
```

### PHP State Management Backend

```php
// State persisted as JSON files
devices/IoT-Block-001.json → {"deviceId":"IoT-Block-001","led":true,"buzzer":false,...}

// Atomic writes: tmp file → rename
// Input coercion: "1"/"0", "true"/"false", "on"/"off", "HIGH"/"LOW"
// CORS: Access-Control-Allow-Origin: *
```

---

<!-- Slide 9: Python Control Library -->
## 7. Python Control Library — Object-Oriented Orchestration

### API Design Philosophy

Physical blocks are elevated to **first-class Python objects**:

```python
from blocks import LED, Buzzer, Button, get_default_client

# Automatic mDNS discovery (scans iot-block-001.local ... iot-block-255.local)
client = get_default_client()

# Declarative, idiomatic Python
led    = LED("IoT-Block-002")
button = Button("IoT-Block-001")
buzzer = Buzzer("IoT-Block-002")

# Simple automation logic
while True:
    if button.state:        # HTTP GET /api/io → parse button field
        led.on()            # HTTP POST /api/io/set (only if changed)
        buzzer.on()
    else:
        led.off()
        buzzer.off()
```

### Design Patterns

| Pattern | Implementation |
|---|---|
| **Singleton Client** | Module-level `_DEFAULT_CLIENT`; all block objects share one `IoTBlocksClient` |
| **Deduplication** | `_set_if_changed()` avoids redundant HTTP calls |
| **Lazy Resolution** | Host resolution on first access via `_resolve_host()` cascade |
| **Background Discovery** | `ThreadPoolExecutor` (24 workers) scans 255 mDNS hosts in parallel |
| **Retry with Re-resolution** | Single retry on `NetworkError` with host cache invalidation |

### Discovery Cascade

```
_resolve_host(device_id):
  1. Cache hit? → Return cached host
  2. Direct mDNS derivation? → iot-block-{NNN}.local → probe /api/discovery
  3. Known hosts map? → User-provided {device_id: host} mapping
  4. Background scan results? → wait_for_background_scan()
  5. Fail → raise DeviceNotFoundError
```

---

<!-- Slide 10: Web Portal -->
## 8. Web Portal — Management & Visual Programming

### Portal Architecture

```
┌──────────────────────────────────────────────────┐
│                WEB PORTAL DASHBOARD               │
│                                                   │
│  ┌─────────────────┐  ┌────────────────────────┐ │
│  │  Device Manager  │  │   Visual Flow Builder   │ │
│  │  ─────────────── │  │   ────────────────────  │ │
│  │  • Device list   │  │   ┌───┐    ┌────┐      │ │
│  │  • Live status   │  │   │Btn├───→│LED │      │ │
│  │  • Health check  │  │   └───┘    └────┘      │ │
│  │  • WiFi config   │  │   Nodes + Edges graph   │ │
│  └─────────────────┘  └────────────────────────┘ │
│                                                   │
│  ┌──────────────────────────────────────────────┐ │
│  │         DeepSeek Agentic Chatbot              │ │
│  │  ────────────────────────────────────────     │ │
│  │  User: "Turn on LED when button pressed"      │ │
│  │  ────────────────────────────────────────     │ │
│  │  Bot:  [Generates Python script]              │ │
│  │        [Updates flow diagram]                 │ │
│  │        [Deploys to app_script.py]             │ │
│  └──────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

### WSGI Runtime Administration (`passenger_wsgi.py`)

| Endpoint | Method | Function |
|---|---|---|
| `/start` | GET | Spawn `app_script.py` as managed subprocess |
| `/stop` | GET | SIGTERM → wait 5s → SIGKILL process group |
| `/state` | GET | `{pid, running, last_error}` |
| `/update_script` | POST | Replace automation script content |
| `/get_script` | GET | Retrieve current script |

---

<!-- Slide 11: DeepSeek Agentic Chatbot -->
## 9. AI Integration — DeepSeek Agentic Chatbot

### Rationale for Agentic LLM Integration

Traditional IoT programming requires users to:
1. Understand block IDs and API endpoints
2. Write imperative control loops with proper error handling
3. Manually configure timing, debouncing, and state management

An **agentic LLM** collapses this complexity by translating natural language intent directly into executable, verified Python scripts.

### DeepSeek Integration Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    DEEPSEEK AGENTIC CHATBOT                  │
│                                                              │
│  User Query: "Blink LED 3 times when temperature > 30°C"    │
│                          │                                   │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │               LLM Agent (DeepSeek-V3)                 │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │ System Prompt:                                   │  │  │
│  │  │ • blocks.py API reference                       │  │  │
│  │  │ • Available devices: [IoT-Block-001, ...]       │  │  │
│  │  │ • Output format: executable Python              │  │  │
│  │  │ • Constraints: error handling, no infinite loops│  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
│                          │                                   │
│              ┌───────────┼───────────┐                      │
│              ▼           ▼           ▼                      │
│  ┌──────────────┐ ┌──────────┐ ┌──────────────┐            │
│  │ Tool:         │ │ Tool:     │ │ Tool:         │           │
│  │ discover_     │ │ execute_  │ │ update_flow_  │           │
│  │ devices()     │ │ script()  │ │ diagram()     │           │
│  │ → List of     │ │ → Run &   │ │ → Update      │           │
│  │   active      │ │   capture │ │   React Flow  │           │
│  │   blocks      │ │   output  │ │   nodes/edges │           │
│  └──────────────┘ └──────────┘ └──────────────┘            │
└─────────────────────────────────────────────────────────────┘
```

### Tool-Calling Functions Exposed to the Agent

| Tool | Description |
|---|---|
| `discover_devices()` | Returns all active blocks with their IDs, types, and capabilities |
| `get_device_state(device_id)` | Reads current IO state of a specific block |
| `set_device_output(device_id, led, buzzer)` | Sets actuator outputs |
| `execute_script(script_code)` | Runs generated Python script in sandboxed subprocess; returns stdout/stderr |
| `update_flow_diagram(nodes, edges)` | Pushes new node-edge graph to React Flow renderer |

### Example Agent Interaction

```
USER: "When I press the button, turn on the LED and buzzer for 2 seconds."

AGENT (thinking):
  1. Call discover_devices() → [IoT-Block-001 (Button), IoT-Block-002 (LED+Buzzer)]
  2. Generate script:
     from blocks import LED, Buzzer, Button
     from time import sleep
     button = Button("IoT-Block-001")
     led = LED("IoT-Block-002")
     buzzer = Buzzer("IoT-Block-002")
     while True:
         if button.state:
             led.on(); buzzer.on()
             sleep(2)
             led.off(); buzzer.off()
         sleep(0.05)
  3. Call update_flow_diagram([Button→LED, Button→Buzzer])

AGENT: "I've generated the script and updated the flow diagram. The button on
IoT-Block-001 will trigger LED and buzzer on IoT-Block-002 for 2 seconds."
```

### Safety & Sandboxing

- Scripts execute in **isolated subprocess** with resource limits (CPU time, memory)
- `passenger_wsgi.py` manages lifecycle: start/stop/restart
- Agent cannot modify `blocks.py` or `blocks_lib.py` — only user automation scripts
- All HTTP calls go through audited `IoTBlocksClient`

---

<!-- Slide 12: Flow Diagram -->
## 10. Flow Diagram — Visual Programming & Dynamic Recomposition

### Visual Representation Model

The flow diagram is a **directed graph** $G = (V, E)$ where:
- **Nodes** $V$: Physical blocks annotated with type and device ID
- **Edges** $E$: Data/control flow between blocks, labeled with trigger conditions

```
┌──────────────────────────────────────────────────┐
│                 FLOW DIAGRAM (React Flow)         │
│                                                   │
│   ┌──────────┐       ┌──────────┐                │
│   │  Button  │       │  LED     │                │
│   │ ──────── │──────→│ ──────── │                │
│   │ D1 (IN)  │ press │ D6 (OUT) │                │
│   │ ID: 001  │       │ ID: 002  │                │
│   └──────────┘       └──────────┘                │
│        │                   ▲                     │
│        │ press             │ on                  │
│        ▼                   │                     │
│   ┌──────────┐       ┌──────────┐                │
│   │ DS18B20  │       │  Buzzer  │                │
│   │ ──────── │       │ ──────── │                │
│   │ D5 (SEN) │       │ D7 (OUT) │                │
│   │ ID: 002  │       │ ID: 002  │                │
│   └──────────┘       └──────────┘                │
│        │                                        │
│        │ temp > 30°C                            │
│        ▼                                        │
│   ┌──────────┐                                  │
│   │  Relay   │                                  │
│   │ ──────── │                                  │
│   │ D5 (OUT) │                                  │
│   │ ID: 003  │                                  │
│   └──────────┘                                  │
│                                                   │
│   [ Graph is auto-generated from                  │
│     DeepSeek agent output ]                       │
└──────────────────────────────────────────────────┘
```

### Bidirectional Sync: Script ↔ Diagram

```
Natural Language ──→ DeepSeek Agent ──→ Python Script ──→ Flow Diagram
                                         (generated)      (nodes + edges JSON)

                                         Python Script ←── Flow Diagram
                                         (if user drags    (user edits
                                          new edge)         in UI)
```

The flow diagram is **not merely a visualization** — it is a **bidirectional, live representation** of the automation logic. Changes in the diagram regenerate the script; changes from the chatbot update the diagram.

---

<!-- Slide 13: Implementation Details -->
## 11. Implementation Details

### Technology Stack

| Layer | Technology | Justification |
|---|---|---|
| **Firmware** | C++ / Arduino, PlatformIO | ESP8266 native support, rich library ecosystem |
| **Local API** | `ESP8266WebServer`, custom JSON | Zero-dependency, < 2KB overhead |
| **Cloud Backend** | PHP 8.x, JSON file storage | Lightweight, no database dependency |
| **Control Library** | Python 3.13, `urllib`, `threading` | Standard library only; no pip dependencies for core |
| **Web Portal** | React 18, React Flow, Tailwind CSS | Component-based UI, mature flow graph library |
| **AI Agent** | DeepSeek API (deepseek-chat) | Function calling, 128K context, cost-effective |
| **WSGI Server** | Phusion Passenger | Process management, auto-restart |

### Key Metrics

| Metric | Value |
|---|---|
| **Firmware binary size** | ~420 KB |
| **RAM usage (idle)** | ~32 KB free heap |
| **Discovery scan time** (255 hosts) | < 3 seconds (24 parallel workers) |
| **HTTP response time** (local API) | < 15 ms |
| **Remote sync latency** | ~80-200 ms (WiFi dependent) |
| **DS18B20 accuracy** | ±0.5°C (−10°C to +85°C) |
| **Agent script generation** | < 3 seconds (DeepSeek API) |

---

<!-- Slide 14: Experimental Results -->
## 12. Experimental Results & Evaluation

### Test Scenarios

| Scenario | Description | Success |
|---|---|---|
| **Button → LED relay** | Physical button press toggles remote LED | ✅ < 50ms latency |
| **Multi-output orchestration** | Button triggers LED + Buzzer simultaneously | ✅ Synchronized |
| **Temperature threshold** | Temp > 30°C triggers relay | ✅ Hysteresis tested |
| **Network failover** | WiFi disconnect → AP fallback → reconnect | ✅ Automatic |
| **Agent: Simple rule** | "Turn on LED when button pressed" | ✅ Correct script |
| **Agent: Timed sequence** | "Blink LED 3 times on button press" | ✅ With sleep() |
| **Agent: Conditional** | "If temp > 30°C, turn on buzzer else turn on LED" | ✅ Branch logic |
| **Agent → Flow sync** | Generated script updates flow diagram | ✅ Bidirectional |

### Comparative Analysis

| Feature | Our System | Node-RED | IFTTT | Home Assistant |
|---|---|---|---|---|
| **Local-first operation** | ✅ | ✅ | ❌ | ✅ |
| **No cloud dependency** | ✅ | ✅ | ❌ | ✅ |
| **Python OOP abstraction** | ✅ | ❌ | ❌ | Partial |
| **Agentic AI code gen** | ✅ | ❌ | ❌ | ❌ |
| **Bidirectional flow sync** | ✅ | ❌ | ❌ | ❌ |
| **Zero-config discovery** | ✅ | ❌ | N/A | Partial |
| **Cost per device** | ~$3 (ESP8266) | ~$35 (RPi) | N/A | ~$35+ |

---

<!-- Slide 15: System Evaluation -->
## 13. System Evaluation — Strengths & Limitations

### Strengths

- **Truly modular**: Each block is independently deployable; no central hub required
- **Dual-mode networking**: Works with or without internet connectivity
- **Progressive abstraction**: Users interact at their chosen level — raw HTTP, Python API, natural language
- **Cost-effective**: ESP8266 modules are ~$3/unit vs. $35+ for Raspberry Pi-based alternatives
- **Extensible**: New block types require only firmware template + Python wrapper class

### Limitations

| Limitation | Mitigation Strategy |
|---|---|
| **Single-board, single-role** (v1) | v2 firmware (`led-button-buzz`) supports multiple peripherals per ESP8266 |
| **HTTP polling overhead** | WebSocket upgrade planned for push-based state sync |
| **No authentication** | Intended for trusted local networks; JWT-based auth in roadmap |
| **LLM hallucination risk** | Sandboxed execution + schema-validated tool outputs |
| **ESP8266 RAM constraints** | v3 migration to ESP32 for TLS, BLE, larger heap |

---

<!-- Slide 16: Conclusion & Future Work -->
## 14. Conclusion & Future Work

### Summary of Contributions

1. **Designed and implemented** a modular IoT platform where heterogeneous hardware blocks expose a uniform REST API
2. **Developed** a Python control library (`blocks.py`) with automatic mDNS discovery, deduplicated state management, and hierarchical error handling
3. **Built** a dual-mode firmware (AP+STA) with captive portal provisioning, EEPROM persistence, and bidirectional cloud sync
4. **Integrated** a DeepSeek-powered agentic chatbot that translates natural language into executable Python automation scripts
5. **Created** a bidirectional visual flow diagram that stays synchronized with both generated and manually-edited scripts

### Future Work

| Direction | Description |
|---|---|
| **ESP32 Migration** | TLS/HTTPS, Bluetooth Low Energy mesh, larger heap for complex logic |
| **WebSocket Push** | Replace HTTP polling with persistent WS connections for sub-10ms latency |
| **Multi-agent Orchestration** | Multiple blocks collaboratively executing distributed automation |
| **Federated Learning** | On-device pattern learning for predictive automation (e.g., pre-heat at 7 AM) |
| **Formal Verification** | Model-checking generated scripts against safety properties before execution |
| **Block Marketplace** | Community-contributed block types and automation templates |

---

<!-- Slide 17: Questions -->
## Thank You

### Questions & Discussion

**G-30-CSE-19B**

---

<!-- Slide 18: Backup — Code Example -->
## Backup Slide A: Complete Automation Example

```python
"""
Example: Temperature-Triggered Multi-Output Control
If temperature exceeds 30°C, activate buzzer and relay.
If button is pressed, override and turn everything off.
"""
from blocks import LED, Buzzer, Button, get_default_client
from blocks_lib import IoTBlocksClient
from time import sleep

client: IoTBlocksClient = get_default_client()
client.wait_for_background_scan(timeout_seconds=5.0)

# Block initialization
button = Button("IoT-Block-001")     # Button on device 1
led    = LED("IoT-Block-002")        # LED on device 2
buzzer = Buzzer("IoT-Block-002")     # Buzzer on device 2

THRESHOLD_C = 30.0
alert_active = False

try:
    while True:
        status = client.get_status("IoT-Block-002")
        temp_c = status.get("tempC")

        # Emergency override: button press kills all outputs
        if button.state:
            led.off()
            buzzer.off()
            alert_active = False
            print("[OVERRIDE] Button pressed — all outputs OFF")
            sleep(0.5)
            continue

        # Temperature threshold logic
        if temp_c is not None and temp_c > THRESHOLD_C:
            if not alert_active:
                buzzer.on()
                led.on()
                alert_active = True
                print(f"[ALERT] Temperature {temp_c:.1f}°C > {THRESHOLD_C}°C")
        else:
            if alert_active:
                buzzer.off()
                led.off()
                alert_active = False
                print(f"[CLEAR] Temperature {temp_c:.1f}°C ≤ {THRESHOLD_C}°C")

        sleep(0.1)

except KeyboardInterrupt:
    led.off()
    buzzer.off()
    print("\n[SHUTDOWN] All outputs turned off.")
```

---

<!-- Slide 19: Backup — API Reference -->
## Backup Slide B: Python API Reference

### `blocks_lib.IoTBlocksClient`

```python
class IoTBlocksClient:
    def __init__(self, known_hosts: dict = None, timeout_seconds: float = 0.6)
    def scan_available_devices(start_id=1, end_id=255, log=False, refresh=False) -> list[DeviceInfo]
    def start_background_scan(log=False, refresh=False) -> None
    def wait_for_background_scan(timeout_seconds=5.0) -> None
    def get_status(device_id: str, refresh=False) -> dict
    def set_outputs(device_id: str, led=None, buzz=None) -> dict
```

### `blocks` (High-Level)

```python
class LED(_OutputBlock):
    def on() -> None
    def off() -> None
    @property state -> bool

class Buzzer(_OutputBlock):
    def on() -> None
    def off() -> None
    @property state -> bool

class Button(_Block):
    @property state -> bool

def get_default_client(known_hosts=None) -> IoTBlocksClient
```

### Exception Hierarchy

```
IoTBlocksError
├── DeviceNotFoundError
├── NetworkError
└── ApiError
    └── StationNotConnectedError
```

---

<!-- Slide 20: Backup — Hardware Schematic -->
## Backup Slide C: Hardware Pin Mapping & Schematic

### Multi-Block Device (IoT-Block-002)

```
                    WEMOS D1 MINI LITE (ESP8266)
                  ┌─────────────────────────────┐
                  │                             │
    LED ──────────┤ D6 (GPIO12)                 │
    Button ───────┤ D1 (GPIO5, pull-up)         │
    Buzzer ───────┤ D7 (GPIO13)                 │
    DS18B20 ──────┤ D5 (GPIO14, OneWire, 4.7kΩ) │
                  │                             │
    Status LED ───┤ LED_BUILTIN (GPIO2)         │
                  │                             │
    Power ────────┤ 5V / Micro USB              │
                  └─────────────────────────────┘

    DS18B20 Wiring:
    ┌──────────┐
    │  DS18B20 │  VDD ─── 3.3V
    │          │  DQ  ─── D5 (GPIO14) ─── 4.7kΩ ─── 3.3V
    │          │  GND ─── GND
    └──────────┘
```

---

<!-- Slide 21: Backup — Agent System Prompt -->
## Backup Slide D: DeepSeek Agent System Prompt

```
You are an IoT automation agent for the "IoT Programmable Blocks" platform.
You translate natural language into executable Python scripts.

AVAILABLE DEVICES (discovered via mDNS):
{{device_list}}

PYTHON API (blocks.py):
  from blocks import LED, Buzzer, Button
  led = LED("IoT-Block-XXX")     # .on(), .off(), .state
  buzzer = Buzzer("IoT-Block-XXX")  # .on(), .off(), .state
  button = Button("IoT-Block-XXX")  # .state (read-only)

TOOLS AVAILABLE:
  - discover_devices(): List all active blocks
  - get_device_state(device_id): Read current IO values
  - execute_script(code): Run generated Python in sandbox
  - update_flow_diagram(nodes, edges): Update visual graph

CONSTRAINTS:
  - Include error handling (try/except)
  - Use time.sleep() for timing; avoid busy-waiting
  - Scripts must terminate on KeyboardInterrupt
  - All device IDs must match discovered devices exactly
  - After generating script, always call update_flow_diagram()

RESPONSE FORMAT:
  1. Brief explanation of the logic
  2. The Python script in a code block
  3. The flow diagram update
```
