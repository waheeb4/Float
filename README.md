# 🌊 Float Vertical Profiling System - MATE ROV

This repository contains the complete codebase for the Float system used in the MATE ROV competition by the Triton ROV subteam.
It automates vertical profiling by collecting pressure data at different depths using a custom-made float, and visualizes the data on the station side.

---

# 🧠 System Summary

The float system consists of:
- 📡 **ESP32**: Transmits pressure readings and timestamps via Wi-Fi
- ⚙️ **Arduino Nano**: Controls a depth mechanism using motor + encoder
- 💻 **Station Python Script**: Connects to the ESP32, logs and visualizes data

---

# 📂 Project Files

| File | Description |
|------|-------------|
| `esp_float.ino` | ESP32 code: serves pressure data over Wi-Fi |
| `nano_float.ino` | Arduino Nano code: handles motor control via encoder |
| `FloatStation.py` | Python script for the station: connects to ESP32, fetches and plots data |

---

# ⚙️ Hardware

## 🔧 Hardware Involved
- ESP32 (Hotspot)
- Arduino Nano
- Pressure Sensor
- DC Motor + Encoder
- Power Supply


## 🔋 Power Supply Configuration
- The float is powered by a battery pack consisting of 16 × 1.5 V cells. These are arranged as 8 in parallel connected to another 8 in parallel (8P8P), effectively supplying 12 V. 
This 12 V line directly powers the encoder and is also routed through a buck converter to step down the voltage to suitable levels required for the ESP32 and Arduino Nano boards.


# ⚙️ Workflow
- Power on the Float — the ESP32 automatically boots and starts a Wi-Fi hotspot named ESP32_Hotspot.
- The station-side script is executed, which scans for and connects to this hotspot, then immediately sends an HTTP request to the ESP32.
- The ESP32 responds with an initial surface pressure reading and sends an acknowledgment to the station acknowledging the descent and command the nano via serial to begin the descent.
- The Arduino Nano then drives a DC motor connected to a custom syringe system to descend. It uses an encoder to track motor rotations and regulate the movement.
- Every 5 seconds during descent, the ESP32 logs a pressure reading (up to 30 readings in total).
- Once 30 readings are stored, the ESP32 signals the Nano to begin ascent. The Nano empties the syringe by reversing the motor direction until the encoder rotation count reaches zero, 
ensuring full water expulsion.
- After ascent, the station script is executed again to reconnect to the ESP32 hotspot and request the stored data.
- The station script then processes the pressure data, converts it to calibrated depth, and plots a graph alongside a tabulated summary of the readings. The output is saved as both an 
image and a text file locally.
- The station script then turns off Wi-Fi to complete the cycle.

# 🌐 esp_float.ino 

- Runs a Wi-Fi Access Point (`ESP32_Hotspot`)
- Hosts a simple HTTP server:
  - `/data`: Sends pressure/timestamp data
  - `/ack`: Receives acknowledgment from station 

- Data format:  
  ```
  102.51/14:22:10
  101.94/14:22:14, 102.05/14:22:18, ...
  ```

---

# 🖥️ FloatStation.py

- Scans and connects to `ESP32_Hotspot`
- Requests `/data` from ESP32
- Plots a real-time graph (Depth vs Time)
- Displays a table of the last 30 values
- Saves plot and data in `Readings/` directory
- Turns off Wi-Fi when done

# 🧪 Dependencies

requests matplotlib pandas tabulate
