#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <BH1750.h>        // https://github.com/claws/BH1750
#include <JC_Button.h>     // https://github.com/JChristensen/JC_Button
#include "LowPower.h"      // https://github.com/rocketscream/Low-Power

#define BUTTON_PIN 7
#define NUM_LEDS 75

CRGB leds[NUM_LEDS];
Button button(BUTTON_PIN);
BH1750 lightMeter;


// Function to calculate color for each LED
CRGB calculateColor(int ledNumber, float density) {
    // Adjust these values to change the color and fading effect
    int hue = (ledNumber * 10) % 255; // Example hue based on LED position
    int saturation = 255;
    int brightness = (int)(density * 255.0); // Scale density to 0-255

    return CHSV(hue, saturation, brightness);
}

void runAnimation(int lamp_animation){
  switch (lamp_animation) {
    case 0:  // solid warm white
      FastLED.clear();
      for (int i=0; i<NUM_LEDS; i++){
        leds[i] = CRGB(160, 105, 50);
      }
      FastLED.show();
      break;
    case 1:   // slow color fade
      FastLED.clear();
      for (int i=0; i<NUM_LEDS; i++){
        leds[i] = CRGB(
          sin((0.5+millis()/20000.0+i/50.0))*255, 
          cos((millis()/15000.0+i/70.0))*255, 
          cos((1.5+millis()/24000.0+i/90.0))*255
        );
      }
      FastLED.show();
      break;

    case 2:  // droplets
      FastLED.clear();
      for (int i = 0; i < NUM_LEDS; i++) {
        // Example density calculation - replace with your own logic
        float density = sin(millis() / 500.0 + i) * 0.25 + 0.5; // Oscillating density
        leds[i] = calculateColor(i, density);
      }
      FastLED.show();      
      break;

    case 3: // slow rainbow
      FastLED.clear();
      for (int i=0; i<NUM_LEDS; i++){
        leds[i] = CHSV((millis()/100 + i*10) % 255, 255, 255);
      }
      FastLED.show();
      break;

    default:
      break;
  }
}

void setup() {

  Serial.begin(9600);
  Serial.println("Solar Lamp 2023 - Jeremy Seitz");

  Wire.begin();
  lightMeter.begin();
  button.begin();

  pinMode(13, OUTPUT);
  // init fastled on pin 5
  FastLED.addLeds<WS2812B, 5, GRB>(leds, NUM_LEDS);

  // set brightness
  FastLED.setBrightness(150);
  FastLED.clear();
  FastLED.show();

}

#define MAX_ANIMATIONS 4
int lamp_animation = 0;
float avg_lux = 0;
long lastMillis = 0;
long animationMillis = 0;
bool lamp_state = true;   // main lighting turned on or not?
bool long_press = false;  // long press of button?
bool ledState = false;  // state of onboard LED
void loop() {
  // check light level
  // compute average lux over the last 30 seconds
  avg_lux = (avg_lux * 29 + lightMeter.readLightLevel()) / 30.0;
  delay(20);
  if (avg_lux > 100) {
    Serial.println("Light level is too high, going to sleep");
    FastLED.clear();
    FastLED.show();    
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    return;
  }

  // -- run animation
  if (millis() - animationMillis > 10) {
    if (lamp_state == false){
      FastLED.clear();
      FastLED.show();
      Serial.println("sleep...");
      LowPower.idle(SLEEP_250MS, ADC_OFF, TIMER4_OFF, TIMER3_OFF, TIMER1_OFF, 
      		  TIMER0_OFF, SPI_OFF, USART1_OFF, TWI_OFF, USB_OFF);      
    } else {
      runAnimation(lamp_animation);
    }
    animationMillis = millis();
  }

  // --- input checks
  button.read();

  // check if 1 second has passed
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();

    // read light level
    float lux = lightMeter.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");

    Serial.print("FastLED FPS: ");  
    Serial.println(FastLED.getFPS());
    
    // toggle led
    ledState = !ledState;
    digitalWrite(13, ledState);
  }

  // check if button is pressed, and display regular, double or long press
  if (button.wasReleased()) {
    Serial.println("Button was released!");
    if (long_press) {
      long_press = false;
    } else {
      if (lamp_state == false){
        lamp_state = true;
        Serial.println("turn on main lamp");
      } else {
        lamp_animation = (lamp_animation + 1) % MAX_ANIMATIONS;
        Serial.print("lamp animation: ");
        Serial.println(lamp_animation);
      }
    }
  }
  if (button.pressedFor(2000)) {
    if (button.isPressed() && !long_press) {
      long_press = true;
      Serial.println("long press!");
      lamp_state = false; // turn off main lamp
    } 
  }

}