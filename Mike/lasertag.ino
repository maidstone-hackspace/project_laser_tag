#include <Arduino.h>

// Included libraries
#include <LiquidCrystal.h>
#include <Button.h>
#include <IRremote.h>
#include <FastCRC.h>

#define FIRMWARE 0.8

FastCRC16 CRC16;

#define PULLUP false
#define INVERT false
#define DEBOUNCE_MS 20

// pins used
#define TRIGGER_PIN 2
#define FLASH_LED   3
#define BUZZER_PIN  4
#define IR_LED      5
#define RELOAD_PIN  6
#define DOWN_PIN    13
#define UP_PIN      14
#define ENTER_PIN   15
#define RECV_PIN    16
#define HIT_LED     17
#define RED_LED     21
#define BLUE_LED    22
#define GREEN_LED   23

#define ON LOW
#define OFF HIGH

// Object creation
IRsend irsend;
IRrecv irrecv(RECV_PIN);
Button triggerBtn(TRIGGER_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button reloadBtn(RELOAD_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button
Button upBtn(UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button downBtn(DOWN_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button enterBtn(ENTER_PIN, PULLUP, INVERT, DEBOUNCE_MS);
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

decode_results results;
unsigned long ledTime;

// default settings
int MAX_AMMO = 30;
int AMMO = MAX_AMMO;
double HEALTH = 10000;
int CHARGES = 10;
float TEMP = 0;

// state machine variables
byte gameState = 0; // 0 = Menu; 2 = Game On; 3 = Out Of Ammo; 4 = Dead
byte menuState = 0;

struct PACKET {
        uint8_t TEAM;
        uint8_t PLAYER;
        uint8_t DAMAGE;
};

struct PACKET LASER;

uint32_t dataPacket;

// Lookup table for 4 bit damage
uint16_t damageTable[] = {1, 2, 4, 5, 7, 10, 15, 17, 20, 25, 30, 35, 40, 50, 75, 100, 1000};

// Look-up table for team colours
char const* teamTable[] = {"RED", "BLUE", "GREEN", "YELLOW", "CYAN", "MAGENTA", "WHITE", "SOLO"};

void setup() {
        Serial.begin(115200); // for debugging
        lcd.begin(16, 2);
        lcd.clear();
        lcd.home();
        lcd.print("  MHS Lasertag");
        lcd.setCursor(1, 1);
        lcd.print("Firmware: ");
        lcd.print((float)FIRMWARE);
        pinMode(FLASH_LED, OUTPUT);
        pinMode(IR_LED, OUTPUT);
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(TRIGGER_PIN, INPUT);
        pinMode(HIT_LED, OUTPUT);

        delay(1000);
        lcd.clear();

        LASER.TEAM = 0;
        LASER.PLAYER = 0;
        LASER.DAMAGE = 16;

        pinMode(RED_LED, OUTPUT);
        pinMode(GREEN_LED, OUTPUT);
        pinMode(BLUE_LED, OUTPUT);
        irrecv.enableIRIn(); // Start the receiver
}

void setLED(byte colour) {
        // 0Red 1Green 2Blue 3Yellow 4Cyan 5Magenta 6WHite 7Off
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
void laserSound() {
        digitalWrite(FLASH_LED, HIGH);
        for (int start = 50; start < 175; start = start + 1) {
                digitalWrite(BUZZER_PIN, HIGH); //positive square wave
                delayMicroseconds(start); //192uS
                digitalWrite(BUZZER_PIN, LOW); //neutral square wave
                delayMicroseconds(start); //192uS
        }
        digitalWrite(FLASH_LED, LOW);
}

void recharge() {
        float steps = 1;
        for (int start = 3000; start > 0; start = start - (int)steps) {
                digitalWrite(BUZZER_PIN, HIGH); //positive square wave
                delayMicroseconds(start); //192uS
                digitalWrite(BUZZER_PIN, LOW); //neutral square wave
                delayMicroseconds(start); //192uS
                steps = steps * 1.005;
        }
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
        lcd.print(int(ceil(HEALTH / 100)));
        lcd.print("% ");
}


void pickTeam() {
        while (1) {
                lcd.home();
                lcd.print("  Pick a Team");
                switch (LASER.TEAM) {
                // 0Red 1Green 2Blue 3Yellow 4Cyan 5Magenta 6WHite 7Off
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
                        lcd.print("GREEN");
                        setLED(1);
                        break;
                case 3:
                        lcd.setCursor(5, 1);
                        lcd.print("YELLOW");
                        setLED(3);
                        break;
                case 4:
                        lcd.setCursor(6, 1);
                        lcd.print("CYAN");
                        setLED(4);
                        break;
                case 5:
                        lcd.setCursor(5, 1);
                        lcd.print("MAGENTA");
                        setLED(5);
                        break;
                case 6:
                        lcd.setCursor(5, 1);
                        lcd.print("WHITE");
                        setLED(6);
                        break;
                case 7:
                        lcd.setCursor(6, 1);
                        lcd.print("SOLO");
                        setLED(7);
                        break;
                }
                upBtn.read();        //Read the button
                if (upBtn.wasReleased()) {
                        LASER.TEAM += 1;
                        if (LASER.TEAM > 7) LASER.TEAM = 0;
                        lcd.clear();
                }
                downBtn.read();
                if (downBtn.wasReleased()) {
                        LASER.TEAM -= 1;
                        if (LASER.TEAM == 255) LASER.TEAM = 7;
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
                lcd.setCursor(7, 1);
                lcd.print(LASER.PLAYER + 1);
                upBtn.read();        //Read the button
                if (upBtn.wasReleased()) {
                        LASER.PLAYER += 1;
                        if (LASER.PLAYER > 63) LASER.PLAYER = 0;
                        lcd.clear();
                }
                downBtn.read();
                if (downBtn.wasReleased()) {
                        LASER.PLAYER -= 1;
                        if (LASER.PLAYER == 255) LASER.PLAYER = 63;
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
                        gameState = 1; // game on
                        setLED(1);
                        lcd.clear();
                        dataPacket = (LASER.TEAM << 13) | (LASER.PLAYER << 7) | LASER.DAMAGE;
                        uint8_t packetBuffer[2] = {dataPacket >> 8, (dataPacket << 8) >> 8};
                        uint16_t CRC = CRC16.ccitt(packetBuffer, sizeof(packetBuffer));
                        dataPacket = (CRC << 16) | dataPacket;
                        Serial.println(dataPacket, HEX);
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

void printData(uint16_t TEAM, uint16_t PLAYER, uint16_t DAMAGE) {
        Serial.print("Hit by Player ");
        Serial.print(PLAYER + 1);
        Serial.print(" from the ");
        Serial.print(teamTable[TEAM]);
        Serial.print(" team for ");
        Serial.print(damageTable[DAMAGE]);
        Serial.println(" Damage.");
}

void checkForHit() {

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
                if (validCRC == CRC) {
                        ledTime = millis();
                        digitalWrite(HIT_LED, HIGH);
                        printData(TEAM, PLAYER, DAMAGE);
                        HEALTH = HEALTH-damageTable[DAMAGE];
                        if (HEALTH<=0) {
                                HEALTH==0;
                                gameState=4;
                                updateDisplay();
                        }
                        irrecv.resume(); // Receive the next value
                }
        }
}

void dead() {
        digitalWrite(BLUE_LED, OFF);
        digitalWrite(GREEN_LED, OFF);
        digitalWrite(HIT_LED, LOW);
        int count=0;
        byte repeats=30;
        while(1) {}
        digitalWrite(RED_LED, ON);
        if (count<repeats) tone(4, 250,100);
        delay(100);
        digitalWrite(RED_LED, OFF);
        if (count<repeats) tone(4, 150, 100);
        delay(100);
        count++;
}
}



void loop() {
        updateDisplay();
        checkForHit();

        if (!gameState) displayMenu();

        if (gameState == 1) { // Shooting
                triggerBtn.read();        //Read the button

                if (triggerBtn.isPressed()) { //If the button was pressed, change the LED state
                        if (!(CHARGES || AMMO)) return;
                        irsend.sendSony(dataPacket, 32);
                        //Serial.println(dataPacket, HEX); // for debugging
                        laserSound();
                        AMMO -= 1;
                        if (AMMO == 0  && CHARGES) {
                                updateDisplay();
                                gameState = 2;
                        }
                        //TEMP = TEMP + 4;  // comment out for debuggingd
                        if (TEMP > 100) cooldown();
                }
        }

        if (gameState == 2) { // Wait for reload
                reloadBtn.read();        //Read the button

                if (reloadBtn.isPressed()) {
                        CHARGES = CHARGES - 1;


                        if (CHARGES >= 0)
                        {
                                recharge();
                                AMMO = MAX_AMMO;
                                updateDisplay();
                                gameState = 1;
                        }
                        else gameState = 3; // out of ammo
                }
        }

        if (gameState == 4) dead();

        TEMP = TEMP - 0.25;
        if (TEMP < 0) TEMP = 0;
        if (millis() - ledTime > 25) {
                digitalWrite(HIT_LED, LOW);
        }
        setLED(HEALTH >= 66 ? 1 : HEALTH >= 33 ? 3 : HEALTH >= 0 ? 0 : 7);
}
