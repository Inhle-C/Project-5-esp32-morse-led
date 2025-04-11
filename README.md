
# Project 5: Morse Code LED Communication with ESP32 and Raspberry Pi

**Part 5 of my IoT Lab Series using ESP32 and Raspberry Pi 4**

## Overview

This project implements communication between the Raspberry Pi 4 and the ESP32-C3 using Morse code via LED signals.  
The Raspberry Pi transmits messages by blinking an LED in Morse code, while the ESP32 receives and decodes these signals using an ADC with a photodiode sensor.

The goal is to build a working light-based communication system and explore its speed limits.

## Objectives

- ✅ Transmit Morse code messages via LED using the Raspberry Pi 4
- ✅ Decode Morse code messages on the ESP32 using ADC and photodiode
- ✅ Test and improve transmission speeds, recording the limits of reliable communication
- ✅ Report speeds at which transmission is successful and where it begins to fail
- ✅ Compete for extra credit by achieving one of the fastest transmission rates in the class

## Project Structure

esp32-lab5-morse-led/  
├── report.pdf # Lab report (required)  
├── lab5_1/ # Lab 5.1: Morse Code Sender (Pi4)  
│ └── send  
├── lab5_2/ # Lab 5.2: ESP32 Receiver (ADC)  
│ ├── sdkconfig  
│ ├── CMakeLists.txt  
│ ├── README.md  
│ └── main/  
│ ├── CMakeLists.txt  
│ ├── main.c  
│ └── receiver.h  
├── lab5_3/ # Lab 5.3: Speed Improvement  
│ ├── sdkconfig  
│ ├── CMakeLists.txt  
│ ├── README.md  
│ └── main/  
│ ├── CMakeLists.txt  
│ ├── main.c  
│ └── receiver.h  

## Setup Instructions

### 💡 Lab 5.1: Morse LED on Pi4

1. Connect the Raspberry Pi GPIO pin to an LED in series with a 330 Ohm resistor
2. Write the sender program to transmit Morse code messages via LED
3. Run the program with arguments:  
./send 4 "hello ESP32"  
This will flash the LED message 4 times.

### 📡 Lab 5.2: ESP32 Receiver via ADC

1. Connect a photodiode sensor to the ESP32 ADC pin
2. Ensure the photodiode is at least 1mm away from the LED
3. Implement a receiver program to detect Morse code flashes and print decoded messages to the terminal using idf.py monitor

### ⚡ Lab 5.3: Increase Communication Speed

1. Optimize sender and receiver to increase communication speed
2. Use digital GPIO and/or adjust ADC settings for faster detection
3. Measure and report:
- Maximum speed at which transmission is successful
- Speed at which transmission starts to fail
4. Ensure the difference between pass/fail speeds is less than or equal to 25%

## Notes

- Exclude build/ directories when zipping the project.
- Submit the required files and directories: lab5_1/send, lab5_2/*, lab5_3/*
- Document issues or learnings in report.pdf and subfolder README.md.
- All external code must follow APACHE or BSD-like licenses.
- Reference any helpful resources properly in report.pdf (No StackOverflow, Reddit).

## What I Learned

- Implementing Morse code encoding and decoding
- Using Raspberry Pi GPIO for LED control
- Reading light signals with ESP32 ADC and photodiode
- Optimizing embedded systems for high-speed communication
- Measuring system performance and failure thresholds

## Future Improvements

- Automate decoding for different message lengths
- Add checksum verification to improve message integrity
- Explore bi-directional communication
- Test performance in varying ambient light conditions
- Extend to multi-node LED communication systems

## License
This project is for educational purposes.

Previous Project: [ESP32 Bluetooth Mouse](https://github.com/Inhle-C/Project-4-esp32-lab4-bluetooth-mouse)  
(Part 4 of the series)

Next Project: [ESP32 Sensor Data Logger](https://github.com/Inhle-C/Project-6-esp32-ultrasonic-sensor) 🔗  
(To be uploaded as Part 6 of the series)
