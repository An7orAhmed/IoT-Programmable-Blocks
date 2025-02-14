# IoT Programmable Blocks for Real-World Automation  

## Project Description  
This project provides a collection of modular IoT components (sensors, actuators) designed to interact with a central server for real-time automation and monitoring. Built on the ESP8266 platform, each block (temperature sensor, LED, button, buzzer, relay) connects to WiFi and communicates over HTTP to sync data or execute commands. Users can monitor environmental data (e.g., temperature) or remotely control devices (lights, relays) through a web interface.  

---

## Features  
1. **Multi-WiFi Support**: Connects to multiple predefined WiFi networks for failover.  
2. **Real-Time Updates**: Sensors transmit data on change (`Temp.ino`, `Button.ino`); actuators fetch commands periodically (`LED.ino`, `Relay.ino`).  
3. **Device Agnostic**: Modular code structure allows easy extension for new devices.  
4. **HTTP-Based API**: Integrates with `esinebd.com` backend for centralized control.  

---

## Documentation  
The following documents provide design and implementation details:  
- **Project Proposal**: `Project Proposal G-30-CSE-19B.pdf`  
- **Technical Reports**:  
  - `Antor Report.pdf`  
  - `IoT Programmable Block Report.pdf`  
  - `Sabid Report.pdf`  
- **Supplementary Books**:  
  - `Book Antor.pdf`  
  - `Book Sabid.pdf`  

---

## Pin Mapping (ESP8266)  
| Module         | GPIO Pins        | Function                       |  
|----------------|------------------|--------------------------------|  
| DHT11 Sensor   | Connect to `D5`  | Temperature/Humidity Sensing   |  
| LED            | `D5` (Output)    | Remote-Controlled Light        |  
| Button         | `D5` (Input)     | Physical Input Trigger         |  
| Buzzer/Relay   | `D5` (Output)    | Actuator Control               |  

**Note**: Pin assignments in code snippets are simplified. Actual wiring may vary. Verify connections using schematics in project reports.  

---

## Code Structure  
### Key Components:  
1. **Temp.ino**  
   - Reads temperature/humidity from DHT11.  
   - Sends updates to server only when values change.  

2. **LED.ino / Buzzer.ino / Relay.ino**  
   - Polls server for latest command (`?get` request).  
   - Updates output pins (e.g., turn on/off LED) based on response.  

3. **Button.ino**  
   - Detects physical button presses.  
   - Transmits state changes (`HIGH`/`LOW`) to the server.  

### Shared Logic:  
- **WiFi Multi-Connection**: Prioritizes predefined networks (e.g., `Hello`, `GameOn`).  
- **HTTP Client**: Constructs dynamic URLs with user credentials and device states.  

---

## Setup  
1. **Update Credentials**: Replace `ssid1`, `pass1`, `user`, and `pass` in code with your WiFi/account details.  
2. **Server Configuration**: Ensure the `host` URL points to your backend endpoint.  
3. **Dependencies**: Install libraries:  
   ```bash  
   ESP8266WiFi          # Built-in  
   ESP8266HTTPClient    # Built-in  
   DHT_Sensor_Library   # Install via IDE Library Manager  
   ```  
4. **Upload Code**: Compile and flash the appropriate `.ino` file for each device.  

--- 

> **Important**: Diagrams or pin references in documentation may not match actual hardware. Verify configurations before deployment.
