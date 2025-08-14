# Arduino RFID Access Control System

A simple and effective RFID-based access control system built with Arduino.

## Features

- Reads RFID cards using an [MFRC522](https://www.electronicwings.com/nodemcu/mfrc522-rfid-reader-interfacing-with-nodemcu) reader
- Controls door lock/relay based on authorized cards
- Stores authorized RFID tags in code (or EEPROM)
- Serial output for logging access attempts
- Easy to modify and expand for your needs

## Hardware Requirements

- Arduino Nano (or compatible board)
- MFRC522 RFID Reader
- RFID Cards or Keyfobs
- Relay Module (for controlling a door lock)
- Jumper wires, breadboard, power supply

## Wiring

| MFRC522 Pin | Arduino Pin |
|-------------|-------------|
| SDA         | D10         |
| SCK         | D13         |
| MOSI        | D11         |
| MISO        | D12         |
| IRQ         | Not used    |
| GND         | GND         |
| RST         | D9          |
| 3.3V        | 3.3V        |

Relay module should be connected to a digital output pin (e.g., D8).

## Software Setup

1. **Clone this repository:**
   ```sh
   git clone https://github.com/bernoussama/arduino-rfid.git
   ```
2. **Install required libraries:**
   - [MFRC522](https://github.com/miguelbalboa/rfid)

   In Arduino IDE:  
   `Sketch` > `Include Library` > `Manage Libraries` > Search for `MFRC522` and install.

3. **Open the project in Arduino IDE and upload to your board.**

4. **Modify the list of authorized RFID tags in the code as needed.**

## Usage

- Power the system.
- Scan an RFID card/tag.
- If authorized, the relay activates (unlocking the door).
- Unauthorized cards trigger a rejection.

## Customization

- Change pins or add new functionality in the code (`.ino` file).
- Expand with LCD, buzzer, or WiFi for logging and notifications.

## Credits

- [MFRC522 by miguelbalboa](https://github.com/miguelbalboa/rfid)
- Inspired by open-source RFID access projects

## License

[MIT](LICENSE)
