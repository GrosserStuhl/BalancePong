
#include "Arduino.h"
#include <FastLED.h>

#define NUM_LEDS 49

// Data pin that led data will be written out over
#define DATA_PIN 3

#define COLOR_ORDER GRB
#define CHIPSET WS2812B

#define BRIGHTNESS 20

//Buttons
#define BTN_LEFT 4
#define BTN_RIGHT 5

//Direction for boards
#define LEFT 0
#define RIGHT 1

//Params for buttons
int btn_left_state = 0;
int btn_right_state = 0;

// Params for width and height
const uint8_t kMatrixWidth = 7;
const uint8_t kMatrixHeight = 7;

// Param for different pixel layouts
const bool kMatrixSerpentineLayout = true;

CRGB leds[NUM_LEDS];
int matrix[5];
int ballPos = 0;

/* Matrix layout:
Start ->
{0, 0, 1, 1, 1, 0, 0}  --> Board 1
  0, 0, 0, 0, 0, 0, 0
  0, 0, 0, 0, 0, 0, 0
  0, 0, 1, 0, 0, 0, 0  --> Ball
  0, 0, 0, 0, 0, 0, 0
  0, 0, 0, 0, 0, 0, 0
{0, 0, 1, 1, 1, 0, 0}  --> Board 2
              -> End
*/

int matrix2D[7][7] = {
  {1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1}
};

int board1[7] = {1, 1, 1, 0, 0, 0, 0};
int board2[7] = {0, 0, 0, 0, 1, 1, 1};
//Position as {x, y}
int ballPos2D[2] = {2, 3};

//== FastLED Methods ==
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

//== Normal Arduino Code ==

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

//  leds[ XY( 4, 0) ] = CHSV( random8(), 255, 255);
//  FastLED.show();
}

void updateMatrix2D() {
  resetMatrix2D();

  for (int x = 0; x < kMatrixWidth; x++) {
    //Run through the first row of the matrix and apply board1
    matrix2D[0][x] = board1[x];  
  }
  for (int x = 0; x < kMatrixWidth; x++) {
    //Run through the last row of the matrix and apply board2
    matrix2D[kMatrixHeight-1][x] = board2[x];  
  }

  matrix2D[ballPos2D[0]][ballPos2D[1]] = 1; //set ball pixel
}

void resetMatrix() {
  for (int i = 0; i < NUM_LEDS; i++) {
    matrix[i] = 0;
  }
}
void resetMatrix2D() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; i < kMatrixHeight; i++) {
      matrix2D[i][j] = 0;
    }
  }

//OR

  for(auto& rows: matrix2D){
    for(auto& elem: rows){
        elem = 0;
    }
  }
}

void convertMatrixToLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (matrix[i] == 1) {
      leds[i] = CRGB::Red;
    } else {
      leds[i] = CRGB::Black;
    }
  }

  // Serial.println("==LED Array==");
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   Serial.print("leds["); 
  //   Serial.print(i);
  //   Serial.print("] = ");
  //   Serial.println(leds[i]);
  // }
}

void convertMatrix2DToLEDs() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; i < kMatrixHeight; i++) {
      if (matrix2D[i][j] == 1) {
      leds[XY(i,j)] = CRGB::White;
      } else {
        leds[XY(i,j)] = CRGB::Black;
      }
    }
  }
}

//Takes board 1 or 2 and LEFT or RIGHT
void moveBoard(int board, int dir) {
  int tempBoard[7] = {0}; //create empty 7-point array

  if (board == 1) {
    if (dir == LEFT) {
      if (board1[0] != 1) { //detect if the board is not at max left value already
        for (int i = 0; i < kMatrixWidth; i++) {
          if (board1[i] == 1) tempBoard[i-1] = 1; //Shift all 1's to the left
        }
      }

    } else {
      if (board1[kMatrixWidth-1] != 1) { //detect if the board is not at max right value already
        for (int i = 0; i < kMatrixWidth; i++) {
          if (board1[i] == 1) tempBoard[i+1] = 1; //Shift all 1's to the right
        }
      }
    }
    
    memcpy(tempBoard, board1, sizeof tempBoard); //Does this really safely copy array1 to array2?

  } else if (board == 2) {
    if (dir == LEFT) {
      if (board1[0] != 1) { //detect if the board is not at max left value already
        for (int i = 0; i < kMatrixWidth; i++) {
          if (board1[i] == 1) tempBoard[i-1] = 1; //Shift all 1's to the left
        }
      }

    } else {
      if (board1[kMatrixWidth-1] != 1) { //detect if the board is not at max right value already
        for (int i = 0; i < kMatrixWidth; i++) {
          if (board1[i] == 1) tempBoard[i+1] = 1; //Shift all 1's to the right
        }
      }
    }
    
    memcpy(tempBoard, board1, sizeof tempBoard); //Does this really safely copy array1 to array2?
  }
}

void handleInput() {
  btn_left_state = digitalRead(BTN_LEFT);
  btn_right_state = digitalRead(BTN_RIGHT);

  //This if block has to be duplicated for the other player
  //With 2 individual buttons
  if (btn_left_state == 1) {
    moveBoard(1, LEFT);
    Serial.println("left button pressed");
  } else if (btn_right_state == 1) {
    moveBoard(1, RIGHT);
    Serial.println("right button pressed");
  }
}
