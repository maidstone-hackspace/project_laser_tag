// Included libraries
#include <LiquidCrystal.h>
#include <Button.h>
#include <IRremote.h>
#include <FastCRC.h>

FastCRC16 CRC16;

#define PULLUP false
#define INVERT false
#define DEBOUNCE_MS 20
#define TRIGGER_PIN 2
#define IR_PIN      5
#define BUZZER_PIN  4
#define FLASH_PIN   3        // Muzzle flash LED
#define UP_PIN      14
#define DOWN_PIN    13
#define ENTER_PIN   15

#define red    23
#define green  22
#define blue   21

#define ON LOW
#define OFF HIGH

// Object creation
IRsend irsend;
Button triggerBtn(TRIGGER_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button upBtn(UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button downBtn(DOWN_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button enterBtn(ENTER_PIN, PULLUP, INVERT, DEBOUNCE_MS);
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// default settings
int MAX_AMMO = 5000;
int AMMO = MAX_AMMO;
double HEALTH = 10000;
int CHARGES = 5;
float TEMP = 0;

// state machine variables
byte gameState = 0;
byte menuState = 0;

struct PACKET {
  uint8_t TEAM;
  uint8_t PLAYER;
  uint8_t DAMAGE;
};

struct PACKET LASER;

uint32_t dataPacket;

// Lookup table for 4 bit damage
static const uint8_t damageTable[16] = {1, 2, 4, 5, 7, 10, 15, 17, 20, 25, 30, 35, 40, 50, 75, 100};

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.home();
  lcd.print("  MHS Lasertag");
  lcd.setCursor(2, 1);
  lcd.print("Game System");
  pinMode(FLASH_PIN, OUTPUT);
  pinMode(IR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);

  delay(1000);
  lcd.clear();

  LASER.TEAM = 0;
  LASER.PLAYER = 0;
  LASER.DAMAGE = 8;

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
}

void setLED(byte colour) {
  switch (colour) {
    case 0: // red
      digitalWrite(red, ON);
      digitalWrite(green, OFF);
      digitalWrite(blue, OFF);
      break;
    case 1: // green
      digitalWrite(red, OFF);
      digitalWrite(green, ON);
      digitalWrite(blue, OFF);
      break;
    case 2: // blue
      digitalWrite(red, OFF);
      digitalWrite(green, OFF);
      digitalWrite(blue, ON);
      break;
    case 3: // yellow
      digitalWrite(red, ON);
      digitalWrite(green, ON);
      digitalWrite(blue, OFF);
      break;
    case 4: // cyan
      digitalWrite(red, OFF);
      digitalWrite(green, ON);
      digitalWrite(blue, ON);
      break;
    case 5: // magenta
      digitalWrite(red, ON);
      digitalWrite(green, OFF);
      digitalWrite(blue, ON);
      break;
    case 6: // white
      digitalWrite(red, ON);
      digitalWrite(green, ON);
      digitalWrite(blue, ON);
      break; // off
    case 7:
      digitalWrite(red, OFF);
      digitalWrite(green, OFF);
      digitalWrite(blue, OFF);
      break;
    default:
      digitalWrite(red, OFF);
      digitalWrite(green, OFF);
      digitalWrite(blue, OFF);
      break;
  }
}
void laserSound() {
  digitalWrite(FLASH_PIN, HIGH);
  for (int start = 50; start < 175; start = start + 1) {
    digitalWrite(BUZZER_PIN, HIGH);  //positive square wave
    delayMicroseconds(start);      //192uS
    digitalWrite(BUZZER_PIN, LOW);     //neutral square wave
    delayMicroseconds(start);      //192uS
  }
  digitalWrite(FLASH_PIN, LOW);
}

void recharge() {
  float steps = 1;
  for (int start = 3000; start > 0; start = start - (int)steps) {
    digitalWrite(BUZZER_PIN, HIGH);  //positive square wave
    delayMicroseconds(start);      //192uS
    digitalWrite(BUZZER_PIN, LOW);     //neutral square wave
    delayMicroseconds(start);      //192uS
    steps = steps * 1.005;
  }
}

void waitForReload() {
  recharge();
  AMMO = MAX_AMMO;
  TEMP = 0;
  CHARGES = CHARGES - 1;
}

void updateDisplay() {
  // Print number of charges
  lcd.setCursor(1, 0);
  lcd.print("C:");
  lcd.print(CHARGES);
  lcd.print(" ");

  // print number of rounds
  lcd.setCursor(9, 0);
  lcd.print("R:");
  lcd.print(AMMO);
  lcd.print(" ");

  // print temperature
  lcd.setCursor(1, 1);
  lcd.print("T:");
  lcd.print((int)TEMP);
  lcd.print("% ");

  // print health
  lcd.setCursor(9, 1);
  lcd.print("H:");
  lcd.print(int(ceil(HEALTH/100)));
  lcd.print("% ");
}


void pickTeam() {
  while (1) {
    lcd.home();
    lcd.print("  Pick a Team");
    switch (LASER.TEAM) {
      case 0:
        lcd.setCursor(6, 1);
        lcd.print("RED");
        setLED(0);
        break;
      case 1:
        lcd.setCursor(6, 1);
        lcd.print("BLUE");
        setLED(2);
        break;
      case 2:
        lcd.setCursor(5, 1);
        lcd.print("YELLOW");
        setLED(3);
        break;
      case 3:
        lcd.setCursor(6, 1);
        lcd.print("GREEN");
        setLED(1);
        break;
    }
    upBtn.read();                    //Read the button
    if (upBtn.wasReleased()) {
      LASER.TEAM += 1;
      if (LASER.TEAM > 3) LASER.TEAM = 0;
      lcd.clear();
    }
    downBtn.read();
    if (downBtn.wasReleased()) {
      LASER.TEAM -= 1;
      if (LASER.TEAM == 255) LASER.TEAM = 3;
      Serial.println(LASER.TEAM);
      lcd.clear();
    }
    enterBtn.read();
    if (enterBtn.wasReleased()) {
      menuState = 1;
      return;
    }
  }
}

void pickPlayer() {
  while (1) {
    lcd.home();
    lcd.print("Pick a Player ID");
    lcd.setCursor(8, 1);
    lcd.print(LASER.PLAYER);
    upBtn.read();                    //Read the button
    if (upBtn.wasReleased()) {
      LASER.PLAYER += 1;
      if (LASER.PLAYER > 31) LASER.PLAYER = 0;
      lcd.clear();
    }
    downBtn.read();
    if (downBtn.wasReleased()) {
      LASER.PLAYER -= 1;
      if (LASER.PLAYER == 255) LASER.PLAYER = 31;
      Serial.println(LASER.PLAYER);
      lcd.clear();
    }
    enterBtn.read();
    if (enterBtn.wasReleased()) {
      menuState = 2;
      lcd.clear();
      return;
    }
  }
}

void displayMenu() {
  while (1) {
    lcd.clear();
    lcd.home();
    switch (menuState) {
      case 0:
        pickTeam();
        break;
      case 1:
        pickPlayer();
        break;
      case 2:
        gameState = 1;  // game on
        setLED(1);
        lcd.clear();
        dataPacket = (LASER.TEAM << 9) | (LASER.PLAYER << 4) | LASER.DAMAGE;
        uint8_t packetBuffer[2] = {dataPacket >> 8, (dataPacket << 8) >> 8};
        uint16_t CRC = CRC16.ccitt(packetBuffer, sizeof(packetBuffer));
        dataPacket = (CRC << 16) | (LASER.TEAM << 9) | (LASER.PLAYER << 4) | LASER.DAMAGE;
        return;
    }
  }
}

void cooldown() {
  for (float countdown = 0; countdown < 100; countdown = countdown + 0.1) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(countdown * 5);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(countdown);
    TEMP = 100 - countdown;
    updateDisplay();

  }
}


void loop() {
  if (!gameState) displayMenu();
  updateDisplay();

  triggerBtn.read();                    //Read the button

  if (triggerBtn.isPressed()) {       //If the button was pressed, change the LED state
    if (!(CHARGES || AMMO)) return;
    irsend.sendSony(dataPacket, 32);
    //Serial.println(dataPacket, HEX); // for debugging
    laserSound();
    AMMO -= 1;
    //TEMP = TEMP + 3;
    if (TEMP > 100) cooldown();
  }
  if (AMMO == 0  && CHARGES) {
    updateDisplay();
    waitForReload();
  }
  TEMP = TEMP - 0.25;
  if (TEMP < 0) TEMP = 0;
  setLED(HEALTH>=66 ? 1 : HEALTH>=33 ? 3 : HEALTH>=0 ? 0 : 7);
}



