
#include "Arduino.h"
#include <FastLED.h>

#define NUM_LEDS 20

// Data pin that led data will be written out over
#define DATA_PIN 3

#define COLOR_ORDER GRB
#define CHIPSET WS2812B

#define BRIGHTNESS 20

//Buttons
#define BTN_LEFT 4
#define BTN_RIGHT 5

//Params for buttons
int btn_left_state = 0;
int btn_right_state = 0;

// Params for width and height
const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 1;

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;

CRGB leds[NUM_LEDS];
int matrix[5];
int ballPos = 0;

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

uint16_t XY(uint8_t x, uint8_t y)
{
  uint16_t i;

  if (kMatrixSerpentineLayout == false)
  {
    i = (y * kMatrixWidth) + x;
  }

  if (kMatrixSerpentineLayout == true)
  {
    if (y & 0x01)
    {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    }
    else
    {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}

void DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  byte lineStartHue = startHue8;
  for (byte y = 0; y < kMatrixHeight; y++)
  {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;
    for (byte x = 0; x < kMatrixWidth; x++)
    {
      pixelHue += xHueDelta8;
      leds[XY(x, y)] = CHSV(pixelHue, 255, 255);
    }
  }
}

void doCrazyShit()
{
  uint32_t ms = millis();
  int32_t yHueDelta32 = ((int32_t)cos16(ms * (27 / 1)) * (350 / kMatrixWidth));
  int32_t xHueDelta32 = ((int32_t)cos16(ms * (39 / 1)) * (310 / kMatrixHeight));
  DrawOneFrame(ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
  if (ms < 5000)
  {
    FastLED.setBrightness(scale8(BRIGHTNESS, (ms * 256) / 5000));
  }
  else
  {
    FastLED.setBrightness(BRIGHTNESS);
  }
  FastLED.show();
}

void setup()
{
  //sanity delay
  delay(2000);

  Serial.begin(9600);

  // initialize LED digital pin as an output.
  // pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_RIGHT, INPUT);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  for (int i = 0; i < NUM_LEDS; i++) {
    matrix[i] = 0;
  }
}

void loop() {

  handleInput();
  resetMatrix();
  matrix[ballPos] = 1;
  convertMatrixToLEDs();
  FastLED.show();
  Serial.println(ballPos);
  delay(200);

//    doCrazyShit();

  // leds[XY(1, 1)] = CRGB::Red;
  // FastLED.show();

  // delay(500);

  // leds[XY(1, 1)] = CRGB::Black;
  // leds[XY(3, 1)] = CRGB::Red;
  // FastLED.show();

  //byte x = 1;
  //byte y = 1;

//  leds[ XY( 4, 0) ] = CHSV( random8(), 255, 255);
//  FastLED.show();
}

void resetMatrix() {
  for (int i = 0; i < NUM_LEDS; i++) {
    matrix[i] = 0;
  }
}

void convertMatrixToLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (matrix[i] == 1) {
      Serial.print("matrix[i] == 1 for i:");
      Serial.println(i);
      leds[i] = CRGB::Red;
    } else {
      Serial.print("matrix[i] == 0 for i:");
      Serial.println(i);
      leds[i] = CRGB::Black;
    }
  }

  Serial.println("==LED Array==");
  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.print("leds["); 
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(leds[i]);
  }

}

void handleInput() {
  btn_left_state = digitalRead(BTN_LEFT);
  btn_right_state = digitalRead(BTN_RIGHT);

  if (btn_left_state == 1 && ballPos > 0) {
    ballPos--;
    Serial.println("left button pressed");
  } else if (btn_right_state == 1 && ballPos < NUM_LEDS) {
    ballPos++;
    Serial.println("right button pressed");
  }
}
