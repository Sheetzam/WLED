#include "wled.h"

// for information how FX metadata strings work see https://kno.wled.ge/interfaces/json-api/#effect-metadata

// static effect, used if an effect fails to initialize
static uint16_t mode_static(void) {
  SEGMENT.fill(SEGCOLOR(0));
  return strip.isOffRefreshRequired() ? FRAMETIME : 350;
}

const uint8_t noDirection = 0;
const uint8_t up = 1;
const uint8_t upRight = 2;
const uint8_t right = 3;
const uint8_t downRight = 4;
const uint8_t down = 5;
const uint8_t downLeft = 6;
const uint8_t left = 7;
const uint8_t upLeft = 8;
const uint8_t numSprites = 8; //max number of sprites is 8?
uint8_t thisSprite=0;
struct aColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
struct aVector {
  uint8_t xLocation;
  uint8_t yLocation;
  uint8_t theDirection;
};
struct aSprite {
  struct aColor theColor;
  struct aVector theVector;
};
uint8_t goal_r;
uint8_t goal_g;
uint8_t goal_b;
uint8_t thisR;
uint8_t thisG;
uint8_t thisB;
uint8_t theStep; //we're going to deliberately overflow this 8 bit integer
char working_background;
struct aColor theBackground;


/////////////////////////
//  User FX functions  //
/////////////////////////

static uint16_t mode_sprites(void) {
  if (!strip.isMatrix || !SEGMENT.is2D())
    return mode_static();  // not a 2D set-up

  const int cols = SEG_W;
  const int rows = SEG_H;
  const auto XY = [&](int x, int y) { return x + y * cols; };
  uint8_t i;
  uint8_t x;
  uint8_t y;

  // we need to allocate memory for:
  // theStep - an 8 bit integer
  // a single sprite structure times the number of sprites
  // theField, which is an array of bytes sized width * height
  // working_background, a char
  // theBackground, a single aColor structure
  unsigned dataSize = sizeof(theStep) + sizeof(aSprite) * numSprites + cols * rows + sizeof(working_background) + sizeof(aColor);
  if (!SEGENV.allocateData(dataSize))
    return mode_static();  // allocation failed
  uint8_t* theStep = reinterpret_cast<uint8_t*>(SEGENV.data);
  aSprite* ourSprites = reinterpret_cast<aSprite*>(SEGENV.data + sizeof(uint8_t));
  auto theField = reinterpret_cast<uint8_t*>(SEGENV.data + sizeof(uint8_t) + sizeof(aSprite) * numSprites);
  auto working_background = reinterpret_cast<char*>(SEGENV.data + sizeof(uint8_t) + sizeof(aSprite) * numSprites + cols * rows);
  auto theBackground = reinterpret_cast<aColor*>(SEGENV.data + sizeof(uint8_t) + sizeof(aSprite) * numSprites + cols * rows + sizeof(working_background));
  // here we set up things on our first call
  if (SEGENV.call == 0) {
    (*theBackground).r = 0;
    (*theBackground).g = 0;
    (*theBackground).b = 30;
    (*working_background) = 'b';
    for( y = 0; y < rows; y++) {
      for( x = 0; x < cols; x++) {
        SEGMENT.setPixelColorXY(x,y,(*theBackground).r, (*theBackground).g, (*theBackground).b);
      }
    }

    SEGENV.step = 0;
    for(thisSprite=0; thisSprite < numSprites; thisSprite++){
      ourSprites[thisSprite].theVector.theDirection=noDirection;
    }
  }

  SEGENV.step = strip.now;
  if (0 == *theStep){
  //loop through our sprites
    for(thisSprite=0; thisSprite < numSprites; thisSprite++){
      //see if it has no direction
      if (ourSprites[thisSprite].theVector.theDirection == noDirection) {
        //decide if we're going to add it to the mix
        if (hw_random8(0,10) > 8){
          //set the sprite's color
          ourSprites[thisSprite].theColor.r=hw_random8(0,255);
          ourSprites[thisSprite].theColor.g=hw_random8(0,255);
          ourSprites[thisSprite].theColor.b=hw_random8(0,255);
          ourSprites[thisSprite].theVector.theDirection=hw_random8(1,9);
          switch (ourSprites[thisSprite].theVector.theDirection) {
            case up:
              ourSprites[thisSprite].theVector.xLocation=hw_random8(0,cols);
              ourSprites[thisSprite].theVector.yLocation=0;
              break;
            case upRight:
              if (hw_random8(0,2)){
                ourSprites[thisSprite].theVector.xLocation=hw_random8(0,cols);
                ourSprites[thisSprite].theVector.yLocation=0;
              }
              else {
                ourSprites[thisSprite].theVector.xLocation=0;
                ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              }
              break;
            case right:
              ourSprites[thisSprite].theVector.xLocation=0;
              ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              break;
            case downRight:
              if (hw_random8(0,2)){
                ourSprites[thisSprite].theVector.xLocation=hw_random8(0,cols);
                ourSprites[thisSprite].theVector.yLocation=rows-1;
              }
              else {
                ourSprites[thisSprite].theVector.xLocation=0;
                ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              }
              break;
            case down:
              ourSprites[thisSprite].theVector.xLocation=0;
              ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              break;
            case downLeft:
              if (hw_random8(0,2)){
                ourSprites[thisSprite].theVector.xLocation=hw_random8(0,cols);
                ourSprites[thisSprite].theVector.yLocation=rows-1;
              }
              else {
                ourSprites[thisSprite].theVector.xLocation=cols-1;
                ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              }
              break;
            case left:
              ourSprites[thisSprite].theVector.xLocation=cols-1;
              ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              break;
            case upLeft:
              if (hw_random8(0,2)){
                ourSprites[thisSprite].theVector.xLocation=hw_random8(0,cols);
                ourSprites[thisSprite].theVector.yLocation=0;
              }
              else {
                ourSprites[thisSprite].theVector.xLocation=cols-1;
                ourSprites[thisSprite].theVector.yLocation=hw_random8(0,rows);
              }
              break;
          }
        }
      }
      else {
        //Set theField's bit for this sprite so we know we need to fade it in
        int xPos = ourSprites[thisSprite].theVector.xLocation;
        int yPos = ourSprites[thisSprite].theVector.yLocation;
        theField[XY(xPos,yPos)] |= (1 << thisSprite);
        //calculate our new location
        switch (ourSprites[thisSprite].theVector.theDirection) {
          case up:
            ourSprites[thisSprite].theVector.yLocation += 1;
            break;
          case upRight:
            ourSprites[thisSprite].theVector.xLocation += 1;
            ourSprites[thisSprite].theVector.yLocation += 1;
            break;
          case right:
            ourSprites[thisSprite].theVector.xLocation += 1;
            break;
          case downRight:
            ourSprites[thisSprite].theVector.xLocation += 1;
            ourSprites[thisSprite].theVector.yLocation -= 1;
            break;
          case down:
            ourSprites[thisSprite].theVector.yLocation -= 1;
            break;
          case downLeft:
            ourSprites[thisSprite].theVector.xLocation -= 1;
            ourSprites[thisSprite].theVector.yLocation -= 1;
            break;
          case left:
            ourSprites[thisSprite].theVector.xLocation -= 1;
            break;
          case upLeft:
            ourSprites[thisSprite].theVector.xLocation -= 1;
            ourSprites[thisSprite].theVector.yLocation += 1;
            break;
        }
        if ((ourSprites[thisSprite].theVector.xLocation >= cols) ||
            (ourSprites[thisSprite].theVector.xLocation == 255) ||
            (ourSprites[thisSprite].theVector.yLocation >= rows) ||
            (ourSprites[thisSprite].theVector.yLocation == 255)
          ) {
          //setting direction to 0 means it's inactive and off the screen
          ourSprites[thisSprite].theVector.theDirection = 0;
        }
      }
    }
  }

  //loop through our matrix
  for( y = 0; y < rows; y++) {
    for( x = 0; x < cols; x++) {
      goal_r = (*theBackground).r;
      goal_g = (*theBackground).g;
      goal_b = (*theBackground).b;
      CRGB thisPixel = SEGMENT.getPixelColorXY(x,y);
      thisR = thisPixel.r;
      thisG = thisPixel.g;
      thisB = thisPixel.b;

      if ((0 == theField[XY(x,y)]) && (0 == *theStep % 5)){
        //fade our current color to the background
        if (thisR > (*theBackground).r){
          thisR--;
        }
        else {
          thisR = (*theBackground).r;
        }
        if (thisG > (*theBackground).g){
          thisG--;
        }
        else {
          thisG = (*theBackground).g;
        }
        if (thisB > (*theBackground).b){
          thisB--;
        }
        else {
          thisB = (*theBackground).b;
        }
      }
      else if (0 != theField[XY(x, y)]) {
        //accumulate our intensity with bitwise OR
        for ( i = 0; i < numSprites; i++) {
          if ((theField[XY(x,y)] & (1 << i)) != 0 ){
            goal_r = goal_r | ourSprites[i].theColor.r;
            goal_g = goal_g | ourSprites[i].theColor.g;
            goal_b = goal_b | ourSprites[i].theColor.b;
          }
        }
        if (goal_r > thisR){
          thisR += 1;
        }
        if (goal_g > thisG){
          thisG +=1;
        }
        if (goal_b > thisB){
          thisB += 1;
        }
      }
      SEGMENT.setPixelColorXY(x, y, thisR, thisG, thisB);
      //when we're done, set that part of theField to 0, ready to be used again
      if (255 == *theStep) {
        theField[XY(x,y)] = 0;
      }
    }
  }
  //now change our background color
  if (0 == *theStep){
    switch ((int)*working_background){
      case 'b':
        if ((*theBackground).b >= 30){
          *working_background = 'r';
        }
        else {
          (*theBackground).b++;
          (*theBackground).g--;
        }
        break;
      case 'r':
        if ((*theBackground).r >= 30){
          *working_background = 'g';
        }
        else {
          (*theBackground).r++;
          (*theBackground).b--;
        }
        break;
      default:
        if ((*theBackground).g >= 30){
          *working_background = 'b';
        }
        else {
          (*theBackground).g++;
          (*theBackground).r--;
        }
    }
  }
  (*theStep)++;
  return FRAMETIME;
}

static const char _data_FX_MODE_SPRITES[] PROGMEM = "Sprites@!,,,,,;;;2;";


/////////////////////
//  UserMod Class  //
/////////////////////

class spritesMod : public Usermod {
 private:
 public:
  void setup() override {
    strip.addEffect(255, &mode_sprites, _data_FX_MODE_SPRITES);
  }
  void loop() override {} // nothing to do in the loop
  uint16_t getId() override { return USERMOD_ID_USER_FX; }
};

static spritesMod sprites_fx;
REGISTER_USERMOD(sprites_fx);