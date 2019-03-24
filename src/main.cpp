
#include "Arduino.h"
#include <FastLED.h>

#define NUM_LEDS 5

// Data pin that led data will be written out over
#define DATA_PIN 3

#define COLOR_ORDER GRB
#define CHIPSET WS2812B

#define BRIGHTNESS 20

// Params for width and height
const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 1;

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;

CRGB leds[NUM_LEDS];

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
  delay(2000);

  // initialize LED digital pin as an output.
  // pinMode(LED_BUILTIN, OUTPUT);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop()
{
  // for (int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1)
  // {
  //   // Turn our current led on to white, then show the leds
  //   leds[whiteLed] = CRGB::White;

  //   // Show the leds (only one of which is set to white, from above)
  //   FastLED.show();

  //   // Wait a little bit
  //   delay(150);

  //   // Turn our current led back to black for the next loop around
  //   leds[whiteLed] = CRGB::Black;
  // }

  // leds[XY(1, 1)] = CRGB::Red;
  // FastLED.show();

  // delay(500);

  // leds[XY(1, 1)] = CRGB::Black;
  // leds[XY(3, 1)] = CRGB::Red;
  // FastLED.show();

  byte x = 1;
  byte y = 1;

  leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
  FastLED.show();
}