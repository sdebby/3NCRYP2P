/*
11.04.2025
3NCRYPT2P project
__ Omni Screen and keyboard program __

This code will be implemented in both devices (Sender and Receiver)
the communication will be thru screen(SSD1306) and keyboard (CardKB).
** Change the AIM flag to generate automated interval messages
** Chenge to core debug level (in arduino IDE menu - Tools) to communicate using console and see system messages
*/  

#include <SPI.h>
#include <LoRa.h>
#include <AES.h>
#include "base64.hpp"
#include <base64.h>
#include <CTR.h>
#include <SHA224.h>
#include <Preferences.h>
#include "esp32-hal-log.h"

// Define screen and keyboard
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define CARDKB_ADDR 0x5F 
#define SCRN_ADDR 0x3C 

// Define screen width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MAX_LINES 6  // Maximum number of lines that can fit on screen
#define LINE_HEIGHT 8  // Height of each line in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
int TextSize = 1;
String lines[MAX_LINES];  // Array to store lines
int currentLine = 0;      // Current line position

#if CONFIG_IDF_TARGET_ESP32S3  // Define LoRa-ESP32 pin out for ESP32 - S3
  #define BoadMSG  "[*] ESP32-S3 board found"
  #include <WiFi.h>
  #define ss 10
  #define rst 6
  #define dio0 5

#elif CONFIG_IDF_TARGET_ESP32C6  // Define LoRa-ESP32 pin out for ESP32 - C6
  #define BoadMSG  "[*] ESP32-C6 board found"
  #include <WiFi.h>
  #define ss 18
  #define rst 23
  #define dio0 22
  #define WireSDA 3
  #define WireSCL 2

  #else
  #error "[X] This code is for ESP32 S3 and ESP32 C6 only."
#endif

bool Consl = false; // Console mode
bool TransmissionACK = true; // Transmission acknowledge by 2nd device flag
unsigned long TransmissionMillis = 0;
const long TransmissionRcvACKInterval = 3.5 * 1000; // waiting time for Tramsnission acknowledge (milliseconds)

  // Define Encryption key
byte keyOriginal[16] = {0x5E, 0xC3, 0x7A, 0x1F, 0xB8, 0x9D, 0x42, 0x86, 0xE4, 0x3B, 0x60, 0xF5, 0x2C, 0xAB, 0x91, 0x08};
byte key[16] ;
byte iv[16] ;
CTR<AES128> ctr;

  // Debug - send messages in intervals
bool AIM = true; // AIM = Automated Interval Messages
  int counter = 0;
  unsigned long previousMillis = 0; 
  const long interval = 10 * 1000;  // intervals to send messages in ms


String RecievedMSG_Pack = "";
int RTN_OK = 200;
int RTN_NG = 502;

bool FirstMsg = true; // Check if this is the first message
int ErrCount = 0;
int ErrAllowed = 10;
Preferences preferences;
String PayloadInput = "";
String ReSendPayloadInput = "";

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("**-- LoRa OMNI SC-KB --**");
  // Get compiled debug level
  #ifdef CORE_DEBUG_LEVEL
    Serial.print("[*] CORE_DEBUG_LEVEL Value: ");
    Serial.println(CORE_DEBUG_LEVEL);  // Try printing its value
    #if CORE_DEBUG_LEVEL == 4// if compiled debug level = DEBUG
       Consl = true; 
    #endif
  #else
    Serial.println("[*] CORE_DEBUG_LEVEL is NOT defined.");
  #endif

  // I2C pins
  Wire.begin(WireSDA, WireSCL);
  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCRN_ADDR)) {
    Serial.println(F("[X] SSD1306 allocation failed"));
    for (;;);
  }
  // initize display SSD1306
  LogDisp(); // Display logo on screen
  intDisp();
  intPref();
  if (preferences.getBool("DeviceLOCK",false)){ //TODO - set red light
    LockLoop();
  }
  Serial.println(BoadMSG);
	// Initialize LoRa 
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    log_d("[X] Starting LoRa failed!");
    delay(100);
    while (1);
  }
  SetLoRaParam();// LoRa parameters
}

void loop() {
  unsigned long currentMillis = millis();
  onReceive(LoRa.parsePacket());
  delay(50);
  if (Consl && Serial.available()) { // Get text from serial if using debug 
    PayloadInput = Serial.readStringUntil('\n');
    SndCmd(PayloadInput);
  } else { // Get text from CardKB
    Wire.requestFrom(CARDKB_ADDR, 1);  
    while (Wire.available())  {
      char c = Wire.read();  // Store the received data.  
      PayloadInput += String(c) ;
      if (c != 0) {
        if (c == 13){  // Enter
          PayloadInput.remove (PayloadInput.length() - 1); // remove last charecter
          SndCmd(PayloadInput);
          String TmpPayloadInput = PayloadInput;
          ReSendPayloadInput = PayloadInput; // for resending payload if needed
          PayloadInput = "-> " + TmpPayloadInput;
          scrollLines();
          PayloadInput = "";
          redrawDisplay();
        }
        else if (c == 8) {  // Backspace
          int cursorX = display.getCursorX() - 6 * TextSize;
          int cursorY = display.getCursorY();
          if (cursorX < 0) cursorX = 0 ;
          display.setCursor(cursorX, cursorY);
          display.fillRect(cursorX,cursorY,cursorX + 6 * TextSize , cursorY + LINE_HEIGHT * TextSize,BLACK);
          display.display();
          if (PayloadInput.length() > 0) {
            PayloadInput.remove (PayloadInput.length() - 2); // remove last charecter
          }
          c = 0;
        } else {
        display.print(c);
        }
        display.display();
      }
    }
  }
  if (AIM){ // Generate message and send
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      Serial.print("[*] Sending DEBUG packet: ");
      Serial.println(counter);
      String Ttext = "DBG PKT No.:" + String(counter);
      PayloadInput = "-> " + Ttext;
      scrollLines();
      redrawDisplay();
      SndCmd(Ttext);
      counter ++;
      }
  }
  // Check if received acknowledge
  if ( currentMillis >= TransmissionMillis && currentMillis - TransmissionMillis >= TransmissionRcvACKInterval && !TransmissionACK) {
    // Message acknowledge did not receaved after TransmissionACKInterval time
    Serial.print("[X] Error sending message - no ACK");
    TransmissionACK = true;
    PayloadInput = "-X Err sending msg";
    scrollLines();
    PayloadInput = "";
    redrawDisplay();
  }

}

/* --------------------------------------- */

  // Shift all lines down by one
void scrollLines() {
  for (int i = MAX_LINES - 1; i > 0; i--) {
    lines[i] = lines[i - 1];
  }
  // Add new line at the top
  lines[0] = PayloadInput;
}

  // draw display again 
void redrawDisplay() {
  display.clearDisplay();
  for (int i = 0; i < MAX_LINES; i++) {
    display.setCursor(0, i * LINE_HEIGHT);
    display.print(lines[i]);
  }
  
  // Draw input line and separator
  display.setCursor(0, SCREEN_HEIGHT - LINE_HEIGHT);
  display.print(PayloadInput);
  int BottomLineY = SCREEN_HEIGHT - LINE_HEIGHT - 2;
  display.drawFastHLine(0, BottomLineY, SCREEN_WIDTH, WHITE);
  display.display();
}

  //initialize preferences
void intPref(){
  preferences.begin("3NCRYPT2P", false); // The second parameter is false for read/write access. Use true for read-only.
  if (AIM){
    preferences.clear();
  }
}

  // Send payload
void SndCmd(String Ttext){  // prepare and send message
  if (FirstMsg){
      NewKey();
      delay(1500);
      FirstMsg = false;
  }
  NewIV();  // generate random IV
    // Hashing text and converting to byte
  String TxtHash = Hash_base64(Ttext.c_str());
  byte* TxtSHA224 = stringToByteArray(TxtHash);
    // Encrypting
  byte* plaintext = stringToByteArray(Ttext);
  SetEnc(key,sizeof(key));
  byte ciphertext[Ttext.length()]; // Buffer for the ciphertext
  ctr.encrypt(ciphertext, plaintext, Ttext.length());
  log_d("[*] Before Encryption: %s",Ttext);
    // send packet
  SendMSG(1, Ttext.length() , ciphertext , sizeof(iv), iv, TxtHash.length(), TxtSHA224);
}

// Generate a new key , encrypt with predefine key and random IV and send
void NewKey(){
  log_d("[*] Creating new key --");
  for (int i =0 ; i<sizeof(keyOriginal) ; i++){
    key[i] = RandByte();
  }
  NewIV();
  SetEnc(keyOriginal,sizeof(keyOriginal));
  byte ciphertext[sizeof(key)]; // Buffer for the ciphertext
  ctr.encrypt(ciphertext, key, sizeof(key));

    // Send message with Setup 0
  SendMSG(0, sizeof(ciphertext),ciphertext, sizeof(iv), iv, 0, 0);
}

  // Set encryption parameters
void SetEnc(byte* inputkey,int KeySize){
  if (!ctr.setKey(inputkey, KeySize))
    log_d("[X] setKey failes!");
  if (!ctr.setIV(iv, sizeof(iv))) 
    log_d("[X] setIV failes!");
}

  // Setting LoRa parameters
void SetLoRaParam(){
  LoRa.setSyncWord(0xB2);  //Unique Sync Word
  LoRa.setSignalBandwidth(41.7E3); //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3.
  LoRa.setSpreadingFactor(9) ;  // Supported values are between `6` and `12`. If a spreading factor of `6` is set, implicit header mode must be used to transmit and receive packets.
  LoRa.setTxPower(17); //2-17 
  LoRa.enableCrc();  
  log_d("[V] LoRa init succeeded.");
}

  // generate new IV
void NewIV(){
  for (int i =0 ; i<sizeof(iv) ; i++){
    iv[i] = RandByte();
  }
}

  // onReceive  function
void onReceive(int packetSize){
  if (!packetSize) return; //Drop packet
      // Print packet data
      FirstMsg = false;
      Serial.print("[*] Packet size: "); //print to serial
      Serial.print(packetSize);
      String MyText = "  SNR: ";
      MyText += LoRa.packetSnr();
      MyText += "  Signal: ";
      float RSSI_pre = (1.0 - (-1*LoRa.packetRssi())/125.0) * 100.0 ;
      MyText += RSSI_pre;
      MyText += " %";

      Serial.println(MyText);
      RecievedMSG_Pack += "[*] Packet received - "; //print to portal
      RecievedMSG_Pack += MyText;
         /* read packet */

    // read setup param
  int SetupParam = LoRa.read();
  switch (SetupParam) {
    case 0:{//Update new key
        // Read encrypted key
      int KeySize = LoRa.read();
      byte EncKey[KeySize];
      for (int i = 0; i < KeySize; i++) {
        EncKey[i] = LoRa.read();
        }

          // Read iv
      int IV64Size = LoRa.read();
      for (int i = 0; i < IV64Size; i++) {
        iv[i] = LoRa.read();
      }
          log_d("New key received,Key size: %d  IV size: %d",KeySize,IV64Size);
          RecievedMSG_Pack += "[*] New key received";
      SetEnc(keyOriginal,16);
      ctr.decrypt(key, EncKey, KeySize);
      for(int i=0;i<sizeof(key);i++){
        Serial.print(String(key[i], HEX));
        Serial.print(" ");
      }
      Serial.println();    
    break; }

    case 1:{ // continue read packet
          // Read payload
      byte payloadSize = LoRa.read();
      byte payload[payloadSize];
      for (int i = 0; i < payloadSize; i++) {
        payload[i] = LoRa.read();
      }

        // Read iv
      byte IV64Size = LoRa.read();
      for (int i = 0; i < IV64Size; i++) {
        iv[i] = LoRa.read();
      }

        //Read text hash
      byte TxtHashSize = LoRa.read();
      String TxtHashStr;
        for (int i = 0; i < TxtHashSize; i++) {
          TxtHashStr +=(char)LoRa.read();
      }
        // Decrypt
      SetEnc(key,16);
      byte decryptedtext[payloadSize]; // Buffer for the decrypted text
      ctr.decrypt(decryptedtext, payload, payloadSize);
      Serial.print("[*] Received packet: ");
      String Ttext;
      for (int i=0 ; i<sizeof(decryptedtext) ; i++){
        Ttext += char(decryptedtext[i]);
      }
      Serial.println(Ttext);
        // Check hash
      if (CompareHash(Ttext, TxtHashStr)){
        RecievedMSG_Pack += "[V] " + Ttext ;
        String receivedMessage = "<- " + Ttext;  // Add prefix for received message
        scrollLines();
        lines[0] = receivedMessage;  // Store the received message
        redrawDisplay();
        SendMSG(2, RTN_OK ,0 ,0 , 0 , 0 , 0);
      } else {
        RecievedMSG_Pack += "[X] Error message hash";
        SendMSG(2, RTN_NG ,0 ,0 , 0 , 0 , 0);
      } 
  break;}

  case 2:{  //payload reply
    int PKT;
    PKT = LoRa.read();
    if (PKT == RTN_OK){
      ErrCount = 0;
      log_d("[V] Packet recieved OK");
      TransmissionACK = true; 
      // LockLoop(); //Debug
    } else { // Packet acknowledge receive error
        ErrCount += 1;
        if (ErrCount == ErrAllowed){ // large errors can indicate keybrote force - system will lock
          LockLoop();
          log_d("[X] Device Locked - Resync master key");
        } else {
          log_d("[X] Wrong Hash on receiver, lockup in %d  attempts.",ErrAllowed - ErrCount);
          NewKey();
          log_d("[*] Sending payload again");
          delay(500);
          SndCmd(ReSendPayloadInput);
        }
      }
  break;}

  default:{
      // do something else
  break;}
  }
}

  // This is a lock loop, it will lock unit for to many errors
void LockLoop(){
  preferences.putBool("DeviceLOCK",true);
  preferences.end();
  Serial.println ("To many errors - unit will lock !\nResync keys to enable unit transmission");
  while (true){
    Serial.print(" .");
    delay(1000);
  }
}

  /* send message via LoRa
  Input:
  Setup (0) - SEND NEW KEY
  Key size (int)
  Encrypted key (byte array)
  IV size (int)
  IV (byte array)
  ---
  Setup (1) - SEND PAYLOAD
  payload size (int)
  payload (byte array)
  IV size (int)
  IV (byte array)
  text hash size (int)
  text hash (byte array)
  ---
  Setup (2) - PAYLOAD REPLY
  Payload reply (int)
  */
  // send payload via LoRa
void SendMSG(int Setup,int payloadSize, byte* payload, int IVSize, byte* IV ,int TxtHashSize, byte* TxtHash){
  LoRa.beginPacket();
    LoRa.write(Setup);
    LoRa.write(payloadSize);
  if (Setup != 2){ // End of sending payload reply
    LoRa.write(payload,payloadSize);
    LoRa.write(IVSize);
    LoRa.write(IV,IVSize);
    LoRa.write(TxtHashSize);
    LoRa.write(TxtHash,TxtHashSize);

    TransmissionACK = false; 
    TransmissionMillis = millis();
  }
  LoRa.endPacket();
}