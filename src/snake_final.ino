
#define Y_INPUT A2 // Y joytick axis
#define X_INPUT A1 // X joytick axis
#define PIXELS 8*8 // Number of pixels in the string
#define BUTTON 8 // joytick button

#define PIXEL_PORT  PORTB  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRB   // Port of the pin the pixels are connected to
#define PIXEL_BIT   4      // Bit of the pin the pixels are connected to

#define T1H  900    // Width of a 1 bit in ns
#define T1L  600    // Width of a 1 bit in ns

#define T0H  400    // Width of a 0 bit in ns
#define T0L  900    // Width of a 0 bit in ns



#define RES 250000    // Width of the low gap between bits to cause a frame to latch



#define NS_PER_SEC (1000000000L)          // this has to be SIGNED since we want to be able to check for negative values of derivatives

#define CYCLES_PER_SEC (F_CPU)

#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

#define S_UP 0          //
#define S_RIGHT  1      // Snake directions
#define S_DOWN  2       //
#define S_LEFT  3       //


inline void sendBit( bool bitVal ) {

  if (  bitVal ) {        // 1 bit

    asm volatile (
      "sbi %[port], %[bit] \n\t"                              // Set the output bit
      ".rept %[onCycles] \n\t"                                // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      "cbi %[port], %[bit] \n\t"                              // Clear the output bit
      ".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      ::
      [port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
      [bit]   "I" (PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T1H) - 2),    // 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
      [offCycles]   "I" (NS_TO_CYCLES(T1L) - 2)     // Minimum interbit delay. Note that we probably don't need this at all since the loop overhead will be enough, but here for correctness

    );

  } else {          // 0 bit


    asm volatile (
      "sbi %[port], %[bit] \n\t"        // Set the output bit
      ".rept %[onCycles] \n\t"        // Now timing actually matters. The 0-bit must be long enough to be detected but not too long or it will be a 1-bit
      "nop \n\t"                                              // Execute NOPs to delay exactly the specified number of cycles
      ".endr \n\t"
      "cbi %[port], %[bit] \n\t"                              // Clear the output bit
      ".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
      "nop \n\t"
      ".endr \n\t"
      ::
      [port]    "I" (_SFR_IO_ADDR(PIXEL_PORT)),
      [bit]   "I" (PIXEL_BIT),
      [onCycles]  "I" (NS_TO_CYCLES(T0H) - 2),
      [offCycles] "I" (NS_TO_CYCLES(T0L) - 2)

    );

  }
}


inline void sendByte( unsigned char byte ) {

  for ( unsigned char bit = 0 ; bit < 8 ; bit++ ) {

    sendBit( bitRead( byte , 7 ) );                // Neopixel wants bit in highest-to-lowest order
    // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
    byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc

  }
}

void ledsetup() {

  bitSet( PIXEL_DDR , PIXEL_BIT );  //Set bit for output

}

inline void sendPixel( unsigned char r, unsigned char g , unsigned char b )  {
  sendByte(g);          // Neopixel wants colors in green then red then blue order
  sendByte(r);
  sendByte(b);

}


void show() {
  _delay_us( (RES / 1000UL) + 1);       // Delay for frame to latch
}



void showColor( unsigned char r , unsigned char g , unsigned char b ) {  // Fill matrix with specified color
  cli();
  for ( int p = 0; p < PIXELS; p++ ) {
    sendPixel( r , g , b );
  }
  sei();
  show();
}



char field[PIXELS];        //Array thats represent game field
char testPoint;            //Point for testing horizontal movement
char snakeDirection;
char snake[18];            //Array thats represent snake and contain filed positions
char snakeLength;
char fruitPos;             //Fruit positions
int snakeSpeed;            // Speed can be 1000 800  600 400
int moveLock;              // Lock for snake, after changing directon, directon can't be cahnged again

//char debInput;
//
//void Debug() {
//  Serial.println("Snake array");
//  for (int i = 0; i < 18; i++) {
//    Serial.print("[");
//    Serial.print(snake[i], DEC);
//    Serial.print("] ");
//  }
//  Serial.println();
//}

char genFruit(char r) {                     //Fruit generation
  for (int i = 0; i < snakeLength; i++) {
    if (r == snake[i] || r == -1) {
      r = genFruit(random(64));
      break;
    }
  }
  return r;
}

void drawButton() {                         //Draw blue button
  boolean buttonCode[64] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 0, 1, 1, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
  };
  cli();
  for (int i = 0; i < PIXELS; i++) {
    if (buttonCode[i]) sendPixel(0, 0, 10);
    else sendPixel(0, 0, 0);
  }
  sei();
  show();
  bool flag = false;
  uint32_t btnTimer = millis();
  while (1) {
    if (!digitalRead(BUTTON)) {
      flag = true;
    }
    else {
      flag = false;
      btnTimer = millis();
    }
    if (flag && (millis() - btnTimer > 75)) {
      btnTimer = millis();
      break;
    }
  }
}

void showResult() {             //Show result
  int positions[18] = {48, 49, 50, 51, 52, 53, 54, 55, 40, 39, 38, 37, 36, 35, 28, 20, 19, 11};  // Positions that will be filled

  char pict[64] = {0};
  char count = 0;
  int i = 0;
  for (; i < 18 - snakeLength; i++) {                 //Fill with red
    pict[positions[i]] = 2;
  }
  for (; i < 17; i++) {                               //Fill with green
    pict[positions[i]] = 1;
  }
  pict[positions[i]] = 3;
  cli();
  for (int i = 0; i < PIXELS; i++) {
    switch (pict[i]) {
      case 3:
        sendPixel(10, 0, 10);
        break;
      case 2:
        sendPixel(10, 0, 0);
        break;
      case 1:
        sendPixel(0, 10, 0);
        break;
      case 0:
        sendPixel(0, 0, 0);
        break;
    }
  }
  sei();
  show();
  bool flag = false;
  uint32_t btnTimer = millis();
  while (1) {
    if (!digitalRead(BUTTON)) {
      flag = true;
    }
    else {
      flag = false;
      btnTimer = millis();
    }
    if (flag && (millis() - btnTimer > 75)) {
      btnTimer = millis();
      break;
    }
  }
}

int currentDifficult = 5;        //Starting difficult, can be 1, 3, 5, 7
void changeDifficluty() {
  int difficultCode[64] = {           // Difficluties cpicture
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 3, 3, 3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
  };


  bool flag = false;
  uint32_t btnTimer = millis();

  while (1) {                                           //Count joystick movement and button press
    difficultCode[(8 - currentDifficult) * 8] = 5;
    cli();
    for (int i = 0; i < PIXELS; i++) {
      switch (difficultCode[i]) {
        case 5:
          sendPixel(10, 0, 10);
          break;
        case 4:
          sendPixel(10, 0, 0);
          break;
        case 3:
          sendPixel(15, 5, 0);
          break;
        case 2:
          sendPixel(10, 10, 0);
          break;
        case 1:
          sendPixel(0, 10, 0);
          break;
        case 0:
          sendPixel(0, 0, 0);
          break;
      }
    }
    sei();
    show();
    int y_val = map(analogRead(Y_INPUT), 0, 1023, 0 , 8) - 4;       //High up difficult
    if (y_val < -1 && currentDifficult > 1) {
      difficultCode[(8 - currentDifficult) * 8] = 0;
      currentDifficult -= 2;
      delay(250);
    }
    else if (y_val > 1 && currentDifficult < 7) {                  //low down difficult 
      difficultCode[(8 - currentDifficult) * 8] = 0;
      currentDifficult += 2;
      delay(250);
    }
    if (!digitalRead(BUTTON)) {                           //Button preess analyse
      flag = true;
    }
    else {
      flag = false;
      btnTimer = millis();
    }
    if (flag && (millis() - btnTimer > 75)) {
      btnTimer = millis();
      break;
    }
  }
  snakeSpeed = ((8 - currentDifficult) + 1) * 100;
}

void startGame() {                    // Game start
  for (int i = 0; i < PIXELS; i++) {      //Zero out field
    field[i] = 0;
  }
  changeDifficluty();                     //Ask for difficult
  snake[0] = 20;                          //Starting snake positions
  snake[1] = 11;
  snake[2] = 4; 
  snakeDirection = S_UP;                  //Starting snake direction
  snakeLength = 3;                        //Starting snake length
  fruitPos = genFruit(-1);                //Generate fruit
}

void setup() {                      
  ledsetup();                             //Setup pin for matrix
  randomSeed(analogRead(5));              //Seed random with nois from pin 5
  pinMode(BUTTON, INPUT_PULLUP);          //Setup joystick button
  drawButton();                           //Draw blue button
  startGame();                         
}

void endOfGame() {                      //If lose, then red screen and score display
  showColor(1, 0, 0);
  delay(200);
  showResult();
  startGame();
}

void winGame() {                        //If win, then green screen and score display
  showColor(0, 1, 0);
  delay(500);
  showResult();
  startGame();
}

void processController() {             //Process data from joystick
  if ((snakeDirection == S_RIGHT || snakeDirection == S_LEFT) && (!moveLock)) {
    int y_val = map(analogRead(Y_INPUT), 0, 1023, 0 , 8) - 4;
    if (y_val < -1) {
      snakeDirection = S_UP;
      moveLock = 1;
    }
    else if (y_val > 1) {
      snakeDirection = S_DOWN;
      moveLock = 1;
    }
  }
  else if ((snakeDirection == S_DOWN || snakeDirection == S_UP) && (!moveLock)) {
    int x_val = map(analogRead(X_INPUT), 0, 1023, 0 , 8) - 4;
    if (x_val > 1) {
      snakeDirection = S_RIGHT;
      moveLock = 1;
    }
    else if (x_val < -1) {
      snakeDirection = S_LEFT;
      moveLock = 1;
    }
  }
}

char moveSnake() {        //Change values in snake array
  char testVal = 0;

  for (int i = snakeLength; i > 0; i--) {                     //
    snake[i] = snake[i - 1];                                  // Shift snake elements from head to tail
  }

  testVal = ((snake[1] % 16) - 8);                            // Test value for horizontal move

  if (snakeDirection == S_RIGHT) {                            //
    if (testVal == 0 || testVal == -1) {                      //
      Serial.println("RIGHT CRUSH");
      return 1;                                               //
    }                                                         //
    else if (testVal > 0 ) {                                  //  Process movement to right
      snake[0] = snake[1] - 1;                                //
    }                                                         //
    else {                                                    //
      snake[0] = snake[1] + 1;                                //
    }                                                         //
  }                                                           //

  if (snakeDirection == S_LEFT) {                             //
    testVal = ((snake[1] % 16) - 8);                          //
    //
    if (testVal == 7 || testVal == -8) {
      return 1;              //
    }
    else if (testVal >= 0) {                                  // Process movement to left
      snake[0] = snake[1] + 1;                                //
    }                                                         //
    else {                                                    //
      snake[0] = snake[1] - 1;                                //
    }                                                         //
  }

  if (snakeDirection == S_UP) {                               //
    if ((snake[1] > 55) && (snake[1] < 64)) {
      return 1;                                               //  Process movement to Up
    }
    snake[0] += ((8 - (snake[1] % 8)) * 2) - 1;               //
  }                                                           //

  if (snakeDirection == S_DOWN) {                             //
    if ((snake[1] >= 0) && (snake[1] < 8)) {
      return 1;                                               // Process movement to Down
    }
    snake[0] += (((snake[1] % 8) * -2) - 1);                  //
  }                                                           //

  for (int i = 1; i < snakeLength; i++) {                     //
    if (snake[0] == snake[i]) {
      return 1;                                               // Snake collision check
    }
  }                                                           //

  if (snake[0] == fruitPos) {                                 //Fruit check
    snakeLength++;
    if (snakeLength >= 18) {                              
      winGame();
      return 2;
    }
    fruitPos = genFruit(random(64));                          //generate fruit
  }
  moveLock = 0;
  return 0;
}

char processSnake() {                       //Fill "field" wit snake values
  if (moveSnake() == 1) return 1;           //If somthing went wrong with snake movement, tell about it
  field[snake[snakeLength]] = 0;
  field[snake[0]] = 2;                        
  for (int i = 1; i < snakeLength; i++) {
    field[snake[i]] = 1;
  }
  return 0;
}

char drawField() {                              //Draw filed with snake and fuit on matrix
  if (processSnake() != 0) return 1;            // If somthing went wrong with snake, tell about it
  field[fruitPos] = 3;
  cli();
  for (int i = 0; i < PIXELS; i++) {
    switch (field[i]) {
      case 0:
        sendPixel(0, 0, 0);
        break;
      case 1:
        sendPixel(0, 1, 0);
        break;
      case 2:
        sendPixel(1, 0, 1);
        break;
      case 3:
        sendPixel(1, 1, 0);
    }
  }
  show();
  sei();
  return 0;
}

unsigned long delayVal = 0;
unsigned long timeVal;


void loop() {

  processController();                  //Change snake direction according to joystick movement
  timeVal = millis();
  if ((timeVal - delayVal) > snakeSpeed) {            //Draw field once per difficult time 
    if (drawField() != 0) {                           //If somthing went wrong with snake, game over
      endOfGame();
    }
    delayVal = timeVal;
  }
}
