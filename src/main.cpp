
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

byte matrix[7][7] = {
  {1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1}
};

byte board1[7] = {1, 1, 1, 0, 0, 0, 0};
byte board2[7] = {0, 0, 0, 0, 1, 1, 1};
//Position as {x, y}
byte ballPos2D[2] = {2, 3};
//Direction on the board
byte ballXDir = 1;
byte ballYDir = 1;
byte ballSpeed = 1;

byte player1Score = 0;
byte player2Score = 0;


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

  // pinMode(BTN_LEFT, INPUT);
  // pinMode(BTN_RIGHT, INPUT);

  // FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.setBrightness(BRIGHTNESS);
}

void loop() {

  handleInput();
  resetMatrix();
  updateMatrix();

  //TODO: Print to Serial in a nice way

  // convertMatrixToLEDs();
  // FastLED.show();
  delay(200);

//    doCrazyShit();

//  leds[ XY( 4, 0) ] = CHSV( random8(), 255, 255);
//  FastLED.show();
}

void updateMatrix() {
  resetmatrix();

  for (int x = 0; x < kMatrixWidth; x++) {
    //Run through the first row of the matrix and apply board1
    matrix[0][x] = board1[x];  
  }
  for (int x = 0; x < kMatrixWidth; x++) {
    //Run through the last row of the matrix and apply board2
    matrix[kMatrixHeight-1][x] = board2[x];  
  }

  matrix[ballPos2D[0]][ballPos2D[1]] = 1; //set ball pixel
}

void resetMatrix() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; i < kMatrixHeight; i++) {
      matrix[i][j] = 0;
    }
  }

//OR

  for(auto& rows: matrix){
    for(auto& elem: rows){
        elem = 0;
    }
  }
}

void convertMatrixToLEDs() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; i < kMatrixHeight; i++) {
      if (matrix[i][j] == 1) {
      leds[XY(i,j)] = CRGB::White;
      } else {
        leds[XY(i,j)] = CRGB::Black;
      }
    }
  }
}

void moveBall() {
  //Check horizontal bounds first
  if(ballPos2D[0]==kMatrixWidth-1 && ballXDir > 0) { //going right ->
      ballXDir = -ballXDir;
  } else if (ballPos2D[0]==0 && ballXDir < 0) {//going left <-
      ballXDir = -ballXDir;
  }

  //Check against board bounds
  //Against P1's board
  if (ballPos2D[1] == 1 && ballYDir < 0 && ballPos2D[0] == ) {
    
  }

  //Check if scored
  if (ballPos2D[1] == 0 && ballYDir < 0) { //against P1, dir up as safety check
    player2Score ++;
    resetBallPos();

  } else if (ballPos2D[1] == kMatrixHeight-1 && ballYDir > 0) { //against P2, dir down as safety check
    player1Score ++;
    resetBallPos();
  }
}

//Reset to center + mirror direction
void resetBallPos() {
  ballPos2D[0] = kMatrixWidth/2;
  ballPos2D[1] = kMatrixHeight/2;
  ballYDir = -ballYDir;
}

//Takes board 1 or 2 and LEFT or RIGHT
void moveBoard(int board, byte dir) {
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
