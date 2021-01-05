/*
   This sketch demonstrates add contact on an andoird phone by
   tapping nfc antenna on Ameba.

   Pre-requirement:
   (1) By default the NFC antenna is not connected. You have to weld it.
   (2) The Android phone needs have NFC function.

   Android framework support NDEF (NFC Data Exchange Format) data message.
   As we configure NDEF message on Ameba's NFC firmware, tap phone on it,
   then Android framework will parse the NDEF messages and performs
   correspond actions.

   This sketch demonstrates how to configure NDEF message that make Android
   phone add contact.

*/

#include <NfcTag.h>

#define MAX_VCARD_LEN 110
char vcard_buf[MAX_VCARD_LEN];
char vcard_str[MAX_VCARD_LEN] = "BEGIN:VCARD\nVERSION:2.1\n";

char name[] = "Forrest Gump";
char family_name[] = "Forrest";
char given_name[] = "Gump";
char additional_name[] = "";
char honorific_prefix[] = "Mr.";
char honorific_suffixes[] = "";
//char work_phone[] = "(444) 555-1212";
char home_phone[] = "(111) 555-1212";
//char email[] = "email@email.com";
//char org[] = "";
//char title[] = "";

void setup() {
  int vcard_len = 0;

  // For vcard version 1.0
  //  vcard_len = sprintf( vcard_buf, "BEGIN:VCARD\r\nVERSION:1.0\r\nN:%s\r\nFN:%s;%s;%s;%s;%s\r\nTEL;VOICE:%s\r\nEND:VCARD",
  //                       name,
  //                       family_name, given_name, additional_name, honorific_prefix, honorific_suffixes,
  //                       home_phone);

  // For vcard version 2.1
  sprintf(vcard_str, "%sN:%s;%s;%s;%s;%s;\n", vcard_str, given_name, family_name, additional_name, honorific_prefix, honorific_suffixes);
  sprintf(vcard_str, "%sFN:%s\n", vcard_str, name);
  //sprintf(vcard_str, "%sORG:%s\n", vcard_str, org);
  //sprintf(vcard_str, "%sTITLE:%s\n", vcard_str, title);
  //sprintf(vcard_str, "%sTEL;WORK;VOICE:%s\n", vcard_str, work_phone);
  sprintf(vcard_str, "%sTEL;HOME;VOICE:%s\n", vcard_str, home_phone);
  //sprintf(vcard_str, "%sEMAIL;PREF;INTERNET:%s\n", vcard_str, email);
  sprintf(vcard_str, "%sEND:VCARD", vcard_str);
  vcard_len = sprintf(vcard_buf, vcard_str);


  if (vcard_len <= MAX_VCARD_LEN) {
    NfcTag.appendVcard(vcard_buf, vcard_len);
    NfcTag.begin();
  } else {
    Serial.println("ERR: Invalid length of vCard!");
  }
}

void loop() {
  delay(1000);
}
