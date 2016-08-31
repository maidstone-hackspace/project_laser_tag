#include <FastCRC.h>

FastCRC16 CRC16;

uint8_t damageTable[]= {1,2,4,5,7,10,15,17,20,25,30,35,40,50,75,100};
char* teamTable[]= {"RED", "BLUE", "YELLOW", "GREEN"};

#include <IRremote.h>

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(115200); 
  delay(1000);
  irrecv.enableIRIn(); // Start the receiver
}

void printData(uint16_t TEAM, uint16_t PLAYER, uint16_t DAMAGE) {
    Serial.print("Hit by Player ");
    Serial.print(PLAYER);
   Serial.print(" from the ");
    Serial.print(teamTable[TEAM]);
    Serial.print(" team for ");
    Serial.print(damageTable[DAMAGE]);
    Serial.println(" Damage.");

}

void loop() {
  if (irrecv.decode(&results)) {
     // results.value & (((1 << NUMBER_OF_BITS) - 1) << POSITION_OF_LEFTSHIFTED_FIRST_BIT)) >> SHIFT_TO_RIGHT_EDGE
    uint16_t TEAM = (results.value & (((1 << 2) - 1) << 9)) >> 9;
    uint16_t PLAYER = (results.value & (((1 << 5) - 1) << 4)) >> 4;
    uint16_t DAMAGE = (results.value & (((1 << 4) - 1)));
    uint16_t CRC = results.value >> 16;
    uint16_t DATA = (TEAM<<9) | (PLAYER<<4) | DAMAGE;
    uint8_t MSB = DATA  >> 8;
    uint8_t LSB = (DATA << 8) >> 8;
    uint8_t crcBuffer[2] = {MSB, LSB};
    uint16_t validCRC = CRC16.ccitt(crcBuffer, sizeof(crcBuffer));
    if (validCRC == CRC) printData(TEAM, PLAYER, DAMAGE);
    irrecv.resume(); // Receive the next value
  }
}

