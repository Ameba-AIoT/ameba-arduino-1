/*************************************************** 
 ****************************************************/

#include "peng_fei_Fingerprint.h"
//#include <util/delay.h>
#if (ARDUINO >= 100)
  #include <SoftwareSerial.h>
#else
  #include <NewSoftSerial.h>
#endif

//static SoftwareSerial mySerial = SoftwareSerial(2, 3);

#if ARDUINO >= 100
peng_fei_Fingerprint::peng_fei_Fingerprint(SoftwareSerial *ss) {
#else
peng_fei_Fingerprint::peng_fei_Fingerprint(NewSoftSerial *ss) {
#endif
  thePassword = 0;
  theAddress = 0xFFFFFFFF;

  mySerial = ss;
}

void peng_fei_Fingerprint::begin(uint16_t baudrate) {
  mySerial->begin(baudrate);
}

boolean peng_fei_Fingerprint::verifyPassword(void) {
  uint8_t packet[] = {FINGERPRINT_VERIFYPASSWORD, 
                      (thePassword >> 24), (thePassword >> 16),
                      (thePassword >> 8), thePassword};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, 7, packet);
  uint8_t len = getReply(packet,5000);
  
  if ((len == 1) && (packet[0] = FINGERPRINT_ACKPACKET) && (packet[1] == FINGERPRINT_OK))
    return true;

/*
  Serial.print("\nGot packet type "); Serial.print(packet[0]);
  for (uint8_t i=1; i<len+1;i++) {
    Serial.print(" 0x");
    Serial.print(packet[i], HEX);
  }
  */
  return false;
}

uint8_t peng_fei_Fingerprint::getImage(void) {
  uint8_t packet[] = {FINGERPRINT_GETIMAGE};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, 3, packet);
  uint8_t len = getReply(packet);
  
  if ((len != 1) && (packet[0] != FINGERPRINT_ACKPACKET))
   return -1;
  return packet[1];
}

uint8_t peng_fei_Fingerprint::image2Tz(uint8_t slot) {
  uint8_t packet[] = {FINGERPRINT_IMAGE2TZ, slot};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet)+2, packet);
  uint8_t len = getReply(packet);
  
  if ((len != 1) && (packet[0] != FINGERPRINT_ACKPACKET))
   return -1;
  return packet[1];
}


uint8_t peng_fei_Fingerprint::createModel(void) {
  uint8_t packet[] = {FINGERPRINT_REGMODEL};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet)+2, packet);
  uint8_t len = getReply(packet);
  
  if ((len != 1) && (packet[0] != FINGERPRINT_ACKPACKET))
   return -1;
  return packet[1];
}


uint8_t peng_fei_Fingerprint::storeModel(uint16_t id) {
  uint8_t packet[] = {FINGERPRINT_STORE, 0x01, id >> 8, id & 0xFF};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet)+2, packet);
  uint8_t len = getReply(packet);
  
  if ((len != 1) && (packet[0] != FINGERPRINT_ACKPACKET))
   return -1;
  return packet[1];
}

uint8_t peng_fei_Fingerprint::fingerFastSearch(void) {
  fingerID = 0xFFFF;
  confidence = 0xFFFF;
  // high speed search of slot #1 starting at page 0x0000 and page #0x00A3 
  uint8_t packet[] = {FINGERPRINT_HISPEEDSEARCH, 0x01, 0x00, 0x00, 0x00, 0xA3};
  writePacket(theAddress, FINGERPRINT_COMMANDPACKET, sizeof(packet)+2, packet);
  uint8_t len = getReply(packet);
  
  if ((len != 1) && (packet[0] != FINGERPRINT_ACKPACKET))
   return -1;

  fingerID = packet[2];
  fingerID <<= 8;
  fingerID |= packet[3];
  
  confidence = packet[4];
  confidence <<= 8;
  confidence |= packet[5];
  
  return packet[1];
}



void peng_fei_Fingerprint::writePacket(uint32_t addr, uint8_t packettype, 
				       uint16_t len, uint8_t *packet) {
#ifdef FINGERPRINT_DEBUG
  Serial.print("---> 0x");
  Serial.print((uint8_t)(FINGERPRINT_STARTCODE >> 8), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)FINGERPRINT_STARTCODE, HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(addr >> 24), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(addr >> 16), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(addr >> 8), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(addr), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)packettype, HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(len >> 8), HEX);
  Serial.print(" 0x");
  Serial.print((uint8_t)(len), HEX);
#endif
 
#if ARDUINO >= 100
  mySerial->write((uint8_t)(FINGERPRINT_STARTCODE >> 8));
  mySerial->write((uint8_t)FINGERPRINT_STARTCODE);
  mySerial->write((uint8_t)(addr >> 24));
  mySerial->write((uint8_t)(addr >> 16));
  mySerial->write((uint8_t)(addr >> 8));
  mySerial->write((uint8_t)(addr));
  mySerial->write((uint8_t)packettype);
  mySerial->write((uint8_t)(len >> 8));
  mySerial->write((uint8_t)(len));
#else
  mySerial->print((uint8_t)(FINGERPRINT_STARTCODE >> 8), BYTE);
  mySerial->print((uint8_t)FINGERPRINT_STARTCODE, BYTE);
  mySerial->print((uint8_t)(addr >> 24), BYTE);
  mySerial->print((uint8_t)(addr >> 16), BYTE);
  mySerial->print((uint8_t)(addr >> 8), BYTE);
  mySerial->print((uint8_t)(addr), BYTE);
  mySerial->print((uint8_t)packettype, BYTE);
  mySerial->print((uint8_t)(len >> 8), BYTE);
  mySerial->print((uint8_t)(len), BYTE);
#endif
  
  uint16_t sum = (len>>8) + (len&0xFF) + packettype;
  for (uint8_t i=0; i< len-2; i++) {
#if ARDUINO >= 100
    mySerial->write((uint8_t)(packet[i]));
#else
    mySerial->print((uint8_t)(packet[i]), BYTE);
#endif
#ifdef FINGERPRINT_DEBUG
    Serial.print(" 0x"); Serial.print(packet[i], HEX);
#endif
    sum += packet[i];
  }
#ifdef FINGERPRINT_DEBUG
  //Serial.print("Checksum = 0x"); Serial.println(sum);
  Serial.print(" 0x"); Serial.print((uint8_t)(sum>>8), HEX);
  Serial.print(" 0x"); Serial.println((uint8_t)(sum), HEX);
#endif
#if ARDUINO >= 100
  mySerial->write((uint8_t)(sum>>8));
  mySerial->write((uint8_t)sum);
#else
  mySerial->print((uint8_t)(sum>>8), BYTE);
  mySerial->print((uint8_t)sum, BYTE);
#endif
}


uint8_t peng_fei_Fingerprint::getReply(uint8_t packet[], uint16_t timeout) {
  uint8_t reply[20], idx;
  uint16_t timer=0;
  
  idx = 0;
#ifdef FINGERPRINT_DEBUG
  Serial.print("<--- ");
#endif
while (true) {
    while (!mySerial->available()) {
      delay(1);
      timer++;
      if (timer >= timeout) return FINGERPRINT_TIMEOUT;
    }
    // something to read!
    reply[idx] = mySerial->read();
#ifdef FINGERPRINT_DEBUG
    Serial.print(" 0x"); Serial.print(reply[idx], HEX);
#endif
    if ((idx == 0) && (reply[0] != (FINGERPRINT_STARTCODE >> 8)))
      continue;
    idx++;
    
    // check packet!
    if (idx >= 9) {
      if ((reply[0] != (FINGERPRINT_STARTCODE >> 8)) ||
          (reply[1] != (FINGERPRINT_STARTCODE & 0xFF)))
          return FINGERPRINT_BADPACKET;
      uint8_t packettype = reply[6];
      //Serial.print("Packet type"); Serial.println(packettype);
      uint16_t len = reply[7];
      len <<= 8;
      len |= reply[8];
      len -= 2;
      //Serial.print("Packet len"); Serial.println(len);
      if (idx <= (len+10)) continue;
      packet[0] = packettype;      
      for (uint8_t i=0; i<len; i++) {
        packet[1+i] = reply[9+i];
      }
#ifdef FINGERPRINT_DEBUG
      Serial.println();
#endif
      return len;
    }
  }
}

