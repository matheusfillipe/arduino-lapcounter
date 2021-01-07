// #include <Array.h>
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
  unsigned int player : 1;
  int num : BITS_LAPS;
  unsigned long lap_time;
} Lap;

class GameOptions{
  public:
    int n_laps;
    int autonomy;
    int fails;
    GameOptions(int n_laps, int autonomy, int fails): n_laps(n_laps), autonomy(autonomy), fails(fails){}
};


// Variables
Lap p1Laps[MAX_LAPS];
Lap p2Laps[MAX_LAPS];
unsigned long p1bltime;
unsigned long p2bltime;
int p1_laps;
int p2_laps; 

char inputBuffer[4];
bool gameloop = false;
Cursor tela1_cursor(128, 64, 18, 22, 5);
Cursor tela2_cursor(128, 64, 18, 22, 5);
Cursor bl1Cursor(128, 64, 60);
Cursor bl2Cursor(128, 64, 60);
Cursor f1Cursor(128, 64, 12);
Cursor f2Cursor(128, 64, 12);

Cursor matrix1_cursor(8, 8, 8);
Cursor matrix2_cursor(8, 8, 8);
ReactionManager rman;

GameOptions options(String(DEFAULT_LAPS).toInt(), DEFAULT_AUTONOMY, DEFAULT_FAILURE);

// Screens
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela1(U8G2_R0, /* clock=*/ 14, /* data=*/ 16);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C tela2(U8G2_R0, /* clock=*/ 18, /* data=*/ 17);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix1(U8G2_R0, /* clock=*/ 43, /* data=*/ 39, /* cs=*/ 41, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix2(U8G2_R0, /* clock=*/ 27, /* data=*/ 23, /* cs=*/ 25, /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);


// OutputPane
OutputPane OS1(tela1_cursor,   &tela1,   FONT_BIG);
OutputPane BLS1(bl1Cursor,    &tela1,   FONT_SMALL);
OutputPane FS1(f1Cursor,    &tela1,   FONT_SMALL);

OutputPane OS2(tela2_cursor,   &tela2,   FONT_BIG);
OutputPane BLS2(bl2Cursor,   &tela2,   FONT_SMALL);
OutputPane FS2(f2Cursor,    &tela2,   FONT_SMALL);

OutputPane MS1(matrix1_cursor, &matrix1, FONT_MATRIX);
OutputPane MS2(matrix2_cursor, &matrix2, FONT_MATRIX);

// Games
#include "utils.h"

void countdown();
void game();
void race();
void startup();
void menu_input();

void printWin(int n){
    log("Player" + String(n) + " won");
    if(n==1){
      print(TEXT_WIN, OS1);
      write(TEXT_BESTLAP+timestamp(p1bltime), BLS1);
      BLS1.cursor.x+=80;
      write(timestamp(p1Laps[p1_laps-1].lap_time), BLS1);
      BLS1.cursor.x-=80;

      print(TEXT_LOOSE, OS2);
      write(TEXT_BESTLAP+timestamp(p2bltime), BLS2);
      BLS2.cursor.x+=80;
      write(timestamp(p2Laps[p2_laps-1].lap_time), BLS2);
      BLS2.cursor.x-=80;
    }
    else if(n==2){
      print(TEXT_WIN, OS2);
      write(TEXT_BESTLAP+timestamp(p2bltime), BLS2);
      BLS2.cursor.x+=80;
      write(timestamp(p2Laps[p2_laps-1].lap_time), BLS2);
      BLS2.cursor.x-=80;

      print(TEXT_LOOSE, OS1);
      write(TEXT_BESTLAP+timestamp(p1bltime), BLS1);
      BLS1.cursor.x+=80;
      write(timestamp(p1Laps[p1_laps-1].lap_time), BLS1);
      BLS1.cursor.x-=80;
    }
}

void win(int n){
  rman.free();
  matrix1.clear();
  matrix2.clear();
  tone(NOTE_A5, 1500);
  printWin(n);

  if(n==1){
    rman.add(app.repeat(500, [](){
        static bool wonState = true;
        if (wonState)
          ASemOn(SEMA1);
        else
          ASemOff(SEMA1);
        wonState = !wonState;
    }));
  }
  if(n==2){
    rman.add(app.repeat(500, [](){
        static bool wonState = true;
        if (wonState)
          ASemOn(SEMA2);
        else
          ASemOff(SEMA2);
        wonState = !wonState;
    }));
  }
  bindKey('D', [](){
    game();
  });

}

unsigned long start;

void writeFuel(int fuel, OutputPane FS){
    FS.tela->setFont(FS.font);
    FS.tela->drawStr(FS.cursor.x, FS.cursor.y, TEXT_FUEL);	
    FS.tela->drawBox(15, 5, fuel*(128-45)/100, 5);
    FS.tela->drawLine(15, 8, 15+fuel*(128-45)/100, 8);
    FS.tela->drawStr(FS.cursor.x+(128-25), FS.cursor.y, String(fuel/(100/options.autonomy)).c_str());	
}

void printLap(unsigned long dt, OutputPane &OS, unsigned long pbltime, OutputPane &LS, unsigned long fuel, OutputPane &FS){
    FS.tela->clear();
    writeFuel(fuel, FS);
    FS.tela->setFont(OS.font);
    OS.tela->drawStr(OS.cursor.x, OS.cursor.y,   timestamp(dt).c_str()); 
    FS.tela->setFont(LS.font);
    LS.tela->drawStr(LS.cursor.x, LS.cursor.y, (TEXT_BESTLAP+timestamp(pbltime)).c_str());
    FS.tela->sendBuffer();
}

bool handleSensorEntered(int player, int pin, bool animating, unsigned long &pbltime, int &p_laps, Lap pLaps[], reaction &matrix_print_reaction, bool  (*matrix_print)(int), OutputPane &OS, OutputPane &BLS, OutputPane &FS, int &fuel){
    if(digitalRead(pin)==0)
      return animating;
    if(p_laps < 0) {
      p_laps++;
      OS.clear();
      fuel=100;
      return animating;
    }
    unsigned long total_time = 0;
    for(int i = 0; i < p_laps; i++){
      total_time += pLaps[i].lap_time;
    }
    unsigned long dt = (millis() - start) - total_time; 
    if(dt < MIN_LAP_TIME){  
      debug("Ignoring"); 
      return animating; 
    } 
    pLaps[p_laps].lap_time = dt;

    if(p_laps >= options.n_laps - 1){
      win(player);
      return animating;
    }
    if(fuel - 200/options.autonomy <= 0)
      tone(NOTE_E5, 500);
    else
      tone(NOTE_E4, 150);
    pbltime = pbltime > dt || pbltime == 0 ? dt : pbltime;
    animating = matrix_print(++p_laps);
    printLap(dt, OS, pbltime, BLS, fuel, FS);
    if(animating)
      app.free(matrix_print_reaction);

    fuel -= 100/options.autonomy;
    fuel = fuel < 0 ? 0 : fuel;

    return animating;
}

void resetPlayers(){
  p1_laps = LAP_START;
  p2_laps = LAP_START;
  p1bltime = 0;
  p2bltime = 0;
}


reaction qdr1, qdr2;
void race(){
  debug("Race");
  resetPlayers();

  start = millis();
  rman.add(bindKey('D', game));

  rman.add(app.onPinRising(LAPP1, [](){
      static bool animating = false;
      static int fuel = 100;
      animating = handleSensorEntered(1, LAPP1, animating, p1bltime, p1_laps, p1Laps, matrix1_print_reaction, matrix1_print, OS1, BLS1, FS1, fuel);
  }));

  rman.add(app.onPinRising(LAPP2, [](){
      static bool animating = false;
      static int fuel = 100;
      animating = handleSensorEntered(2, LAPP2, animating, p2bltime, p2_laps, p2Laps, matrix2_print_reaction, matrix2_print, OS2, BLS2, FS2, fuel);
  }));
}


void restart_countdown(){
  app.free(qdr1);
  app.free(qdr2);
  rman.free();
  matrix1.clear();
  matrix2.clear();
  bindKey('D', [](){
    startup();
  });
  app.delay(10, REACT(tone(NOTE_C4, 200)));
  app.delay(400, REACT(tone(NOTE_C4, 600)));
}

void countdown(){
  debug("Countdown");
  const int offset = 1500;
  const int delays[4] = {10, offset+200, 2*offset, 3*offset};


  rman.add(app.delay(delays[0],[](){
      analogWrite(SEMA1[0], 255);
      analogWrite(SEMA2[0], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 5, 8, 8);
      mirrowedPrint("3");
  }));

  rman.add(app.delay(delays[1],[](){
      analogWrite(SEMA1[1], 255);
      analogWrite(SEMA2[1], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 3, 8, 8);
      mirrowedPrint("2");
  }));

  rman.add(app.delay(delays[2],[](){
      analogWrite(SEMA1[2], 255);
      analogWrite(SEMA2[2], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 0, 8, 8);
      mirrowedPrint("1");
  }));

  rman.add(app.delay(delays[3],[](){
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
  }));
}

void startup(){
    rman.free();
    ASemOff(SEMA1);
    ASemOff(SEMA2);
    print(TEXT_STARTUP_READY, &tela1, tela1_cursor, FONT_BIG);
    print(TEXT_STARTUP_READY, &tela2, tela2_cursor, FONT_BIG);
    rman.add(app.delay(1000, REACT(countdown())));

    // Setup sensors
    qdr1 = app.onPinFalling(LAPP1, [](){
      tela1_cursor.x=30;
      tela1_cursor.y=40;
      print(TEXT_STARTUP_BURNED, &tela1, tela1_cursor, FONT_BIG);
      print("", &tela2, tela2_cursor, FONT_BIG);
      ASemOff(SEMA1);
      ASemOff(SEMA2);
      restart_countdown();
      rman.add(app.repeat(BLINK_TIME_SLOW, [](){
            static bool state = true;
            analogWrite(SEMA1[0], 255*state);
            state = !state;
      }));
    });

    qdr2 = app.onPinFalling(LAPP2, [](){
      tela2_cursor.x=30;
      tela2_cursor.y=40;
      print(TEXT_STARTUP_BURNED, &tela2, tela2_cursor, FONT_BIG);
      print("", &tela1, tela1_cursor, FONT_BIG);
      ASemOff(SEMA1);
      ASemOff(SEMA2);
      restart_countdown();
      rman.add(app.repeat(BLINK_TIME_SLOW, [](){
            static bool state = true;
            analogWrite(SEMA2[0], 255*state);
            state = !state;
      }));
    }); 

    app.disable(qdr1);
    app.disable(qdr2);
    // Avoid bad readings to cause problems (NO idea why but this fixes false
    // quemadas de largada)
    app.delay(50, [](){
        app.enable(qdr1);
        app.enable(qdr2);
    });
}

// Menu
void printMenu(String msg, String num){
  FS1.tela->clear();
  iwrite(TEXT_MENU_HEADER, FS1);
  OS1.cursor.y+=10;
  OS1.cursor.x=0;
  iwrite(msg, OS1);
  OS1.cursor.y-=10;
  OS1.tela->sendBuffer();
  print(num, OS2);
  write(TEXT_MENU_CONFIRM, FS2);
}

int touched[] = {0, 0, 0};
void handleNumberInput(char key, String &last, int digits, int min_, int max_){
  if (last.length()>digits - 1)
    last = "";
  last += String(key);
  if (last.toInt()>max_)
    last = String(max_);
  if (last.toInt() < min_ && last.length()>digits - 1){
    String zeroes = "";
    String _min(min_);
    for(int i = 0; i < digits - _min.length(); i++)
      zeroes += "0";
    last = zeroes+_min;
  }
}

void printMenuValue(String last){
  tela2_cursor.x=0;
  tela2_cursor.y=40;
  tela2.clear();
  iwrite(TEXT_MENU_CONFIRM, FS2);
  iwrite(last, &tela2, tela2_cursor, FONT_BIG);
  tela2.updateDisplayArea(0, 0, 8, 6);
}

void menu_input(){
  static String laps(options.n_laps);
  static String autonomy(options.autonomy);
  static String fails(options.fails);
  static int menu_opt = 0;
  char key = receiveKey(); 
  switch (key){
    case 'A': 
      printMenu(TEXT_MENU_LAPS, laps);
      menu_opt = 0;
      break;
    case 'B': 
      printMenu(TEXT_MENU_AUTONOMY, autonomy.toInt() == 0 ? TEXT_MENU_NOTUSE : autonomy );
      menu_opt = 1;
      break;
    case 'C': 
      printMenu(TEXT_MENU_FAILURE, fails.toInt() == 0 ? TEXT_MENU_NOTUSE : fails );
      menu_opt = 2;
      break;
    case 'D': 
      if(laps.toInt()>0)
        options.n_laps = laps.toInt();
      log("Set "+String(options.n_laps)+" laps.");
      if(autonomy.toInt()>=0)
        options.autonomy = autonomy.toInt();
      log("Set "+String(options.autonomy)+" autonomy.");
      if(fails.toInt()>=0)
        options.fails = fails.toInt();
      log("Set "+String(options.fails)+" fails.");
      startup();
      break;
  }
  if(key != '-' && key != ' ' && key && isdigit(key)){
    switch (menu_opt){
      case 0:
        laps = touched[0] ? laps : "";
        handleNumberInput(key, laps, 3, 1, MAX_LAPS);
        printMenuValue(laps);
        touched[0] = 1;
        break;
      case 1:
        autonomy = touched[1] ? autonomy : "";
        handleNumberInput(key, autonomy, 3, 0, MAX_LAPS);
        if(autonomy.toInt() == 0){
          printMenuValue(TEXT_MENU_NOTUSE);
          touched[1] = 0;
        }
        else {
          printMenuValue(autonomy);
          touched[1] = 1;
        }
        break;
      case 2:
        fails = touched[2] ? fails : "";
        handleNumberInput(key, fails, 3, 0, MAX_LAPS);
        if(fails.toInt() == 0){ 
          printMenuValue(TEXT_MENU_NOTUSE);
          touched[2] = 0;
        }
        else{ 
          printMenuValue(fails);
          touched[2] = 1;
        }
        break;
    }
  }
}

void game(){
  rman.free();
  touched[0] = 0;
  touched[1] = 0;
  touched[2] = 0;
  ASemOff(SEMA2);
  ASemOff(SEMA1);
  printMenu(TEXT_MENU_LAPS, String(options.n_laps));
  matrixMirrowedPrint("");
  debug("Menu");
  rman.add(app.repeat(KEYBOARD_DELAY, menu_input));
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
  m1r=app.repeat(1000, REACT(inc_matrix1(TEST_START)()));
  m2r=app.repeat(800, REACT(inc_matrix2(TEST_START + 2)()));
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
      inc_matrix1(0)();
  });
  app.onPinFalling(LAPP2, [](){
      app.free(m2r);
      inc_matrix2(10)();
  });
}

reaction br, tr;
void boot(){
  br = app.delay(TEST_DELAY, [](){
    //start game
    log("STARTED!!");
    app.free(tr);
    game();
  });

  app.repeat(KEYBOARD_RESET_DELAY, [](){
      if(keypad.getState() == HOLD){
        log("RESETING");
        app.free(tr);
        app.free(br);
        resetProgram();
      }
  });

  //trigger test mode
  tr = app.repeat(KEYBOARD_DELAY, [](){
    static bool testing = false;
    if (testing){
      app.free(tr);
      return;
    }
    if ( keypad.getKey() == 'D') {
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
  keypad.setHoldTime(RESET_TIME);
  randomSeed(analogRead(0));
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
