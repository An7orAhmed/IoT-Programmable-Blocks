Okay, I need to create a README for this IoT project based on the user's instructions. Let me start by understanding the requirements.

The project is named "15. IoT Real World Programmable Blocks," and I need a project title within 50 characters. Maybe "IoT Programmable Blocks for Real-World Automation" fits and is descriptive enough.

Next, the detailed description. The provided code snippets include Temp, LED, Button, Buzzer, and Relay, which are different sensor and actuator modules. Each connects to WiFi and communicates with a server. So the description should mention that it's a collection of modular components enabling IoT automation with real-time data handling.

The user doesn't want contribution or license info, so skip those sections. They want a pinmap if possible. Looking at the code snippets, each uses D5 as the main pin, but in the actual project, other pins might be used for different modules. The note about diagrams not being accurate is important, so I'll include that in the pinmap section.

The PDF files listed need to be included. Since they're in Google Drive paths, maybe just list their names under "Documentation" as related documents. But since the user is creating a README, it's better to mention them as available resources without links.

Code structure: Each .ino file seems to be a separate sketch for different components. The Temp.ino uses a DHT sensor, while others like LED.ino and Relay.ino control outputs based on server responses. The setup and loop functions show they connect to WiFi and interact with the server.

Pinmap section: I'll list the common GPIO pins used. Since all code uses D5, but in reality, different pins are likely assigned. I need to make a table, maybe with Module and GPIO Pins columns. For example, Temp Sensor (DHT11) on D5, LED on D5, etc. But since each example uses D5, I should clarify that in a note, stating the pin assignments might vary, so users should check their hardware setup.

Including the note about the diagram's accuracy is important. Maybe under the pinmap, add a note that actual wiring may differ.

Documentation section: List the PDFs as Project Proposal, Reports, and Books. Use the filenames provided. Use the base filenames from the paths given, like "Project Proposal G-30-CSE-19B.pdf," "Book Antor.pdf," etc.

Features section: The main features are WiFi connection with failover, real-time data sync, support for multiple devices like sensors and actuators, and modular code structure.

Setup instructions: Mention updating WiFi credentials, server details, dependencies like ESP8266 and DHT library, and compiling the code for the specific module.

The code examples can be summarized in a section, showing the structure of each .ino file, but perhaps not including the full code here since snippets are already provided.

Make sure not to mention "contribution" or "license" anywhere, as per the user's instruction.

Double-check all the sections are included: Title, Description, Features, Documentation, Pin Mapping, Code Structure, Setup, and PDF list.

Okay, structuring it step by step now.
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