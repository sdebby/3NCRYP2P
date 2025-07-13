// Helper functions

  // Initise display SSD1306
void intDisp(){
  display.clearDisplay();
  display.setTextSize(TextSize);
  display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);
  int BottomLineY = SCREEN_HEIGHT - LINE_HEIGHT - 2; // Set line
  display.drawFastHLine(0, BottomLineY, SCREEN_WIDTH, WHITE);
  display.setCursor(0, SCREEN_HEIGHT - LINE_HEIGHT);
  display.display();
  delay(1000);
}

  // Convert String to byte arry
byte* stringToByteArray(const String& str) {
  byte* byteArray = new byte[str.length() + 1];
  str.getBytes(byteArray, str.length() + 1);
  return byteArray;
}

  // Convert byte arry to string
String ByteArryToString(byte* input){
  String rtn="";
  for (int i=0; i<sizeof(input);i++){
    rtn += char(input[i]);
  }
  return rtn;
}

  // Convert byte to byte array
byte* ByteToByteArry(byte myByte){
  byte array[sizeof(myByte)];
  for(int j = 0; j < sizeof(myByte); j++){
      array[j] = bitRead(myByte, j);
  }
  return array;
}

// Hash string using SHA224
byte* Hash_SHA224(const char* input){
  SHA224 hasher;
  hasher.update(input, strlen(input));
  byte hash[sizeof(hasher)];
  hasher.finalize(hash,sizeof(hasher));
  return hash;
}

  // Hash string using SHA224
  // return base64 of hash
String Hash_base64(const char* input){
  SHA224 hasher;
  hasher.update(input, strlen(input));
  byte hash[sizeof(hasher)];
  hasher.finalize(hash,sizeof(hasher));
  String rtn = base64::encode(ByteArryToString(hash));
  return rtn;
}

  // Generate random byte arry
byte* GetRandByte(int iteractions){
  byte RTNByte[iteractions];
  for(int i=0 ; i< iteractions; i++){
    RTNByte[i]= RandByte();
    Serial.print(RTNByte[i]);
    Serial.print(" ");
  }
  return RTNByte;
}

  //returns random byte from 0 to 256
byte RandByte(){
  // randomSeed(analogRead(0)); //when no analog input- it creats same values
  // int randNumber = random(256);
  int randNumber = esp_random() % 256;
  char hexString[3];
  sprintf(hexString, "%02X", randNumber);
  return randNumber;
}

  // Compare input string and hash string
  // return True if hash similar
boolean CompareHash (String input, String hash){
  String inputHash = Hash_base64(input.c_str());
  // Serial.println(inputHash);
  // Serial.println(hash);
  if (inputHash == hash){
    log_d("[V] Hash sum OK");
    return true;
  } else {
    log_d("[X] Hash sum Error!");
    return false;
  } 
}