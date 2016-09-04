#include <FastCRC.h>
#include <IRremote.h>

FastCRC16 CRC16;

uint8_t damageTable[] = {1, 2, 4, 5, 7, 10, 15, 17, 20, 25, 30, 35, 40, 50, 75, 100};
char const* teamTable[] = {"RED", "BLUE", "GREEN", "YELLOW", "CYAN", "MAGENTA", "WHITE", "SOLO"};

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);

decode_results results;
unsigned long ledTime;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
}

void printData(uint16_t TEAM, uint16_t PLAYER, uint16_t DAMAGE) {
  ledTime = millis();
  digitalWrite(13, HIGH);
  digitalWrite(12, HIGH);
  Serial.print("Hit by Player ");
  Serial.print(PLAYER+1);
  Serial.print(" from the ");
  Serial.print(teamTable[TEAM]);
  Serial.print(" team for ");
  Serial.print(damageTable[DAMAGE]);
  Serial.println(" Damage.");
}

void loop() {
  if (irrecv.decode(&results)) {
    // results.value & (((1 << NUMBER_OF_BITS) - 1) << POSITION_OF_LEFTSHIFTED_FIRST_BIT)) >> SHIFT_TO_RIGHT_EDGE
    uint16_t TEAM = (results.value & (((1 << 3) - 1) << 13)) >> 13;
    uint16_t PLAYER = (results.value & (((1 << 6) - 1) << 7)) >> 7;
    uint16_t DAMAGE = (results.value & (((1 << 7) - 1)));
    uint16_t CRC = results.value >> 16;
    uint16_t DATA = (TEAM << 13) | (PLAYER << 7) | DAMAGE;
    uint8_t MSB = DATA  >> 8;
    uint8_t LSB = (DATA << 8) >> 8;
    uint8_t crcBuffer[2] = {MSB, LSB};
    uint16_t validCRC = CRC16.ccitt(crcBuffer, sizeof(crcBuffer));
    if (validCRC == CRC) printData(TEAM, PLAYER, DAMAGE);

    //printData(TEAM, PLAYER, DAMAGE); // For debugging error rates
    irrecv.resume(); // Receive the next value
  }
  if (millis() - ledTime > 25) {
    digitalWrite(13, LOW);
    digitalWrite(12, LOW);
  }

}

