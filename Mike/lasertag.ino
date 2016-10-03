// Included libraries
#include <LiquidCrystal.h>  // included with IDE
#include <Button.h>         //https://github.com/JChristensen/Button
#include <IRremote.h>       // https://github.com/z3t0/Arduino-IRremote
#include <FastCRC.h>        // https://github.com/FrankBoesing/FastCRC

#define FIRMWARE 0.15       // Firmware version

FastCRC16 CRC16;            // Create a Fast 16 bit CRC object

#define PULLUP false        // Do NOT use internal pullups on buttons
#define INVERT false        // Do NOT invert the button logic
#define DEBOUNCE_MS 50      // Set button debounce at 20ms

// Game Modes
#define GAME_ON             1
#define NO_AMMO_MAGS_LEFT   2
#define OUT_OF_AMMO         3
#define DEAD                4

// Input Pins
#define TRIGGER_PIN 2
#define RELOAD_PIN  6
#define DOWN_PIN    13
#define UP_PIN      14
#define ENTER_PIN   15
#define RECV_PIN    16

// Output Pins
#define FLASH_LED   3
#define BUZZER_PIN  4
#define IR_LED      5
#define HIT_LED     17
#define RED_LED     21
#define BLUE_LED    22
#define GREEN_LED   23

#define ON LOW            // RGB LED is Common Anode so
#define OFF HIGH          // are pulled LOW to turn ON

// OBJECT CREATION
// Infrared objects send/receive
IRsend irsend;
IRrecv irrecv(RECV_PIN);
// Button Objects
Button TRIGGER_BUTTON(TRIGGER_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button RELOAD_BUTTON(RELOAD_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button LEFT_BUTTON(UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button RIGHT_BUTTON(DOWN_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button ENTER_BUTTON(ENTER_PIN, PULLUP, INVERT, DEBOUNCE_MS);
// Display Object
LiquidCrystal LCD(12, 11, 10, 9, 8, 7);

decode_results RESULT;   // For storing receiered IR packets
unsigned long ledTime;    // For storing last time HIT_LED was lit

// default settings
int AMMO_PER_MAG = 30;    // 30 rounds per magazine
int AMMO = AMMO_PER_MAG;  // Ammo remain in current magazine
int HEALTH = 10000;       // Lots of health for Terminator mode
int MAGAZINES = 5;        // Number of magazines per game

// State Machine variables
int GAME_STATE = 1; // 1 = Normal Mode; 2 = No Ammo/Mags Left; 3 = No Ammo/No Mags; 4 = Dead
int MENU_STATE = 1;
int RGB_STATE = 1;

// Struct for storing the Infrared Data Packet
struct PACKET {
  uint8_t TEAM;
  uint8_t PLAYER;
  uint8_t DAMAGE;
};

// Create an object of type PACKET
struct PACKET DATA;

// Right 32 bits of 64 bit data packet
uint32_t dataPacket;

// Lookup table for 4 bit damage
uint16_t damageTable[] = {1, 2, 4, 5, 7, 10, 15, 17, 20, 25, 30, 35, 40, 50, 75, 100, 1000};

// Look-up table for team colours
char const* teamTable[] = {"RED", "GREEN", "BLUE", "YELLOW", "CYAN", "MAGENTA", "WHITE", "SOLO"};

void setup() {
  Serial.begin(115200); // for debugging
  delay(1000); // Allow time for the serial port to open
  pinMode(FLASH_LED, OUTPUT);
  pinMode(IR_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(HIT_LED, OUTPUT);
  pinMode(RECV_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  setLED(7); // OFF

  // Welcome message and firmware version
  LCD.begin(16, 2);
  LCD.clear();
  LCD.home();
  LCD.print("  MHS LaserTag");
  LCD.setCursor(1, 1);
  LCD.print("Firmware: ");
  LCD.print((float)FIRMWARE);

  // Wait one second and clear the display
  tone(4, 200, 200);
  delay(200);
  tone(4, 300, 200);
  delay(200);
  tone(4, 500, 200);
  delay(1000);
  LCD.clear();

  // Initialise data struct with defaut values
  DATA.TEAM = 0;
  DATA.PLAYER = 0;
  DATA.DAMAGE = 16;

  PickTeam();   // Pick a team menu
  pickPlayer(); // Pick player ID menu
  setDataPacket() 
  
  LCD.clear();
  DisplayGameStats();
  irrecv.enableIRIn(); // Start the receiver
}

void PickTeam() {
  LCD.clear();
  LCD.print("  Pick a Team");
  Serial.println();
  Serial.println("Pick a team:");
  PrintTeamColour();

  while (1) {
    // If the left button is pressed reduce team number by 1
    LEFT_BUTTON.read();
    if (LEFT_BUTTON.wasReleased()) {
      DATA.TEAM += 1;
      if (DATA.TEAM > 7) DATA.TEAM = 0;
      PrintTeamColour();
    }
    // If the right button is pressed increase team number by 1
    RIGHT_BUTTON.read();
    if (RIGHT_BUTTON.wasReleased()) {
      DATA.TEAM -= 1;
      if (DATA.TEAM == 255) DATA.TEAM = 7;
      Serial.println(DATA.TEAM);
      PrintTeamColour();
    }
    // If the enter button is pressed go to next menu
    ENTER_BUTTON.read();
    if (ENTER_BUTTON.wasReleased()) {
      Serial.println();
      Serial.print(teamTable[DATA.TEAM]);
      Serial.println(" team selected.");

      return;
    }
  }
}

void PrintTeamColour() {
  LCD.setCursor(0, 1);
  LCD.print("                ");
  LCD.setCursor(5, 1);
  LCD.print(teamTable[DATA.TEAM]);
  setLED(DATA.TEAM);
  Serial.println(teamTable[DATA.TEAM]);
}

void pickPlayer() {
  Serial.println();
  Serial.println("Pick a player ID Number:");
  LCD.clear();
  while (1) {
    LCD.home();
    LCD.print("Pick a Player ID");
    LCD.setCursor(7, 1);
    LCD.print(DATA.PLAYER + 1);
    // If the left button is pressed decrease the player ID by 1
    LEFT_BUTTON.read();        //Read the button
    if (LEFT_BUTTON.wasReleased()) {
      DATA.PLAYER += 1;
      if (DATA.PLAYER > 63) DATA.PLAYER = 0;
      LCD.clear();
      Serial.println(DATA.PLAYER + 1);
    }
    // If the right button is pressed increase the player ID by 1
    RIGHT_BUTTON.read();
    if (RIGHT_BUTTON.wasReleased()) {
      DATA.PLAYER -= 1;
      if (DATA.PLAYER == 255) DATA.PLAYER = 63;
      LCD.clear();
      Serial.println(DATA.PLAYER + 1);
    }
    // If the enter button is pressed start game
    ENTER_BUTTON.read();
    if (ENTER_BUTTON.wasReleased()) {
      Serial.println();
      Serial.print("Player ");
      Serial.print(DATA.PLAYER + 1);
      Serial.println(" selected.");
      Serial.println();
      Serial.print("You are Player ");
      Serial.print(DATA.PLAYER + 1);
      Serial.print(" on the ");
      Serial.print(teamTable[DATA.TEAM]);
      Serial.println(" team.");
      Serial.println();
      Serial.println("### GAME STARTED ###");
      Serial.println();

    }
  }
}

void setDataPacket() {
  dataPacket = (LASER.TEAM << 13) | (LASER.PLAYER << 7) | LASER.DAMAGE;
  uint8_t packetBuffer[2] = {dataPacket >> 8, (dataPacket << 8) >> 8};
  uint16_t CRC = CRC16.ccitt(packetBuffer, sizeof(packetBuffer));
  dataPacket = (CRC << 16) | dataPacket;
  Serial.println(dataPacket, HEX); return;
}

void setLED(byte colour) {
  // 0=Red 1=Green 2=Blue 3=Yellow 4=Cyan 5=Magenta 6=WHite 7=Off
  // Code is valid for common anode LED
  switch (colour) {
    case 0: // RED_LED
      digitalWrite(RED_LED, ON);
      digitalWrite(GREEN_LED, OFF);
      digitalWrite(BLUE_LED, OFF);
      break;
    case 1: // GREEN_LED
      digitalWrite(RED_LED, OFF);
      digitalWrite(GREEN_LED, ON);
      digitalWrite(BLUE_LED, OFF);
      break;
    case 2: // BLUE_LED
      digitalWrite(RED_LED, OFF);
      digitalWrite(GREEN_LED, OFF);
      digitalWrite(BLUE_LED, ON);
      break;
    case 3: // yellow
      digitalWrite(RED_LED, ON);
      digitalWrite(GREEN_LED, ON);
      digitalWrite(BLUE_LED, OFF);
      break;
    case 4: // cyan
      digitalWrite(RED_LED, OFF);
      digitalWrite(GREEN_LED, ON);
      digitalWrite(BLUE_LED, ON);
      break;
    case 5: // magenta
      digitalWrite(RED_LED, ON);
      digitalWrite(GREEN_LED, OFF);
      digitalWrite(BLUE_LED, ON);
      break;
    case 6: // white
      digitalWrite(RED_LED, ON);
      digitalWrite(GREEN_LED, ON);
      digitalWrite(BLUE_LED, ON);
      break; // off
    case 7:
      digitalWrite(RED_LED, OFF);
      digitalWrite(GREEN_LED, OFF);
      digitalWrite(BLUE_LED, OFF);
      break;
    default:
      digitalWrite(RED_LED, OFF);
      digitalWrite(GREEN_LED, OFF);
      digitalWrite(BLUE_LED, OFF);
      break;
  }
}

void CheckForHit() {

  // if anything received by the IR Receier then run this:
  if (irrecv.decode(&RESULT)) {

    // RESULT.value contains received 64 bits
    // Pick ou the bits as per protocol using the bitwise calculation below
    // RESULT.value & (((1 << NUMBER_OF_BITS) - 1) << POSITION_OF_LEFTSHIFTED_FIRST_BIT)) >> SHIFT_TO_RIGHT_EDGE
    uint16_t TEAM = (RESULT.value & (((1 << 3) - 1) << 13)) >> 13;
    uint16_t PLAYER = (RESULT.value & (((1 << 6) - 1) << 7)) >> 7;
    uint16_t DAMAGE = (RESULT.value & (((1 << 7) - 1)));
    uint16_t CRC = RESULT.value >> 16;
    uint16_t DATA = (TEAM << 13) | (PLAYER << 7) | DAMAGE;
    // Split the data portion into two 8 bit bytes
    uint8_t MSB = DATA  >> 8;
    uint8_t LSB = (DATA << 8) >> 8;
    // Put those 2 bytes into an array
    uint8_t crcBuffer[2] = {MSB, LSB};
    // Find out what the CRC-16 value is for the data portion
    uint16_t validCRC = CRC16.ccitt(crcBuffer, sizeof(crcBuffer));
    // If there are no errors run this:

    if (validCRC == CRC) {
      DebugIRPacketData(TEAM, PLAYER, DAMAGE);
      HEALTH = HEALTH - damageTable[DAMAGE]; // You've been hit so health is reduced
      if (HEALTH <= 0) {
        HEALTH = 0;
        GAME_STATE = DEAD; // DEAD!

      }
      ledTime = millis();
      digitalWrite(HIT_LED, HIGH);

      setLED(HEALTH >= 6666 ? 1 : HEALTH >= 3333 ? 3 : HEALTH >= 0 ? 0 : 7);
      DisplayGameStats();
    }
    irrecv.resume(); // Receive the next value
  }

  if (millis() - ledTime > 25) {
    digitalWrite(HIT_LED, LOW);
  }

  if (GAME_STATE == DEAD) YouAreDead();
}

void DisplayGameStats() {

  // Print number of shots left
  LCD.clear();
  LCD.setCursor(3, 0);
  LCD.print("Shots: ");
  LCD.print(AMMO);
  LCD.print(" ");

  // print health
  LCD.setCursor(2, 1);
  LCD.print("Health: ");
  LCD.print(int(ceil(HEALTH / 100)));
  LCD.print("% ");
  Serial.print("Shots: ");
  Serial.print(AMMO);
  Serial.print("  Health: ");
  Serial.println(int(ceil(HEALTH / 100)));
}

void CheckIfTriggerPressed() {

  TRIGGER_BUTTON.read();  //Read the button

  if (TRIGGER_BUTTON.isPressed()) { //If the button was pressed....
    if (GAME_STATE == GAME_ON) {

      if (!(MAGAZINES || AMMO)) return; // If there is no ammo left don't send data packet

      irsend.sendSony(dataPacket, 32); // Serial.println(dataPacket, HEX); // for debugging
      irrecv.enableIRIn(); // Start the receiver
      PlayLaserSound();
      AMMO = AMMO - 1;
      DisplayGameStats();

      if ((AMMO == 0)  && (MAGAZINES > 0)) {
        GAME_STATE = NO_AMMO_MAGS_LEFT; // Out of Ammo/Mags left

        LCD.clear();
        LCD.print("  !! RELOAD !!");
        LCD.setCursor(2, 1);
        LCD.print(MAGAZINES);
        LCD.print(" MAGS LEFT");
        Serial.print("Reload !!! ");
        Serial.print(MAGAZINES);
        Serial.println(" magazines left.");
      }

      if ((AMMO == 0)  && (MAGAZINES == 0)) {
        GAME_STATE = OUT_OF_AMMO; // Out of Ammo/No mags left
        LCD.clear();
        LCD.print("  OUT OF AMMO");
        LCD.setCursor(2, 1);
        LCD.print("NO MAGAZINES");
        Serial.print("NO AMMO LEFT !!!");
      }
    }

    if (GAME_STATE != GAME_ON) PlayErrorBeep();


  }

}

void PlayErrorBeep() {
  tone(4, 80, 100);
}

void CheckIfReloadPressed() {

  RELOAD_BUTTON.read();  //Read the button

  if (RELOAD_BUTTON.isPressed()) {

    if (GAME_STATE == GAME_ON) {
      PlayErrorBeep();
      return;
    }

    if (GAME_STATE == NO_AMMO_MAGS_LEFT) {
      //if (AMMO>0) PlayErrorBeep();
      MAGAZINES = MAGAZINES - 1;

      Serial.println("Reloading!");
      PlayRechargeSound();
      AMMO = AMMO_PER_MAG;
      DisplayGameStats();
      GAME_STATE = GAME_ON;
    }

    if (GAME_STATE == OUT_OF_AMMO) PlayErrorBeep();

  }
}

// TBD - CONVERT TO STATE MACHINE
void PlayLaserSound() {
  digitalWrite(FLASH_LED, HIGH);
  for (int start = 50; start < 175; start = start + 1) {
    digitalWrite(BUZZER_PIN, HIGH); //positive square wave
    delayMicroseconds(start); //192uS
    digitalWrite(BUZZER_PIN, LOW); //neutral square wave
    delayMicroseconds(start); //192uS
    CheckForHit();
  }
  digitalWrite(FLASH_LED, LOW);
}

// TBD - CONVERT TO STATE MACHINE
void PlayRechargeSound() {
  float steps = 1;
  for (int start = 3000; start > 100; start = start - (int)steps) {
    digitalWrite(BUZZER_PIN, HIGH); //positive square wave
    delayMicroseconds(start); //192uS
    digitalWrite(BUZZER_PIN, LOW); //neutral square wave
    delayMicroseconds(start); //192uS
    steps = steps * 1.004;
    CheckForHit();
    if (millis() - ledTime > 25) {
      digitalWrite(HIT_LED, LOW);
    }
  }
}

void YouAreDead() {
  Serial.println("You have been EXTERMINATED!");
  digitalWrite(BLUE_LED, OFF);
  digitalWrite(GREEN_LED, OFF);
  digitalWrite(HIT_LED, LOW);
  int count = 0;
  byte repeats = 30;
  while (1) {
    digitalWrite(RED_LED, ON);
    if (count < repeats) tone(4, 250, 100);
    delay(100);
    digitalWrite(RED_LED, OFF);
    if (count < repeats) tone(4, 150, 100);
    delay(100);
    count++;
  }
}

void DebugIRPacketData(uint16_t TEAM, uint16_t PLAYER, uint16_t DAMAGE) {
  Serial.print("Hit by Player ");
  Serial.print(PLAYER + 1);
  Serial.print(" from the ");
  Serial.print(teamTable[TEAM]);
  Serial.print(" team for ");
  Serial.print(damageTable[DAMAGE]);
  Serial.print(" Damage.       HEALTH:");
  Serial.println(int(ceil(HEALTH / 100)));
}

void loop() {

  CheckForHit();
  CheckIfTriggerPressed();
  CheckIfReloadPressed();

}
