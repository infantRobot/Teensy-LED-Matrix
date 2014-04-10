/* Created with resources from Adafruit, Thanks! http://learn.adafruit.com/32x16-32x32-rgb-led-matrix?view=all
 * Game of life from Andrew at http://pastebin.com/f22bfe94d
 * PITimer.h by Daniel Gilbert, Thanks a ton! https://github.com/loglow/PITimer
 *
 *
 * Creater Alex Medeiros, http://PenguinTech.info
 * _16x32_Matrix R3.0
 * Use code freely and distort its contents as much as you want, just remeber to thank the 
 * original creaters of the code by leaving their information in the header. :)
 * 
 * There may be bug in the code that I have not found, if you experiance any please let me 
 * know so I can fix it and updat this project. And feel free to ask any question about the code.
 *
 */

#include <PITimer.h> //This saved the project!
#include "colors.h"

//Define pins
const uint8_t
CLOCKPIN  = 27, LATCHPIN  = 29, OEPIN     = 30,
APIN      = 11, BPIN      = 12, CPIN      = 28,
R1PIN     = 15, R2PIN     = 22,
G1PIN     = 23, G2PIN     = 9,
B1PIN     = 10, B2PIN     = 13;

uint8_t pinTable[12] = {
  R1PIN,R2PIN,G1PIN,G2PIN,B1PIN,B2PIN,
  APIN,BPIN,CPIN,CLOCKPIN,LATCHPIN,OEPIN};

//Acts like a 16 bit shift register
uint16_t const SCLK   = 0x200;
uint16_t const LATCH  = 0x400;
uint16_t const OE     = 0x800;

//Addresses 1/8 rows Through a decoder
uint16_t const A = 0x040, B = 0x080,C = 0x100;

uint16_t const abcVar[8] = { //Decoder counter var
  0,A,B,A+B,C,C+A,C+B,A+B+C};

//Data Lines for row 1 red and row 9 red, ect.
uint16_t const RED1   = 0x001, RED2   = 0x002;
uint16_t const GREEN1 = 0x004, GREEN2 = 0x008;
uint16_t const BLUE1  = 0x010, BLUE2  = 0x020;

//Here is where the data is all read
uint8_t rMatrix[32][16];
uint8_t gMatrix[32][16];
uint8_t bMatrix[32][16];
//Buffers might be used in later code
uint8_t rBuffer[32][16];
uint8_t gBuffer[32][16];
uint8_t bBuffer[32][16];

//BAM and interrupt variables
boolean actDisplay = false;
uint8_t rowN = 0;
uint16_t BAM;
uint8_t BAMMAX = 4; //Max I could get out of it was 5 bit color. not to shaby I think.

// Variables For game of Life
byte analogPin = A5;
byte col = 0;
byte repeat[32][16];
byte world[32][16][2];
long density = 42;
uint8_t SIZEX = 32;
uint8_t SIZEY = 16;
byte  DELAY = 95;

void setup() {
  for(uint8_t i = 0; i < 12; i++){
        pinMode(pinTable[i], OUTPUT);
    }
  timerInit();
  beginLife();
}

void loop() {
   gameOfLife();
  
}
//=================================================LOOP=================================================//


void timerInit() {
    PITimer1.period(0.000042);
    PITimer1.start(timerCallBack);
    BAM = 0;
}


//Where the PIT calls
void timerCallBack() {
    attackMatrix(); // Updated the display

    if(BAM > BAMMAX) { //Checks the BAM cycle for next time.
        BAM = 0;
        PITimer1.period(0.000042);
        actDisplay = false;
    } else {
        BAM ++;
        actDisplay = true;
    }
}

//The updating matrix stuff happens here
//each pair of rows is taken through its BAM cycle
//then the rowNumber is increased and id done again
void attackMatrix() {
    uint16_t portData;

    //sets up which BAM the matrix is on
    if(BAM == 0) { PITimer1.period(.000042); } //code takes max 41 microsec to complete
    if(BAM == 1) { PITimer1.period(.000084); } //so 42 is a safe number
    if(BAM == 2) { PITimer1.period(.000168); }
    if(BAM == 3) { PITimer1.period(.000336); }
    if(BAM == 4) { PITimer1.period(.000672); }
    if(BAM == 5) { PITimer1.period(.001344); }
    if(BAM == 6) { PITimer1.period(.005376); }
    if(BAM == 7) { PITimer1.period(.010752); }

    portData = 0; // Clear data to enter
    portData |= (abcVar[rowN])|OE; // abc, OE
    portData &=~ LATCH;        //LATCH LOW
    GPIOC_PDOR = portData;  // Write to Port
    
    for(uint8_t _x = 0; _x < 32; _x++){
        uint16_t tempC[6] = { //Prepare data in correct place
            ((rMatrix[_x][rowN]))   , ((rMatrix[_x][rowN+8])<<1),
            ((gMatrix[_x][rowN])<<2), ((gMatrix[_x][rowN+8])<<3),
            ((bMatrix[_x][rowN])<<4), ((bMatrix[_x][rowN+8])<<5)                  };
          uint16_t allC =//Put OUTPUT data into temp variable
          ((tempC[0]>>BAM)&RED1)|((tempC[1]>>BAM)&RED2)|
            ((tempC[2]>>BAM)&GREEN1)|((tempC[3]>>BAM)&GREEN2)|
            ((tempC[4]>>BAM)&BLUE1)|((tempC[5]>>BAM)&BLUE2);
          
          GPIOC_PDOR = (portData)|(allC); // Transfer data

          GPIOC_PDOR |=  SCLK;// Clock HIGH
          GPIOC_PDOR &=~ SCLK;// Clock LOW
    }

    GPIOC_PDOR |= LATCH;// Latch HIGH
    GPIOC_PDOR &=~ OE;// OE LOW, Displays line
   

    if(BAM == BAMMAX){
        if(rowN == 7){
            rowN = 0;
        } else {
            rowN ++;
        }
    }
}

void gameOfLife() {
  lifeAndDeath();
  //lifeAndDeathCheck();
  displayCurrentLife();
}


void beginLife() {
  randomSeed(analogRead(analogPin));
  density += random(15);
  for (int i = 0; i < SIZEX; i++) {
    for (int j = 0; j < SIZEY; j++) {
      if (random(100) < density) {
        world[i][j][0] = 1;
      }
      else {
        world[i][j][0] = 0;
      }
      world[i][j][1] = 0;
    }
  }
}

void lifeAndDeath() {
  // Birth and death cycle
  for (int x = 0; x < SIZEX; x++) {
    for (int y = 0; y < SIZEY; y++) {
      // Default is for cell to stay the same
      world[x][y][1] = world[x][y][0];
      int count = neighbours(x, y);
      if (count == 3 && world[x][y][0] == 0) {
        // A new cell is born
        world[x][y][1] = 1;
      }
      if ((count < 2 || count > 3) && world[x][y][0] == 1) {
        // Cell dies
        world[x][y][1] = 0;
      }
    }
  }
  // Copy next generation into place
  for (int x = 0; x < SIZEX; x++) {
    for (int y = 0; y < SIZEY; y++) {
      repeat[x][y] = world[x][y][0];
      world[x][y][0] = world[x][y][1];
    }
  }
}

void displayCurrentLife() {
  // Display current generation
  int _counter = 0;
  int _death   = 0;
  for (int i = 0; i < SIZEX; i++) {
    for (int j = 0; j < SIZEY; j++) {
      if(world[i][j][0]) {
        drawPixel(i,j, randColor(7));
      } 
      else {
        drawPixel(i,j,cBlack);
      } 
      world[i][j][1] = 0;
    }
  }
  delay(DELAY);
  fillMatrix(cBlack);
}

int neighbours(int x, int y) {
  return world[(x + 1) % SIZEX][y][0] +
    world[x][(y + 1) % SIZEY][0] +
    world[(x + SIZEX - 1) % SIZEX][y][0] +
    world[x][(y + SIZEY - 1) % SIZEY][0] +
    world[(x + 1) % SIZEX][(y + 1) % SIZEY][0] +
    world[(x + SIZEX - 1) % SIZEX][(y + 1) % SIZEY][0] +
    world[(x + SIZEX - 1) % SIZEX][(y + SIZEY - 1) % SIZEY][0] +
    world[(x + 1) % SIZEX][(y + SIZEY - 1) % SIZEY][0];
}

uint32_t randColor(uint8_t maxValue) {
  uint8_t var = random(1,maxValue);
  switch(var) {
  case 1: 
    return cRed;
    break;
  case 2: 
    return cBlue;
    break;
  case 3: 
    return cGreen;
    break;
  case 4: 
    return cCyan;
    break;
  case 5: 
    return cMagenta;
    break;
  case 6: 
    return cYellow;
    break;
  case 7: 
    return cWhite;
    break;
  }
}

//used to make entire matrix one color.
void fillMatrix(uint32_t _color) {
  for(int i = 0; i < 16; i++) {
    for(int j = 0; j < 32; j++) {
      rMatrix[j][i] = getRed(_color);
      gMatrix[j][i] = getGreen(_color);
      bMatrix[j][i] = getBlue(_color);
    }
  }
}

// Base graphics function.
void drawPixel(uint8_t x, uint8_t y, uint32_t _color) {
  rMatrix[x][y] = getRed(_color);
  gMatrix[x][y] = getGreen(_color);
  bMatrix[x][y] = getBlue(_color);
}

uint8_t getRed(uint32_t _c) {
  return ((_c>>16)&0xff);
}

uint8_t getGreen(uint32_t _c) {
  return ((_c>>8)&0xff);
}

uint8_t getBlue(uint32_t _c) {
  return ((_c)&0xff);
}
