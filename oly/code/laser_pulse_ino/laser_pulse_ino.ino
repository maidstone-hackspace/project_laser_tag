/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int ir_led = 4;
int rgb_led = 3;
int buzzer = 2;

int trigger = 5;

int buttonState = 0;    


// the setup routine runs once when you press reset:
void setup() {                
  // ouputs
  pinMode(rgb_led, OUTPUT);   
  pinMode(ir_led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  //inputs initial state
  pinMode(trigger, INPUT);

  analogWrite(rgb_led, 255);
  digitalWrite(ir_led, LOW); 
}

void recharge_simple(float multiplier) {
  float steps;
  steps = 1;
  
  float ledIntensity = 0;

  for (int waveDelay = 100; waveDelay < 20000; waveDelay = waveDelay + (int) steps) {
    analogWrite(rgb_led, (int) ledIntensity);
    analogWrite(buzzer, 255);  //positive square wave
    delayMicroseconds(waveDelay);      //192uS

    analogWrite(buzzer, 0);     //neutral square wave
    delayMicroseconds(waveDelay);      //192uS
    steps = steps * multiplier;
    ledIntensity = waveDelay / 255;    // step the led intensity
  }  
  
  // lets just max out the led recharge cycle complete
  analogWrite(rgb_led, 255);
}

// the loop routine runs over and over again forever:
void loop() {
  buttonState = digitalRead(trigger);
  if (buttonState == HIGH) {
    // do the ir stuff, trigger was just pressed
    digitalWrite(ir_led, HIGH);
    delay(100);
    digitalWrite(ir_led, LOW);

    // start  recharge cycle
    recharge_simple(1.030);

  }


}
