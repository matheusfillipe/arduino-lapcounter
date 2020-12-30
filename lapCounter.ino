#include <Array.h>
#include <Keypad.h>
#include <Buzzer.h>
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Reactduino.h>
#include <ctype.h> 


// PINS
#define LAPP1 2
#define LAPP2 3
#define P1LED 12
#define P2LED 11
#define BUZZER1 9
#define BUZZER2 8
const int SEMA1[3] = {45, 47, 49};
const int SEMA2[3] = {48, 46, 44};


// Constants
#define MAX_LAPS 200
#define DEFAULT_LAPS "30"
#define DEBUG true
#define LAPS 3
#define BLINK_TIME 100
#define N_PINS 24
#define DEFAULT_BLINK_DURATION 2000
#define KEYBOARD_DELAY 20
#define TEST_DELAY 500

#define FONT_BIG u8g2_font_logisoso18_tf
#define FONT_MATRIX u8g2_font_trixel_square_tn
#define FONT_SMALL  u8g2_font_7x13B_tf
#define FONT_RATIO 3


#define REACT(func) [](){func;}
#define DEB(varname) debug(String(__FILE__).substring(String(__FILE__).lastIndexOf("/")+1, String(__FILE__).length())+":"+String(__LINE__)+" --> "#varname" = " + String(varname))
#define DELAY_HIGH(n, pin) app.delay(n,[](){digitalWrite(pin, HIGH);})
#define DELAY_LOW(n, pin) app.delay(n,[](){digitalWrite(pin, LOW);})
#define REPEAT_BLINK(n, pin) app.repeat(n, [] () {static bool state = false; digitalWrite(pin, state = !state);})
#define DELAY_AWRITE(n, pin, value) app.delay(n,[](){analogWrite(pin, value);})


// Keypad
const size_t ELEMENT_COUNT = 5;
const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'#','0','*','D'}
};

byte rowPins[rows] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[cols] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


// Classes
void debug(String msg) {
  if (DEBUG) {
    Serial.println(String("DEBUG: ") + msg);
  }
}

void log(String msg){
  Serial.println("--> " + msg);
}

typedef struct Lap_struct {
  int player;
  int num;
  float time;
} Lap;


class Cursor{
  private:
    void setup(int topMargin, int fontHeight, int lineSpacing, int fontWidth){
      this->x = 0;
      this->y = topMargin+fontHeight;
      this->lineSpacing = lineSpacing;
      this->fontWidth = fontWidth;
    }

  public:
    int x, y, width, height, fontHeight, topMargin, lineSpacing, fontWidth;
    Cursor(int width, int height, int fontHeight): width(width), height(height), fontHeight(fontHeight){
      this->setup(0, fontHeight,0, fontHeight/FONT_RATIO);
    }
    Cursor(int width, int height, int fontHeight, int topMargin, int lineSpacing, int fontWidth): width(width), height(height), fontHeight(fontHeight), fontWidth(fontWidth), topMargin(0), lineSpacing(0){
      this->setup(topMargin,fontHeight,lineSpacing, fontWidth);
    }
    Cursor(int width, int height, int fontHeight, int topMargin, int lineSpacing): width(width), height(height), fontHeight(fontHeight), fontWidth(fontHeight/FONT_RATIO), topMargin(0), lineSpacing(0){
      this->setup(topMargin,fontHeight,lineSpacing, fontHeight/FONT_RATIO);
    }

    void inc(int steps){
      this->x+=steps;
      if (this->x > this->width){
        this->x=0;
        this->linebreak();
      }
    }

    void step(int steps){
      this->x+=this->fontWidth*steps;
      if (this->x > this->width){
        this->x=0;
        this->linebreak();
      }
    }
    void linebreak(){
      this->y += this->fontHeight+this->lineSpacing;
    }

    bool isFull(){
      return this->y > this->height;
    }
};

typedef struct Output_struct {
  String msg;
  U8G2 *tela;
  Cursor &cursor;
  const uint8_t *font;
} Output;

// Variables
Lap laps[MAX_LAPS];
char lastKey;
char inputBuffer[4];
bool gameloop = false;
Cursor tela1_cursor(128, 64, 18, 22, 5);
Cursor tela2_cursor(128, 64, 18, 22, 5);
Cursor matrix_cursor1(8, 8, 8);
Cursor matrix_cursor2(8, 8, 8);


// Screens
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela1(U8G2_R0, /* clock=*/ 18, /* data=*/ 17);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela2(U8G2_R0, /* clock=*/ 14, /* data=*/ 16);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix2(U8G2_R0, /* clock=*/ 43, /* data=*/ 39, /* cs=*/ 41, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix1(U8G2_R0, /* clock=*/ 52, /* data=*/ 51, /* cs=*/ 50, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);



// Output
char *str2char(String str){
  int str_len = str.length() + 1; 
  char char_array[str_len];
  str.toCharArray(char_array, str_len);
  return char_array;
}
void write(String msg, U8G2 *tela, Cursor &cursor = tela1_cursor, const uint8_t *font = FONT_SMALL){
  int str_len = msg.length() + 1; 
  u8g2_uint_t char_width = tela->getMaxCharWidth();
  tela->setFont(font);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
  if(tela->getStrWidth(str2char(msg)) > tela->getDisplayWidth()){
    int max = tela->getDisplayWidth()/tela->getMaxCharWidth();
    String buf=msg.substring(0, max);
    while(tela->getStrWidth(str2char(buf)) < tela->getDisplayWidth()){
      buf=msg.substring(0,max);
      max++;
    }
    write(msg.substring(0,max), tela, cursor, font);
    cursor.x=0;
    cursor.linebreak();
    write(msg.substring(max), tela, cursor, font);
    return;  
  }
  char char_array[str_len];
  msg.toCharArray(char_array, str_len);
  tela->clearBuffer();
  tela->drawStr(cursor.x, cursor.y, char_array);	// write something to the internal memory
  tela->sendBuffer();					// transfer internal memory to the display
  // cursor.inc(char_width*str_len);
}

void print(String msg, U8G2 *tela, Cursor &cursor = tela1_cursor, const uint8_t *font = FONT_SMALL){
  tela->clearBuffer();
  write(msg, tela, cursor, font);
}


// react_callback flipStateFunc() {
  // return []() {
    // static bool state = false;
    // digitalWrite(pin, state = !state);
  // };
// }

//void blink_animation(uint32_t time, int duration = 0) {
//    int delay_time = duration / time % 2 == 0 ? duration : duration + time;
//    bool animating = true;
//    OutRegister.react = this->blink_reaction;
//    OutRegister.x = delay_time / time;
//
//    blink_reaction = app.repeat(time, []() {
//      static int end = OutRegister.x;
//      static int count = 0;
//      static reaction react = OutRegister.react;
//      static bool state = false;
//      static int pin = OutRegister.pin;
//      if (count >= end) {
//        debug("Ending blink_animation");
//        app.free(react);
//        return;
//      }
//      count++;
//      digitalWrite(pin, state = !state);
//    });
//  }
//};

// Keyboard Handling
char receiveKey(){
  char key = keypad.getKey();
  if(lastKey == key or key == ' ')
    return '-';
  lastKey = key;
  return key;
}

String input(int size = 2){
  int count = 0;
  char buffer[8];
  do{
    char c = receiveKey();
    if (c!="-"){
      buffer[count] = c;
      count++;
    }
  }while(count < size);
  return String(buffer);
}

// Events
void semOff(int sema[3]){
  for(int i=0; i<3; i++){
    digitalWrite(sema[i], LOW);
  }
}

void ASemOff(int sema[3]){
  for(int i=0; i<2; i++){
    analogWrite(sema[i], 0);
  }
}

void inc_matrix1(){
      static int count = 0;
      if(count >= 100)
        count = 0;
      matrix_cursor1.x=0;
      matrix_cursor1.y=8;
      print(String(count), &matrix1, matrix_cursor1, FONT_MATRIX);
      count++;
}

void inc_matrix2(){
      static int count = 0;
      if(count >= 200)
        count = 0;
      matrix_cursor2.x=0;
      matrix_cursor2.y=8;
      print(String(count), &matrix2, matrix_cursor2, FONT_MATRIX);
      count++;
}

void race(){
}


reaction Rlargada[11], qdr1, qdr2;
void quemada(){
  for(int i = 0; i<11; i++){
    app.free(Rlargada[i]);
  }
}

void mirrowedPrint(int n){
  tela1_cursor.x=30;
  tela1_cursor.y=30;
  print(String(n), &tela1, tela1_cursor, FONT_BIG);
  tela2_cursor.x=30;
  tela2_cursor.y=30;
  print(String(n), &tela2, tela2_cursor, FONT_BIG);
}

void largada(){
  const int delays[4] = {100, 1500, 3000, 4500};
  qdr1 = app.onPinFalling(LAPP1, [](){
    tela1_cursor.y=10;
    tela1_cursor.x=40;
    print("Queimou!", &tela1, tela1_cursor, FONT_BIG);
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    DELAY_AWRITE(0, SEMA1[0], 255);
    quemada();
    app.free(qdr1);
  });
  qdr2 = app.onPinFalling(LAPP2, [](){
    tela2_cursor.y=40;
    tela2_cursor.x=10;
    print("Queimou!", &tela2, tela2_cursor, FONT_BIG);
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    DELAY_AWRITE(0, SEMA2[0], 255);
    quemada();
    app.free(qdr2);
  }); 
  Rlargada[0] = DELAY_AWRITE(delays[0],  SEMA1[0], 255);
  Rlargada[1] = DELAY_AWRITE(delays[0],  SEMA2[0], 255);
  Rlargada[2] = app.delay(delays[0], [](){tone(8, NOTE_E4, 400);});
  Rlargada[3] = app.delay(delays[0], REACT(mirrowedPrint("3")));
  app.delay(delays[0],()[]{
      analogWrite(SEMA1[0], 255);
      analogWrite(SEMA2[0], 255);
      tone(8, NOTE_E4, 400);
      mirrowedPrint("3");
  });

  Rlargada[4] = DELAY_AWRITE(delays[1], SEMA1[1], 255);
  Rlargada[5] = DELAY_AWRITE(delays[1], SEMA2[1], 255);
  Rlargada[6] = app.delay(delays[1], [](){tone(8, NOTE_E4, 400);});
  Rlargada[8] = app.delay(delays[1], REACT(mirrowedPrint("2")));
  app.delay(delays[1],()[]{
      analogWrite(SEMA1[1], 255);
      analogWrite(SEMA2[1], 255);
      tone(8, NOTE_E4, 400);
      mirrowedPrint("2");
  });

  Rlargada[9] = DELAY_AWRITE(delays[2], SEMA1[2], 255);
  Rlargada[10] = DELAY_AWRITE(delays[2], SEMA2[2], 255);
  Rlargada[11] = app.delay(delays[2], [](){tone(8, NOTE_E4, 400);});
  Rlargada[12] = app.delay(delays[2], REACT(mirrowedPrint("1")));
  app.delay(delays[2],()[]{
      analogWrite(SEMA1[2], 255);
      analogWrite(SEMA2[2], 255);
      tone(8, NOTE_E4, 400);
      mirrowedPrint("1");
  });

  Rlargada[13] = app.delay(delays[3], [](){ASemOff(SEMA1);});
  Rlargada[14] = app.delay(delays[3], [](){ASemOff(SEMA2);});
  Rlargada[15] = app.delay(delays[3], [](){tone(8, NOTE_E5, 1000); race();});
  app.delay(delays[3],()[]{
      ASemOff(SEMA1);
      ASemOff(SEMA2);
      tone(8, NOTE_E5, 1000);
      mirrowedPrint("Go!");
      race();
  });
}

// Game
reaction Rmenu;
int race_n_laps;
void game(){
  print("Voltas", &tela1, tela1_cursor, FONT_BIG);
  print(DEFAULT_LAPS, &tela2, tela2_cursor, FONT_BIG);
  Rmenu = app.repeat(KEYBOARD_DELAY, REACT(menu_input()));
}

void menu_input(){
  static String last = "";
  char key = receiveKey(); 
  if (key=='D') {
    race_n_laps = last.toInt();
    print("", &tela1, tela1_cursor, FONT_BIG);
    print("", &tela2, tela2_cursor, FONT_BIG);
    largada();
    app.free(Rmenu);
  }
  if(key != '-' && key != ' ' && key && isdigit(key)){
    if (last.length()>2)
      last = "";
    last += String(key);
    tela2_cursor.x=0;
    tela2_cursor.y=40;
    print(last, &tela2, tela2_cursor, FONT_BIG);
  }
}

reaction m1r, m2r, t2r;
void test_process_input(){
  char key = receiveKey(); 
  static String last = "";
  if(key != '-' && key != ' ' && key){
    app.free(t2r);
    last += String(key);
    tela2_cursor.x=0;
    tela2_cursor.y=30;
    print(last, &tela2, tela2_cursor, FONT_SMALL);
  }
}

void testMode(){
  m1r=app.repeat(1000, REACT(inc_matrix1()));
  m2r=app.repeat(800, REACT(inc_matrix2()));
  app.repeat(KEYBOARD_DELAY, REACT(test_process_input()));
  app.repeat(500, [](){
      static int count = 0;
      tela1_cursor.x=0;
      print("test1 "+String(count), &tela1, tela1_cursor, FONT_BIG);
      count++;
  });
  t2r=app.repeat(500, [](){
      static int count = 0;
      tela2_cursor.x=0;
      print("test2 "+String(count), &tela2, tela2_cursor, FONT_BIG);
      count++;
  });
  app.onPinFalling(LAPP1, [](){
      app.free(m1r);
      inc_matrix1();
  });
  app.onPinFalling(LAPP2, [](){
      app.free(m2r);
      inc_matrix2();
  });
}

reaction br, tr;
void boot(){
  br = app.delay(TEST_DELAY, [](){
    //start game
    log("STARTED!!");
    game();
    app.free(tr);
  });

  //trigger test mode
  tr = app.repeat(KEYBOARD_DELAY, [](){
    static bool testing = false;
    if (testing){
      app.free(tr);
      return;
    }
    if (receiveKey()=='D') {
      log("TEST!!");
      testing = true;
      testMode();
      app.free(br);
    }
  });
}

// Main
// Uncomment #define U8X8_USE_ARDUINO_AVR_SW_I2C_OPTIMIZATION in U8x8lib.h.
void app_main() {
  Serial.begin(9600);
  matrix1.begin();
  matrix2.begin();
  tela1.begin();
  tela2.begin();
  boot();

}

Reactduino app(app_main);
