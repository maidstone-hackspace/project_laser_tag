/*
   IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
   An IR detector/demodulator must be connected to the input RECV_PIN.
   Version 0.1 July, 2009
   Copyright 2009 Ken Shirriff
   http://arcfn.com
*/

#include <IRremote.h>

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  delay(1000);
  irrecv.enableIRIn(); // Start the receiver
}

void printData(uint16_t TEAM, uint16_t PLAYER, uint16_t DAMAGE) {
    Serial.print("Hit by Player  ");
    Serial.print(PLAYER);
   Serial.print(" from Team ");
    Serial.print(TEAM);
    Serial.print(" for ");
    Serial.print(DAMAGE);
    Serial.println(" Damage.");
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, BIN);
    uint16_t TEAM = (results.value & (((1 << 2) - 1) << 9)) >> 9;
    uint16_t PLAYER = (results.value & (((1 << 5) - 1) << 4)) >> 4;
    uint16_t DAMAGE = (results.value & (((1 << 4) - 1)));
    uint8_t CHECKSUM = (results.value & (((1 << 5) - 1) << 11)) >> 11;
    uint8_t validChecksum = (uint8_t)~(TEAM+PLAYER+DAMAGE)>>3;
    if (validChecksum == CHECKSUM) printData(TEAM, PLAYER, DAMAGE);
    irrecv.resume(); // Receive the next value
  }
}
