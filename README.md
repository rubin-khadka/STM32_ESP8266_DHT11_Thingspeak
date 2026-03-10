# STM32 ESP8266 DHT11 ThingSpeak Project

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![ESP8266](https://img.shields.io/badge/NodeMCU-ESP8266-orange)](https://www.espressif.com/en/products/socs/esp8266)

## Project Overview
This project demonstrates an IoT-based temperature and humidity monitoring system using an STM32 Blue Pill (STM32F103C8T6) microcontroller. The STM32 reads environmental data from a DHT11 sensor and communicates with a NodeMCU ESP8266 module via AT commands. The ESP8266 connects to WiFi and sends the sensor data to ThingSpeak's cloud platform, where users can visualize real-time graphs and indicators. 

While NodeMCU is powerful enough to build complete IoT projects on its own, the objective here is different: **use NodeMCU strictly as a WiFi modem** controlled entirely by an STM32. The NodeMCU runs AT firmware and acts as a slave device - the STM32 sends commands, and the ESP8266 executes them. This approach lets you leverage the STM32's processing power while adding WiFi connectivity through a simple UART interface.

I had this module available, so I used it for demonstration - but the code works with any ESP8266 module running AT firmware.

## Video Demonstrations

https://github.com/user-attachments/assets/a4c1c71f-f1ff-4853-99e9-7ecaf0b4da2e

*Hardware Connection*

https://github.com/user-attachments/assets/199908f6-b6e9-4f4f-9b5d-6dd82351187c

- **Left:** Data received by ThingSpeak showing graph and indicator
- **Right:** UART Output showing device connecting to wifi and showing confirmation that data is sent to ThingSpeak

## Project Schematic

<img width="1305" height="500" alt="Schematic Diagram" src="https://github.com/user-attachments/assets/356bde33-78dd-47ef-86ee-8f5eccd704dd" />

## Hardware Components

| Component | Quantity | Purpose |
|-----------|----------|---------|
| STM32F103C8T6 "Blue Pill" | 1 | Main microcontroller running the application |
| NodeMCU ESP8266 | 1 | WiFi module running AT firmware (acts as modem) |
| DHT11 | 1 | Temperature and humidity sensor |
| ST-Link V2 Programmer | 1 | For flashing/debugging STM32 |
| USB to UART Converter | 1 | For debug output (USART2) |

## Pin Configuration

### STM32 to NodeMCU (USART1)

| STM32 Pin | Function | NodeMCU Pin | Notes |
|-----------|----------|-------------|-------|
| **PA9** | USART1 TX | **RX** | STM32 transmits, NodeMCU receives |
| **PA10** | USART1 RX | **TX** | STM32 receives, NodeMCU transmits |
| **GND** | Ground | **GND** | **Must be connected!** |
| **3.3V** | Power | **VCC** | Optional if NodeMCU is USB-powered |

### DHT11 Sensor

| STM32 Pin | Function | DHT11 Pin | Notes |
|-----------|----------|-------------------|---------|
| **PB13** | GPIO | DATA | Data pin (module already contains pull-up) |
| **3.3V** | Power | **VCC** | Power supply |
| **GND** | Ground | GND | Common ground |

### Debug Output (USART3)

| STM32 Pin | Function | USB-UART Converter | Purpose |
|-----------|----------|-------------------|---------|
| **PB10** | USART3 TX | RX | Send debug messages to PC |
| **PB11** | USART3 RX | TX | (Optional) Receive from PC |
| **GND** | Ground | GND | Common ground |

## ESP8266 Driver

### Driver Architecture
```
┌─────────────────┐
│   Application   │  (main.c)
└────────┬────────┘
         │ API Calls
         ▼
┌─────────────────┐
│   ESP8266 Driver│  (esp8266.c)
│  ┌─────────────┐│
│  │ AT Command  ││
│  │  Engine     ││
│  └─────────────┘│
│  ┌─────────────┐│
│  │ IP Parser   ││
│  └─────────────┘│
│  ┌─────────────┐│
│  │ ThingSpeak  ││
│  │  Client     ││
│  └─────────────┘│
└────────┬────────┘
         │ HAL UART
         ▼
┌─────────────────┐
│    ESP8266      │
│   WiFi Module   │
└─────────────────┘
```

### Driver Workflow

The driver follows a simple but robust workflow:

1. Initialization Phase
    - Test communication with AT command
    - Disable echo with ATE0 for cleaner responses

2. WiFi Connection Phase
    - Set station mode with AT+CWMODE=1
    - Connect to network with AT+CWJAP="SSID","PASSWORD"
    - Wait for WIFI CONNECTED response

3. IP Acquisition Phase
    - Request IP with AT+CIFSR
    - Parse STAIP,"xxx.xxx.xxx.xxx" from response
    - Retry up to 3 times if IP is 0.0.0.0

4. ThingSpeak Communication Phase
    - Establish TCP connection to **api.thingspeak.com:80**
    - Send HTTP GET request with API key and sensor data
    - Parse server response to extract entry ID
    - Close connection after transmission

### ThingSpeak Data Flow
```
STM32 ──[UART]──> ESP8266 ──[WiFi]──> ThingSpeak
   │                  │                    │
   │   AT+CIPSTART    │                    │
   ├─────────────────>│                    │
   │                  ├────[TCP Connect]──>│
   │                  │                    │
   │   AT+CIPSEND     │                    │
   ├─────────────────>│                    │
   │                  ├────[HTTP GET]─────>│
   │                  │                    │
   │                  │<───[Entry ID]──────│
   │   +IPD response  │                    │
   │<─────────────────│                    │
```
### Response Parsing
After sending data to ThingSpeak, the driver parses the HTTP response:
- Looks for `+IPD`, indicating incoming data
- Extracts the entry ID number after the colon
- A positive entry ID confirms successful data upload

### ThingSpeak Rate Limiting
The free version of ThingSpeak allows data updates at a minimum interval of 15 seconds. Sending data more frequently will result in rejection by the server. The main application is configured to respect this limit by sending data every 15 seconds.

🔗 [View ESP8266 Driver Source Code](https://github.com/rubin-khadka/STM32_ESP8266_DHT11_Thingspeak/blob/main/Core/Src/esp8266.c)

> **NOTE**: Remove all debug `printf` statements before use. The debug prints in this code are for demonstration only. They slow down execution, can cause timing issues, and may expose sensitive data like MAC addresses. Use status returns instead and let your application handle any necessary printing.

## DHT11 Sensor Driver

The DHT11 uses a **single-wire protocol** with precise timing:

| Phase | Duration | Description |
|-------|----------|-------------|
| **Start Signal** | 18ms LOW + 20µs HIGH | MCU wakes sensor |
| **Sensor Response** | 80µs LOW + 80µs HIGH | Sensor acknowledges |
| **Bit "0"** | 50µs LOW + 26-28µs HIGH | Logic 0 |
| **Bit "1"** | 50µs LOW + 70µs HIGH | Logic 1 |
| **Data Frame** | 40 bits | 5 bytes (humidity ×2 + temp ×2 + checksum) |

Instead of measuring pulse width, I used a **simpler approach** looking at datasheet:

For each bit:
1. Wait for line to go HIGH
2. Delay exactly 40µs
3. If line still HIGH → logic 1 <br>
   If line is LOW → logic 0

To ensure the timing is not interrupted, **interrupts are disabled** while communicating with the sensor. The checksum provided by the sensor is used to verify data integrity.

🔗 [View DHT11 Driver Source Code](https://github.com/rubin-khadka/STM32_ESP8266_DHT11_Thingspeak/blob/main/Core/Src/dht11.c)

## DWT Microsecond Delay Driver
DWT (Data Watchpoint and Trace) is a built-in peripheral in ARM Cortex-M3 cores that provides a cycle counter running at CPU frequency (72MHz). This gives ~13.9ns resolution, making it ideal for generating precise microsecond delays required by the DHT11 1-Wire protocol. Unlike traditional timer-based delays, DWT:
- Does not occupy a dedicated timer peripheral
- Continues running in the background
- Provides cycle-accurate timing without interrupt overhead

🔗 [View DWT Driver (Microsecond Delay)](https://github.com/rubin-khadka/STM32_ESP8266_DHT11_Thingspeak/blob/main/Core/Src/dwt.c) 

## Getting Started

### Software Prerequisites

| Software | Version | Purpose |
|----------|---------|---------|
| STM32CubeIDE | v1.13.0+ | IDE for development and flashing |
| STM32 HAL Library | Built-in | Hardware abstraction layer |
| Serial Terminal | Any (PuTTY, Arduino IDE, Hterm) | For debugging via USART3 |
| ESP8266 AT Firmware | Official or AI-Thinker | Must be flashed to NodeMCU first |

### Setting Up the ESP8266

Before connecting to STM32, ensure the NodeMCU has AT firmware:

1. **Test with PC** using USB and serial terminal:
    ```
    AT 
    OK

    AT+CWMODE=1
    OK

    AT+GMR
    AT version:...
    ```

2. If the NodeMCU has Lua firmware, you'll need to **flash AT firmware first**. The AI-Thinker firmware works well with NodeMCU boards.

### Installation

1. Clone the repository
```bash
git clone https://github.com/rubin-khadka/STM32_ESP8266_DHT11_Thingspeak.git
```
2. Open in STM32CubeIDE
    - `File` → `Import...`
    - `General` → `Existing Projects into Workspace` → `Next`
    - Select the project directory
    - Click `Finish`    

3. Update Configuration
    - Open `main.c` and update your WiFi credentials (Line ~121):
    ```c
    ESP_ConnectWiFi("your_ssid", "your_password", ip_buf, sizeof(ip_buf))
    ```
    - Update ThingSpeak API key (line ~75)
    ```c
    #define API_KEY "HFT599YW7PMFWYKG"
    ```

4. Build & Flash
    - Build: `Ctrl+B`
    - Debug: `F11`
    - Run: `F8` (Resume)

### Debug Output
Connect a USB-UART converter to PB10 (TX) and open a serial terminal (115200 baud, 8N1) to monitor debug messages.

## Related Projects 
- [STM32_ESP8266_IP_ATCommand](https://github.com/rubin-khadka/STM32_ESP8266_IP_ATCommand) - Base of this project, connects to WiFi and gets IP address
- [STM32_DHT11_UART_BareMetal](https://github.com/rubin-khadka/STM32_DHT11_UART_BareMetal) - Basic DHT11 sensor reading with UART output using bare metal programming

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [DHT11 Sensor Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
- [ESP8266 AT Command Set](https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/index.html)
- [NodeMCU Documentation](https://nodemcu.readthedocs.io/en/release/)
- [Flashing AT Firmware to NodeMCU](https://docs.ai-thinker.com/en/esp8266/)

## Project Status
- **Status**: Complete
- **Version**: v1.0
- **Last Updated**: March 2026

## Contact
**Rubin Khadka Chhetri**  
📧 rubinkhadka84@gmail.com <br>
🐙 GitHub: https://github.com/rubin-khadka