# LoRaOmniScreenKB

Project start date: Jan. 2024

![Overview picture](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9120.jpg?raw=true)

## Overview

3NCRYPT2P (Encrypt P2P) project, designed to enable secure, wireless communication between two devices or more, on long ranges, using LoRa (Long Range communication) technology (on 433 Mhz transmission), without cellular networks. The system features an OLED screen (SSD1306) and a CardKB keyboard for user interaction. Both sender and receiver run the same firmware, supporting encrypted messaging, key exchange, and visual feedback.

## MRD (Marketing Requirement)

- Provide a simple, secure, and portable LoRa-based communication terminal.
- Support up to 6 lines of message history on a compact OLED display.
- Enable secure key exchange and encrypted messaging between devices.
- Allow easy user input via a compact keyboard.
- Feedback for system status and errors using debug.
- New transmission protocol for secure communication between devices.

## Skill Set

- Embedded C++ programming (Arduino/ESP32).
- LoRa wireless communication.
- Cryptography (AES, SHA224, CTR mode).
- 3D design.
- 3D printing.

## Setup (Software Setup)

1. **Hardware Requirements:**
   - ESP32-S3 or ESP32-C6 development board
   - LoRa module (433MHz)
   - SSD1306 OLED display (I2C)
   - CardKB keyboard (I2C)
   - Required wiring and power supply

2. **Software Requirements:**
   - Arduino IDE (latest version recommended)
   - ESP32 board support installed via Board Manager
   - Required libraries:
     - Adafruit_SSD1306
     - LoRa
     - AES, CTR, SHA224 (Crypto libraries)
     - Preferences (for ESP32 NVS)
     - base64

3. **Flashing the Firmware:**
   - Clone or download this repository.
   - Open `LoRaOmniScreenKB.ino` in Arduino IDE.
   - Select the correct board and port.
   - Install all required libraries via Library Manager.
   - Compile and upload the firmware to your ESP32 board.

## BOM (Bill Of Materials):

**For R&D:**

1. ESP32 C6 dev kit
2. [LoRa RA02](https://docs.ai-thinker.com/_media/lora/docs/c048ps01a1_ra-02_product_specification_v1.1.pdf) (433 MHz module)
3. [SSD1306 OLED](https://www.adafruit.com/product/938) (64x64 pixels, I2C)
4. [CardKB keyboard](https://docs.m5stack.com/en/unit/cardkb_1.1) (I2C)
5. Breadboard and jumper wires
   
**For production:**
1. ESP32 C6 dev kit
2. [LoRa RA02](https://docs.ai-thinker.com/_media/lora/docs/c048ps01a1_ra-02_product_specification_v1.1.pdf) (433 MHz module)
3. [SSD1306 OLED](https://www.adafruit.com/product/938) (64x64 pixels, I2C)
4. [CardKB keyboard](https://docs.m5stack.com/en/unit/cardkb_1.1) (I2C)
5. [Charger panel](https://www.aliexpress.com/item/1005005037876729.html?spm=a2g0o.order_list.order_list_main.386.6b881802CcDzO8)
6. 433 MHz antenna.
7. LiPo battery 850 mAh.
8. On/Off switch.
9. Custom PCB manufacturing.
10. 3D printing for enclosure.

## Software:

1. Arduino IDE for coding and uploading to ESP32 C6 board.
2. KiCad for PCB design.
3. Fusion360 for 3D enclosure design

## TSD (Technical Specification)

- Microcontroller: ESP32-S3 or ESP32-C6
- Display: SSD1306 OLED, 64x64 pixel
- Keyboard: CardKB
- LoRa: 433MHz, configurable sync word, bandwidth, spreading factor, and power
- Encryption: AES-128 in CTR mode, SHA224 for message hashing, base64 for hash encoding
- Key Exchange: Random key and IV generation, secure transmission
- Error Handling: Device lockout after repeated errors, visual and serial feedback
- Message Handling: Up to 6 lines of message history, scrolling, and input editing

## Scheme

1. **Startup:**
- Display logo and initialize peripherals.
- Check for device lock status.

1. **Key Exchange:**
- On first message, generate and exchange encryption keys and IVs.
- encrypt key using predefined embedded key.
- from now on all messages will be encrypted with the exchanged key, and random IV.

1. **Message Transmission:**
- User inputs message via CardKB or serial (debug mode).
- Message is hashed, encrypted, and transmitted over LoRa.
- Await acknowledgment from receiver.

1. **Message Reception:**
- Decrypt incoming messages using the exchanged key and received IV.
- Verify hash for integrity.
- Display message on OLED and send acknowledgment.

1. **Error Handling:**
- If hash check fails or no acknowledgment is received, display message on OLED, handle retries, and lock device after 10 repeated errors.

### Transmission Protocol
| Field | Description |
|-------|-------------|
| Setup bit | indicator for message type |
| Payload | Encrypted message content |

**Case Setup Bit = 0:**
Packet format:
A | B | C | D | E 

A - Setup bit (Byte) (0).

B - Key length (byte).

C - Key (byte array).

D - IV length(byte).

E - IV (byte array)

**Case Setup Bit = 1:**
Packet format:

A | B | C | D | E | F | G

A - Setup bit (byte) (1).

B - IV length (byte).

C - IV (byte array).

D - payload length (byte).

E - payload (byte array).

F - Hash length (byte).

G - Hash - Base64 (SHA224(payload)) (byte array)

**Case Setup Bit = 2:**
Packet format:

A | B

A - Setup bit (byte) (2).

B - payload reply (int).

### Reply payload
* 200 if payload hash OK

* 502 if payload hash not OK

### Transmission scheme example

Case setup bit = 0

  generate random key
  generate random iv
  Send message by this order:
    Setup
      - 0 for key replace.
    key size
    key
    iv size
    iv


Case setup bit = 1
  User payload input
  Encrypt payload using key and IV
  Create SHA224 hash of payload
  Convert hash to Base64
  Send message by this order:
    Setup
      - 1 for normal text payload
    payload size 
    payload 
    iv size 
    iv 
    text hash size 
    text hash

  Getting random IV

Listen to receiver hash result.
print result status (OK/wrong hash)

if result is wrong hash -> procedure to setup bit 0 case, count number of wrong hashes, if more than 10 times, lock device.

## Receiving payload
Receive message by this order:
  Setup
    - 0 for encrypted new key
    - 1 for normal text payload
    - 2 payload reply
  payload size 
  payload 
  iv size 
  iv 
  text hash size 
  text hash


Decrypt message using key and IV from message
Create SHA224 hash of message
Compare received hash and created hash.

reply on compare results.

Case setup bit = 2
  Sending payload reply (OK/ERROR)

## Security features

1. **Encryption:** All messages are encrypted using AES-128 in CTR mode.
2. **Key Exchange:** Secure key exchange is performed during the initial connection.
3. **Integrity Check:** Message integrity is verified using SHA224 hashes.
4. **Brute Force Protection:** After 10 consecutive errors, the device locks out to prevent further communication until reset.

## Schematics

![Scheme](https://github.com/sdebby/3NCRYP2P/blob/main/media/3ncryp2p%20omni(cardkb%20and%20sdd1306)_bb.jpg?raw=true)

## PCB

* PCB design using KiCad
* 2 layers PCB
* PCB manufacturing using PCBWay
  
![PCB](https://github.com/sdebby/3NCRYP2P/blob/main/media/PCB.jpg?raw=true)

## Hardware assembly:

![PCB Assembly on 3D print top enclosure](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9112.jpg?raw=true)

![Adding RA02 LoRa module](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9113.jpg?raw=true)

![Adding ESP32C6](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9114.jpg?raw=true)

![Adding USB charging, Antenna and battery](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9110.jpg?raw=true)

### 3D enclosure design

* Using Fusion360
* 3D print using Ender 5 plus with PLA filament.
![Fusion360 Design](https://github.com/sdebby/3NCRYP2P/blob/main/media/3ncryp2p%20fusion%20360.png?raw=true)

## Range test
Sender

![Sender](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9122.jpg?raw=true)

Receiver

![Receiver](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9127.jpg?raw=true)

**Line of sight distance** 1.5 km

![Receiver](https://github.com/sdebby/3NCRYP2P/blob/main/media/IMG_9129.jpg?raw=true)


**Non-line of sight distance** 400 - 500 m

## Video

[![](https://markdown-videos-api.jorgenkh.no/youtube/woUXP3gD4LY)](https://youtu.be/woUXP3gD4LY)

## TODO
- [ ] Implement a short range, predefined key exchange mechanism, using Thread protocol or ESP-NOW.
- [ ] Implement a more robust error handling mechanism.
- [ ] Add support for multiple devices in a network.
- [ ] Replace screen with a larger one for better user experience.
- [ ] Implement a more user-friendly interface for message input and display - replace screen with 128x64 OLED display or E-INK display.
- [ ] Implement a secure bootloader to prevent unauthorized firmware updates. 
- [ ] Implement user message history storage and retrieval.

## Achilles heel
The Achilles' heel of the project is the hardcoded key embedded in the code. This key is used to encrypt the initial key exchange, which is crucial for secure communication. If an attacker gains access to the source code, they can extract this key and compromise the security of the entire system. 
To mitigate this risk, it is essential to implement a secure key exchange mechanism that does not rely on a hardcoded key. This could involve using public key cryptography or a secure key exchange protocol like Diffie-Hellman.

A predefined key exchange mechanism, such as using Thread protocol or ESP-NOW, could be implemented to securely exchange keys between devices without relying on a hardcoded key. This would enhance the security of the system and protect against potential attacks.

Another approach can be sending only half of the key over the air and displaying the other half on the screen, allowing the user to manually input it into the other device. This would require user interaction but would eliminate the need for a hardcoded key in the code.

Then the new, predefined key will be stored in the device's non-volatile storage (NVS) for future use, ensuring that the key is not hardcoded in the firmware.

## Feedback

If you have any feedback, please reach out to us at mailto:shmulik.debby@gmail.com