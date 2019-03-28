
#include "Arduino.h"
#include <FastLED.h>
#include <Wire.h> // This library allows you to communicate with I2C devices.

//====== LED Params ======
#define NUM_LEDS 198

#define DATA_PIN 3

#define COLOR_ORDER GRB
#define CHIPSET WS2812B

#define BRIGHTNESS 20

//====== Input Params ======
//Direction for boards
#define LEFT 0
#define RIGHT 1

const uint16_t p1_left_threshhold = 3000;
const uint16_t p1_right_threshhold = -3000;

const uint16_t p2_left_threshhold = 3000;
const uint16_t p2_right_threshhold = -3000;



//====== FastLED Library Params ======
//Params for width and height used by the library
//But because of our own LED matrix structure: 
const uint8_t kMatrixWidth = 18; //This is actually height
const uint8_t kMatrixHeight = 11; //And this width

// Param for different pixel layouts
//Our layout is not a snake, so it's false
const bool kMatrixSerpentineLayout = false;

CRGB leds[NUM_LEDS];

//====== Matrix Data ======
uint8_t matrix[kMatrixWidth][kMatrixHeight] = {0};

/* Matrix layout:
Start (first down, then right)
{0, 0, 1, 1, 1, 0, 0}  --> Board 1
  0, 0, 0, 0, 0, 0, 0
  0, 0, 0, 0, 0, 0, 0
  0, 0, 1, 0, 0, 0, 0  --> Ball
  0, 0, 0, 0, 0, 0, 0
  0, 0, 0, 0, 0, 0, 0
{0, 0, 1, 1, 1, 0, 0}  --> Board 2
              End
*/

//====== Player Board Data ======
//Save boards as starting int + WIDTH
uint8_t b1Start = 0;
uint8_t b2Start = kMatrixHeight / 2 - 1;

//Board width as constant
const uint8_t B_WIDTH = 3;

uint8_t player1Score = 0;
uint8_t player2Score = 0;
bool scoredHit = false;

bool p1_holding_left = false;
bool p1_holding_right = false;

bool p2_holding_left = false;
bool p2_holding_right = false;

//====== Ball Data ======
//Position as {x, y}
int ballPos2D[2] = {2, 3};
//Direction on the board
int ballXDir = 1;
int ballYDir = 1;
uint8_t ballSpeed = 1;



//====== Gyro Params ======
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
const int MPU_ADDR2 = 0x69;

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data


int16_t accelerometer_x2, accelerometer_y2, accelerometer_z2; // variables for accelerometer raw data
int16_t gyro_x2, gyro_y2, gyro_z2; // variables for gyro raw data
int16_t temperature2; // variables for temperature data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

#define AD0_PIN 5



//====== FastLED Methods ======
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

//====== Game Code ======

void resetMatrix() {
  for(auto& rows: matrix){
    for(auto& elem: rows){
        elem = 0;
    }
  }
}

//Reset to center + mirror direction
void resetBallPos() {
  ballPos2D[0] = kMatrixHeight/2;
  ballPos2D[1] = kMatrixWidth/2;
  ballYDir = -ballYDir;
}

void updateMatrix() {
  resetMatrix();

  //Draw boards
  for (int i = b1Start; i < b1Start + B_WIDTH; i++){
    matrix[0][i] = 1;
  }
  for (int i = b2Start; i < b2Start + B_WIDTH; i++){
    matrix[kMatrixWidth - 1][i] = 1;
  }

  //Draw ball pixel
  matrix[ballPos2D[1]][ballPos2D[0]] = 1; //first Y then X (cuz of 2D array logic)

  //For better visualization of ball reaching last row
  //Only reset postion AFTER the last pos was drawn
  //Else it's reset before being visible on last row
  if (scoredHit) {
    resetBallPos();
    scoredHit = false;
    delay(2000);
  }
}


void convertMatrixToLEDs() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      if (matrix[i][j] == 1) {
      leds[XY(i,j)] = CRGB::White;
      } else {
        leds[XY(i,j)] = CRGB::Black;
      }
    }
  }
}

void runValidation() {
  //Check horizontal bounds first
  if(ballPos2D[0]==kMatrixHeight-1 && ballXDir > 0) { //going right ->
    ballXDir = -ballXDir; //Make ball "jump" away from wall
    Serial.println("Hit RIGHT wall, ball dirX mirrored.");
    Serial.println();
  } else if (ballPos2D[0]==0 && ballXDir < 0) {//going left <-
      ballXDir = -ballXDir;
      Serial.println("Hit LEFT wall, ball dirX mirrored.");
      Serial.println();
  }

  //Check against board bounds
  //Check if ball on row directly before board && going towards player
  //+  if ball pixel is inside the board bounds
  //Player1
  else if (ballPos2D[1] == 1 && ballYDir < 0 
      && ballPos2D[0] >=  b1Start && ballPos2D[0] <= b1Start + B_WIDTH - 1) {
    // ballXDir = -ballXDir;
    ballYDir = -ballYDir;
    Serial.println("Ball blocked by P1");
    Serial.println();
  } 
  //Player2
  else if (ballPos2D[1] == kMatrixWidth-2 && ballYDir > 0  
      && ballPos2D[0] >=  b2Start && ballPos2D[0] <= b2Start + B_WIDTH - 1) {
    // ballXDir = -ballXDir;
    ballYDir = -ballYDir;
    Serial.println("Ball blocked by P2");
    Serial.println();
  }

  //Check if scored --> reset ballPos
  if (ballPos2D[1] == 0 && ballYDir < 0) { //against P1, dir up as safety check
    player2Score ++;
    scoredHit = true;
    Serial.println("===> Scored goal against P1");
    Serial.print("Player 1 score: "); Serial.println(player1Score);
  }
  else if (ballPos2D[1] == kMatrixWidth-1 && ballYDir > 0) { //against P2, dir down as safety check
    player1Score ++;
    scoredHit = true;
    Serial.println("===> Scored goal against P2");
    Serial.print("Player 1 score: "); Serial.println(player1Score);
  }
}

void moveBall() {
  //Update position
  ballPos2D[0] += ballXDir;
  ballPos2D[1] += ballYDir;

  runValidation();
}


//Takes board 1 or 2 & LEFT or RIGHT
void moveBoard(int board, byte dir) {

  //Player 1
  if (board == 1) {
    if (dir == LEFT) {
      if (b1Start > 0) { //Is board at max left already?
        b1Start--;
      }
    } else {
      if (b1Start < kMatrixHeight - B_WIDTH) { //Is board at max right?
        b1Start++;
      }
    }
  //Player 2  
  } else if (board == 2) {
    if (dir == LEFT) {
      if (b2Start > 0) { //Is board2 at max left already?
        b2Start--;
      }
    } else {
      if (b2Start < kMatrixHeight - B_WIDTH) { //Is board2 at max right?
        b2Start++;
      }
    }
  }
}

void handleInput() {
  //P1
  if (p1_holding_left) {
    moveBoard(1, LEFT);
    //Here delay neccessary?
    Serial.println("### P1 holding left");
  } else if (p1_holding_right) {
    moveBoard(1, RIGHT);
    //Here as well
    Serial.println("### P1 holding right");
  }

  //Make separate IFs, so players don't block each other

  //P2
  if (p2_holding_left) {
    moveBoard(2, LEFT);
    Serial.println("### P2 holding left");
  } else if (p2_holding_right) {
    moveBoard(2, RIGHT);
    Serial.println("### P2 holding right");
  }
}

//== Serial matrix print ==
void printOutMatrix() {
  byte i = 0;
  Serial.println("===MATRIX===");
  for(auto& rows: matrix){
        Serial.print("Row ");
        Serial.print(i);
        Serial.print(": ");
    for(auto& elem: rows){
        Serial.print(elem);
        Serial.print(",");
    }
    i++;
    Serial.println();
  }
  
  Serial.println("===BALL==");
  Serial.print("DirX: ");
  Serial.println(ballXDir);
  Serial.print("DirY: ");
  Serial.println(ballYDir);
  Serial.print("BallPosX: ");
  Serial.println(ballPos2D[0]);
  Serial.print("BallPosY: ");
  Serial.println(ballPos2D[1]);

  Serial.println();
}

//====== Gyro Methods ======

void setupGyro() {
  //Setting the AD0 pin of one gyro to high changes his address, so they are differentiable
  pinMode(AD0_PIN, OUTPUT);
  digitalWrite(AD0_PIN, HIGH);

  Wire.begin();

  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  delay(50);

  Wire.beginTransmission(MPU_ADDR2); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void gyroLoop() {
  //Player 1's Balance Board
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read() << 8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read() << 8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

    // print out data
  Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature / 340.00 + 36.53);
  Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
  Serial.println();

  delay(50);

  //Player 2's Balance Board
  Wire.beginTransmission(MPU_ADDR2);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR2, 7 * 2, true); // request a total of 7*2=14 registers

  accelerometer_x2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z2 = Wire.read() << 8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

  // print out data2
  Serial.print("aX2 = "); Serial.print(convert_int16_to_str(accelerometer_x2));
  Serial.print(" | aY2 = "); Serial.print(convert_int16_to_str(accelerometer_y2));
  Serial.print(" | aZ2 = "); Serial.print(convert_int16_to_str(accelerometer_z2));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp2 = "); Serial.print(temperature2 / 340.00 + 36.53);
  Serial.print(" | gX2 = "); Serial.print(convert_int16_to_str(gyro_x2));
  Serial.print(" | gY2 = "); Serial.print(convert_int16_to_str(gyro_y2));
  Serial.print(" | gZ2 = "); Serial.print(convert_int16_to_str(gyro_z2));
  Serial.println();

  //Player 1 check
  if (accelerometer_y < p1_left_threshhold) {
    p1_holding_left = true;
    p1_holding_right = false;
  } else if (accelerometer_y > p1_right_threshhold) {
    p1_holding_left = false;
    p1_holding_right = true;
  } else {
    p1_holding_left = false;
    p1_holding_right = false;
  }

  //Player 2 check
  if (accelerometer_y2 < p2_left_threshhold) {
    p2_holding_left = true;
    p2_holding_right = false;
  } else if (accelerometer_y2 > p2_right_threshhold) {
    p2_holding_left = false;
    p2_holding_right = true;
  } else {
    p2_holding_left = false;
    p2_holding_right = false;
  }
}


//====== Arduino Methods ======

void setup()
{
  //sanity delay for LEDs
  delay(2000);

  Serial.begin(9600);
  setupGyro();

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {

  Serial.println("===== New Frame =====");

  gyroLoop();
  handleInput();
  moveBall();
  updateMatrix();
  convertMatrixToLEDs();
  FastLED.show();

  delay(500);
}
