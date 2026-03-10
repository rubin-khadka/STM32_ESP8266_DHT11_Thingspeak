# STM32 ESP8266 DHT11 ThingSpeak Project

## Project Overview
This project demonstrates an IoT-based temperature and humidity monitoring system using an STM32 Blue Pill (STM32F103C8T6) microcontroller. The STM32 reads environmental data from a DHT11 sensor and communicates with a NodeMCU ESP8266 module via AT commands. The ESP8266 connects to WiFi and sends the sensor data to ThingSpeak's cloud platform, where users can visualize real-time graphs and indicators. 

While NodeMCU is powerful enough to build complete IoT projects on its own, the objective here is different: **use NodeMCU strictly as a WiFi modem** controlled entirely by an STM32. The NodeMCU runs AT firmware and acts as a slave device - the STM32 sends commands, and the ESP8266 executes them. This approach lets you leverage the STM32's processing power while adding WiFi connectivity through a simple UART interface.

I had this module available, so I used it for demonstration - but the code works with any ESP8266 module running AT firmware.

## Video Demonstrations

https://github.com/user-attachments/assets/a4c1c71f-f1ff-4853-99e9-7ecaf0b4da2e

*Hardware Connection*

https://github.com/user-attachments/assets/199908f6-b6e9-4f4f-9b5d-6dd82351187c

- **Left:** Data received by ThingSpeak showing graph and indicator
- **Right:** UART Output showing device connecting to wifi and getting ip address and sending data to thingspeak information

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

| STM32 Pin | Function | DHT11 Pin | Purpose |
|-----------|----------|-------------------|---------|
| **PB13** | GPIO | DATA | Data pin (module already contain pullup) |
| **3.3V** | Power | **VCC** | Power supply |
| **GND** | Ground | GND | Common ground |

### Debug Output (USART3)

| STM32 Pin | Function | USB-UART Converter | Purpose |
|-----------|----------|-------------------|---------|
| **PB10** | USART3 TX | RX | Send debug messages to PC |
| **PB11** | USART3 RX | TX | (Optional) Receive from PC |
| **GND** | Ground | GND | Common ground |