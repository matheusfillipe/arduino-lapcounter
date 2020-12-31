#include <Array.h>
#include <Keypad.h>
#include <Buzzer.h>
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Reactduino.h>
#include <ctype.h> 
#include "definitions.h"
#include "display.h"
#include "react.h"

// Classes

typedef struct Lap_struct {
  int player;
  int num;
  unsigned long time;
  unsigned long lap_time;
} Lap;


// Variables
Lap p1Laps[MAX_LAPS];
Lap p2Laps[MAX_LAPS];
char inputBuffer[4];
bool gameloop = false;
Cursor tela1_cursor(128, 64, 18, 22, 5);
Cursor tela2_cursor(128, 64, 18, 22, 5);
Cursor matrix1_cursor(8, 8, 8);
Cursor matrix2_cursor(8, 8, 8);


// Screens
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela1(U8G2_R0, /* clock=*/ 18, /* data=*/ 17);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela2(U8G2_R0, /* clock=*/ 14, /* data=*/ 16);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix1(U8G2_R0, /* clock=*/ 43, /* data=*/ 39, /* cs=*/ 41, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix2(U8G2_R0, /* clock=*/ 52, /* data=*/ 51, /* cs=*/ 50, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);


// OutputPane
OutputPane OS1(tela1_cursor,   &tela1,   FONT_BIG);
OutputPane OS2(tela2_cursor,   &tela2,   FONT_BIG);
OutputPane MS1(matrix1_cursor, &matrix1, FONT_MATRIX);
OutputPane MS2(matrix2_cursor, &matrix2, FONT_MATRIX);

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
  // if(tela->getStrWidth(str2char(msg)) > tela->getDisplayWidth()){
    // int max = tela->getDisplayWidth()/tela->getMaxCharWidth();
    // String buf=msg.substring(0, max);
    // while(tela->getStrWidth(str2char(buf)) < tela->getDisplayWidth()){
      // buf=msg.substring(0,max);
      // max++;
    // }
    // write(msg.substring(0,max), tela, cursor, font);
    // cursor.x=0;
    // cursor.linebreak();
    // write(msg.substring(max), tela, cursor, font);
    // return;  
  // }
  char char_array[str_len];
  msg.toCharArray(char_array, str_len);
  tela->clearBuffer();
  tela->drawStr(cursor.x, cursor.y, char_array);	// write something to the internal memory
  tela->sendBuffer();					// transfer internal memory to the display
  // cursor.inc(char_width*str_len);
}

void write(String msg, OutputPane output) {write(msg, output.tela, output.cursor, output.font);}

void print(String msg, U8G2 *tela, Cursor &cursor = tela1_cursor, const uint8_t *font = FONT_SMALL){
  tela->clearBuffer();
  write(msg, tela, cursor, font);
}

void print(String msg, OutputPane output) {print(msg, output.tela, output.cursor, output.font);}

void tone(int freq, int time){
  analogWrite(A0, 255);
  tone(BUZZER, freq, time);
  app.delay(time, REACT(analogWrite(A0, 0)));
}

void matrixMirrowedDrawBox(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h){
  matrix1.clearBuffer();
  matrix2.clearBuffer();
  matrix1.drawBox(x, y, w, h);
  matrix2.drawBox(x, y, w, h);
  matrix1.sendBuffer();					
  matrix2.sendBuffer();					
}

void matrixMirrowedPrint(String n){
  matrix1_cursor.x=0;
  matrix1_cursor.y=8;
  matrix2_cursor.x=0;
  matrix2_cursor.y=8;
  print(n, &matrix1, matrix1_cursor, FONT_MATRIX);
  print(n, &matrix2, matrix2_cursor, FONT_MATRIX);
}

void mirrowedPrint(String n){
  tela1_cursor.x=30;
  tela1_cursor.y=40;
  tela2_cursor.x=30;
  tela2_cursor.y=40;
  print(n, &tela1, tela1_cursor, FONT_BIG);
  print(n, &tela2, tela2_cursor, FONT_BIG);
}


// Keyboard Handling
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

void U2SemOff(int sema[3]){
  for(int i=0; i<2; i++){
    analogWrite(sema[i], 0);
  }
}

void ASemOff(int sema[3]){
  for(int i=0; i<3; i++){
    analogWrite(sema[i], 0);
  }
}

void ASemOn(int sema[3]){
  for(int i=0; i<3; i++){
    analogWrite(sema[i], 0);
  }
}

void inc_matrix1(int start = 0){
      static int count = start;
      if(count >= 100)
        count = 0;
      matrix1_cursor.x=0;
      matrix1_cursor.y=8;
      print(String(count), &matrix1, matrix1_cursor, FONT_MATRIX);
      count++;
}

void inc_matrix2(int start = 0){
      static int count = start;
      if(count >= 200)
        count = 0;
      matrix2_cursor.x=0;
      matrix2_cursor.y=8;
      print(String(count), &matrix2, matrix2_cursor, FONT_MATRIX);
      count++;
}

int race_n_laps = String(DEFAULT_LAPS).toInt();
reaction p1r;
reaction p2r;
reaction wmr;
char receiveKey();
void largada();
void game();
void win(int n){
  log("Player" + String(n) + " won");
  matrix1.clear();
  matrix2.clear();
  if(n==1){
    print("Won!", &tela1, tela1_cursor, FONT_BIG);
    app.repeat(500, [](){
        static bool state = true;
        if (state)
          ASemOn(SEMA1);
        else
          ASemOff(SEMA1);
        state = !state;
    });
  }
  if(n==2){
    print("Won!", &tela2, tela2_cursor, FONT_BIG);
    app.repeat(500, [](){
        static bool state = true;
        if (state)
          ASemOn(SEMA2);
        else
          ASemOff(SEMA2);
        state = !state;
    });
  }
  wmr = app.repeat(KEYBOARD_DELAY, [](){
  if (receiveKey()=='D') {
    ASemOff(SEMA2);
    ASemOff(SEMA1);
    noTone(BUZZER);
    app.free(wmr);
  }
  });
}

reaction Rlargada[4], qdr1, qdr2, Rwaitd;
void waitd(react_callback cb);
volatile int p1_laps;
volatile int p2_laps;
unsigned long start;
void race(){
  p1_laps = -1;
  p2_laps = -1;
  start = 0;
  bindKey('D', [](){
      app.free(p1r);
      app.free(p2r);
      game();
  });
  debug("Racing");
  p1r = app.onPinRising(LAPP1, [](){
      if(p1_laps == -1){
        p1_laps++;
        return;
      }
      DEB(p1_laps);
      if(p1_laps > race_n_laps){
        app.free(p1r);
        app.free(p2r);
        p1Laps[p1_laps].time = start - millis();
        win(1);
        return;
      }
      if(p1_laps == 0)
        p1Laps[p1_laps].lap_time = - start + millis();
      else
        p1Laps[p1_laps].lap_time = - p1Laps[p1_laps - 1].lap_time + millis();
      tela1_cursor.x = 10;
      int sec = p1Laps[p1_laps].lap_time / 1000;
      int ms = p1Laps[p1_laps].lap_time % 1000;
      print(String(sec)+"."+String(ms), OS1);
      p1_laps++;
      inc_matrix1(0);
  });
  p2r = app.onPinRising(LAPP2, [](){
      if(p2_laps == -1) {
        p2_laps++;
        return;
      }
      DEB(p2_laps);
      if(p2_laps > race_n_laps){
        app.free(p1r);
        app.free(p2r);
        win(2);
        return;
      }
      p2_laps++;
      inc_matrix2(0);
  });
}

void wait_d(react_callback cb){
  char key = receiveKey(); 
  if (key=='D') {
    cb();
    app.free(Rlargada);
  }
}

void quemada(){
  for(int i = 0; i<4; i++){
    app.free(Rlargada[i]);
  }
  Rwaitd = app.repeat(KEYBOARD_DELAY, REACT(wait_d([](){
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    print("Ready?", &tela1, tela1_cursor, FONT_BIG);
    print("Ready?", &tela2, tela2_cursor, FONT_BIG);
    app.delay(1000, REACT(largada()));
  })));

  app.delay(10, REACT(tone(NOTE_C4, 200)));
  app.delay(400, REACT(tone(NOTE_C4, 600)));
}

void largada(){
  const int delays[4] = {100, 1500, 3000, 4500};
  qdr1 = app.onPinFalling(LAPP1, [](){
    tela1_cursor.x=30;
    tela1_cursor.y=40;
    print("Queimou!", &tela1, tela1_cursor, FONT_BIG);
    print("", &tela2, tela2_cursor, FONT_BIG);
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    DELAY_AWRITE(0, SEMA1[0], 255);
    quemada();
    app.free(qdr1);
    app.free(qdr2);
    matrix1.clear();
    matrix2.clear();
  });
  qdr2 = app.onPinFalling(LAPP2, [](){
    tela2_cursor.x=30;
    tela2_cursor.y=40;
    print("Queimou!", &tela2, tela2_cursor, FONT_BIG);
    print("", &tela1, tela1_cursor, FONT_BIG);
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    DELAY_AWRITE(0, SEMA2[0], 255);
    quemada();
    app.free(qdr1);
    app.free(qdr2);
    matrix1.clear();
    matrix2.clear();
  }); 

  Rlargada[0] = app.delay(delays[0],[](){
      analogWrite(SEMA1[0], 255);
      analogWrite(SEMA2[0], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 5, 8, 8);
      mirrowedPrint("3");
  });

  Rlargada[1] = app.delay(delays[1],[](){
      analogWrite(SEMA1[1], 255);
      analogWrite(SEMA2[1], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 3, 8, 8);
      mirrowedPrint("2");
  });

  Rlargada[2] = app.delay(delays[2],[](){
      analogWrite(SEMA1[2], 255);
      analogWrite(SEMA2[2], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 0, 8, 8);
      mirrowedPrint("1");
  });

  Rlargada[3] = app.delay(delays[3],[](){
      app.free(qdr1);
      app.free(qdr2);
      matrix1.clear();
      matrix2.clear();
      U2SemOff(SEMA1);
      U2SemOff(SEMA2);
      tone(NOTE_E5, 1000);
      mirrowedPrint("Go!");
      matrixMirrowedPrint("0");
      race();
  });
}

// Game
reaction Rmenu;
void game(){
  print("Voltas", &tela1, tela1_cursor, FONT_BIG);
  print(DEFAULT_LAPS, &tela2, tela2_cursor, FONT_BIG);
  matrixMirrowedPrint("");
  Rmenu = app.repeat(KEYBOARD_DELAY, REACT(menu_input()));
}

void menu_input(){
  static String last = "";
  char key = receiveKey(); 
  if (key=='D') {
    if (last.toInt()>0)
      race_n_laps = last.toInt();
    log("Set "+String(race_n_laps)+" laps.");
    print("Ready?", &tela1, tela1_cursor, FONT_BIG);
    print("Ready?", &tela2, tela2_cursor, FONT_BIG);
    app.delay(1000, REACT(largada()));
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
  app.delay(0, REACT(tone(NOTE_C4, 100)));
  app.delay(200, REACT(tone(NOTE_C4, 500)));
  m1r=app.repeat(1000, REACT(inc_matrix1()));
  m2r=app.repeat(800, REACT(inc_matrix2()));
  app.repeat(KEYBOARD_DELAY, REACT(test_process_input()));
  app.repeat(500, [](){
      static int count = 0;
      tela1_cursor.x=0;
      print("test1 "+String(count), OS1);
      count++;
  });
  t2r=app.repeat(500, [](){
      static int count = 0;
      tela2_cursor.x=0;
      print("test2 "+String(count), OS2);
      count++;
  });
  app.onPinFalling(LAPP1, [](){
      app.free(m1r);
      inc_matrix1(1);
  });
  app.onPinFalling(LAPP2, [](){
      app.free(m2r);
      inc_matrix2(2);
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
  tela1.enableUTF8Print();	
  tela2.begin();
  tela2.enableUTF8Print();	
  boot();
  // #define test_width 16
  // #define test_height 16
  // static unsigned char test_bits[] = {
     // 0xff, 0xff, 0x01, 0x80, 0xfd, 0xbf, 0x05, 0xa0, 0xf5, 0xaf, 0x15, 0xa8,
     // 0xd5, 0xab, 0x55, 0xaa, 0x55, 0xaa, 0xd5, 0xab, 0x15, 0xa8, 0xf5, 0xaf,
     // 0x05, 0xa0, 0xfd, 0xbf, 0x01, 0x80, 0xff, 0xff};
  // tela1.clear();
  // tela1.drawXBM( 0, 0, test_width, test_height, test_bits);
  // tela1.sendBuffer();
}

Reactduino app(app_main);
