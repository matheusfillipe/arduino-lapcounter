/* ######################################################################### */
/* #  Matheus Fillipe -- 28, January of 2021                               # */
/* #                                                                       # */
/* ######################################################################### */
/* #  Description: A slot car lap counter for arduino. This sketch and the * # */
/* # header files and modified libraries are necessary for it to work co_  # */
/* # rrectly.                                                              # */
/* ######################################################################### */
/* THIS IS LICENSED UNDER THE MIT LICENSE */
        

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
#include "assets.h"


// Classes
typedef struct Lap_struct {
  unsigned int player : 1;
  int num : BITS_LAPS;
  unsigned long lap_time : 18; // 4 min is the max for a lap
} Lap;

class GameOptions{
  public:
    int n_laps;
    int autonomy;
    int fails;
    GameOptions(int n_laps, int autonomy, int fails): n_laps(n_laps), autonomy(autonomy), fails(fails){}
};

struct PlayerState{
  Lap Laps[MAX_LAPS];
  unsigned long pbltime;
  int p_laps;
  int fuel;
  bool justRefueled : 1;
  bool isOnPit;
  bool isOnPitDelay;
  reaction pitstop;
  reaction pitDelay;
  int fail_lap;
  int num;
};


// Globals
PlayerState P1;
PlayerState P2;

bool gameloop = false;
bool mute = false;

Cursor screen1_cursor(128, 64, 18, 22, 5);
Cursor screen2_cursor(128, 64, 18, 22, 5);
Cursor bl1Cursor(128, 64, 60);
Cursor bl2Cursor(128, 64, 60);
Cursor f1Cursor(128, 64, 12);
Cursor f2Cursor(128, 64, 12);

Cursor matrix1_cursor(8, 8, 8);
Cursor matrix2_cursor(8, 8, 8);
ReactionManager rman;

GameOptions options(String(DEFAULT_LAPS).toInt(), DEFAULT_AUTONOMY, DEFAULT_FAILURE);

// Screens
U8G2_SSD1306_128X64_NONAME_F_SW_I2C screen1(U8G2_R0, /* clock=*/ SCREEN1[0], /* data=*/ SCREEN1[1]);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C screen2(U8G2_R0, /* clock=*/ SCREEN2[0], /* data=*/ SCREEN2[1]);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix1(U8G2_R0, /* clock=*/ MATRIX1[0], /* data=*/ MATRIX1[1], /* cs=*/ MATRIX1[2], /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);
U8G2_MAX7219_8X8_F_4W_SW_SPI matrix2(U8G2_R0, /* clock=*/ MATRIX2[0], /* data=*/ MATRIX2[1], /* cs=*/ MATRIX2[2], /* dc=*/ U8X8_PIN_NONE, /* reset=*/ U8X8_PIN_NONE);


// LabelWidget
LabelWidget OS1(screen1_cursor,   &screen1,   FONT_BIG);
LabelWidget BLS1(bl1Cursor,    &screen1,   FONT_SMALL);
LabelWidget FS1(f1Cursor,    &screen1,   FONT_SMALL);

LabelWidget OS2(screen2_cursor,   &screen2,   FONT_BIG);
LabelWidget BLS2(bl2Cursor,   &screen2,   FONT_SMALL);
LabelWidget FS2(f2Cursor,    &screen2,   FONT_SMALL);

LabelWidget MS1(matrix1_cursor, &matrix1, FONT_MATRIX);
LabelWidget MS2(matrix2_cursor, &matrix2, FONT_MATRIX);

// Game
#include "utils.h"

void countdown();
void freePitStop(PlayerState &P, bool force = false);
void game();
void race();
void startup();
void menu_input();
void alertSound();
void negativeSound();
void refuel(PlayerState &P, LabelWidget &FS);
void addLap(PlayerState &P, bool &animating, reaction &matrix_print_reaction, bool  (*matrix_print)(int), LabelWidget &OS, LabelWidget &BLS, LabelWidget &FS);
#include "counter.h"

// Sounds
void negativeSound(){
  tone(NOTE_C4, 200);
  app.delay(400, REACT(tone(NOTE_C4, 600)));
}

#define VICT_TIME 220
void victorySound(){
  int offset = 3000;
  int spacing = 30;
  app.delay(offset,                        REACT(tone(NOTE_E4, VICT_TIME)));
  app.delay(offset+spacing + VICT_TIME,    REACT(tone(NOTE_E4, VICT_TIME)));
  app.delay(offset+2*spacing+2*VICT_TIME,  REACT(tone(NOTE_D4, VICT_TIME*4)));
  app.delay(offset+3*spacing+8*VICT_TIME,  REACT(tone(NOTE_D4, VICT_TIME)));
  app.delay(offset+4*spacing+9*VICT_TIME,  REACT(tone(NOTE_D4, VICT_TIME)));
  app.delay(offset+5*spacing+10*VICT_TIME, REACT(tone(NOTE_C4, VICT_TIME*4)));
}

void alertSound(){
  app.delay(10,           REACT(tone(NOTE_C5,   VICT_TIME)));
  app.delay(10+VICT_TIME, REACT(tone(NOTE_C5, 3*VICT_TIME)));
}

unsigned long get_total_time(PlayerState &P){
    unsigned long total_time = 0;
    for(int i = 0; i < P.p_laps; i++){
      total_time += P.Laps[i].lap_time;
    }
    return total_time;
}


// Race
const int winXOffset = 80;
void printWin(int n){
    if(n==1){
      print(TEXT_WIN, OS1);
      write(TEXT_BESTLAP+timestamp(P1.pbltime), BLS1);
      BLS1.cursor.x+=winXOffset;
      write(timestamp(P1.Laps[P1.p_laps-1].lap_time), BLS1);
      BLS1.cursor.x-=winXOffset;

      print(TEXT_LOOSE, OS2);
      write(TEXT_BESTLAP+timestamp(P2.pbltime), BLS2);
      BLS2.cursor.x+=winXOffset;
      write(timestamp(P2.Laps[P2.p_laps-1].lap_time), BLS2);
      BLS2.cursor.x-=winXOffset;

      matrix_cross(&matrix2);

      FS1.screen->setFont(FS1.font);
      iwrite(TEXT_TOTAL_TIME+timestamp(get_total_time(P1))+"s", FS1);	
      FS1.screen->updateDisplayArea(0, 0, 16, 3);
  }
    else if(n==2){
      print(TEXT_WIN, OS2);
      write(TEXT_BESTLAP+timestamp(P2.pbltime), BLS2);
      BLS2.cursor.x+=winXOffset;
      write(timestamp(P2.Laps[P2.p_laps-1].lap_time), BLS2);
      BLS2.cursor.x-=winXOffset;

      print(TEXT_LOOSE, OS1);
      write(TEXT_BESTLAP+timestamp(P1.pbltime), BLS1);
      BLS1.cursor.x+=winXOffset;
      write(timestamp(P1.Laps[P1.p_laps-1].lap_time), BLS1);
      BLS1.cursor.x-=winXOffset;

      matrix_cross(&matrix1);

      FS2.screen->setFont(FS2.font);
      iwrite(TEXT_TOTAL_TIME+timestamp(get_total_time(P2))+"s", FS2);	
      FS2.screen->updateDisplayArea(0, 0, 16, 3);
    }
}

void serialTranferLaps(){
    log("-----------------------------------------------");
    log("PLAYER 1");
    for(int i = 0; i < P1.p_laps; i++){
      serialSend("LAPTIME", 1, P1.Laps[i].lap_time, i+1);
    }
    log("-----------------------------------------------");
    log("PLAYER 2");
    for(int i = 0; i < P2.p_laps; i++){
      serialSend("LAPTIME", 2, P2.Laps[i].lap_time, i+1);
    }
    log("-----------------------------------------------");
}

void win(int n){
  rman.free();
  serialSend("WIN", n);
  tone(NOTE_A5, 1500);
  freePitStop(P1);
  freePitStop(P2);
  matrix1.clear();
  matrix2.clear();
  victorySound();
  printWin(n);
  serialTranferLaps();

  if(n==1){
    rman.add(app.repeat(500, [](){
        static bool wonState = true;
        if (wonState)
          ASemOn(SEMA[0]);
        else
          ASemOff(SEMA[0]);
        wonState = !wonState;
    }));
    rman.add(app.repeat(100, [](){
          matrix_win_animation(&matrix1);
    }));
  }
  if(n==2){
    rman.add(app.repeat(500, [](){
        static bool wonState = true;
        if (wonState)
          ASemOn(SEMA[1]);
        else
          ASemOff(SEMA[1]);
        wonState = !wonState;
    }));
    rman.add(app.repeat(100, [](){
          matrix_win_animation(&matrix2);
    }));
  }
}

unsigned long start;

void writeFuel(int fuel, LabelWidget FS){
    if (options.autonomy == 0)
      return;

    FS.screen->setFont(FS.font);
    FS.screen->drawStr(FS.cursor.x, FS.cursor.y, TEXT_FUEL);	
    FS.screen->drawBox(15, 5, fuel*(128-45)/100, 5);
    FS.screen->drawLine(15, 8, 15+fuel*(128-45)/100, 8);
    FS.screen->drawStr(FS.cursor.x+(128-25), FS.cursor.y, String(fuel/(100/options.autonomy)).c_str());	
}

void printLap(unsigned long dt, LabelWidget &OS, unsigned long pbltime, LabelWidget &LS, int fuel, LabelWidget &FS){
    FS.screen->clear();
    writeFuel(fuel, FS);
    OS.screen->setFont(OS.font);
    OS.screen->drawStr(OS.cursor.x, OS.cursor.y,   timestamp(dt).c_str()); 
    FS.screen->setFont(LS.font);
    LS.screen->drawStr(LS.cursor.x, LS.cursor.y, (TEXT_BESTLAP+timestamp(pbltime)).c_str());
    FS.screen->sendBuffer();
}

void addFailure( PlayerState &P, LabelWidget &OS ){
  OS.screen->clear();
  negativeSound();
  OS.screen->drawXBMP(0, 0, car_width, car_height, car_bits);
  OS.screen->sendBuffer();
  ASemOff(SEMA[P.num-ONE]);
  analogWrite(SEMA[P.num-ONE][0], 255);
  P.fuel = 0;
}

void addLap(PlayerState &P, bool &animating, reaction &matrix_print_reaction, bool  (*matrix_print)(int), LabelWidget &OS, LabelWidget &BLS, LabelWidget &FS){
  // Ignore first pass
    if(P.p_laps < 0) {
      P.p_laps++;
      OS.clear();
      P.fuel=100;
      return;
    }

    //Failure pitstops must be full time
    if(P.justRefueled && P.p_laps == P.fail_lap && P.fuel < 100){
      P.fuel = 0; 
      P.justRefueled = false;
    }

    // Count total time
    unsigned long total_time = 0;
    for(int i = 0; i < P.p_laps; i++){
      total_time += P.Laps[i].lap_time;
    }

    // Determine lap time
    unsigned long time = P.num == 1 ? p1Time : p2Time; // <---- !BAD;
    unsigned long dt = (time - start) - total_time; 
    if(dt < MIN_LAP_TIME){  
      debug("Ignoring"); 
      return; 
    } 

    // Fuel decrement
    if (!P.justRefueled && options.autonomy != 0){
      if (P.fuel <= 0){
        P.fuel = 0;
        negativeSound();
        ASemOff(SEMA[P.num-ONE]);
        analogWrite(SEMA[P.num-ONE][0], 255);
        OS.screen->clearBuffer();
        OS.screen->clear();
        if(options.fails && P.p_laps == P.fail_lap)
          OS.screen->drawXBMP(0, 0, car_width, car_height, car_bits);
        else
          OS.screen->drawXBMP(40, 14, fuel_width, fuel_height, fuel_bits);
        OS.screen->sendBuffer();
        return;
      }
      P.fuel -= 100/options.autonomy;
    }

    // Store lap time
    P.Laps[P.p_laps].lap_time = dt;
    P.pbltime = P.pbltime > dt || P.pbltime == 0 ? dt : P.pbltime;

    if(P.p_laps >= options.n_laps - 1){
      P.p_laps++;
      win(P.num);
      return;
    }

    // Low fuel alert
    if(P.fuel <= 0){ 
      tone(NOTE_E5, 500);
      ASemOff(SEMA[P.num - ONE]);
      analogWrite(SEMA[P.num-ONE][1], 255);
      if(options.n_laps && P.p_laps+1 == P.fail_lap)
        P.fail_lap = random(P.p_laps+1, options.n_laps-1);
    }else
      tone(NOTE_E4, 150);

    // Increment lap and Draw stuff
    if(animating){
      app.free(matrix_print_reaction);
      debug("Freeing matrix animation of P" +String(P.num));
    }
    animating = matrix_print(++P.p_laps);
    printLap(dt, OS, P.pbltime, BLS, P.fuel, FS);
    serialSend("LAPS", P.num, P.p_laps);
    serialSend("FUEL", P.num, P.fuel);
    DEB(animating);

    // Check random failure for next lap 
    if(options.fails && P.p_laps == P.fail_lap){
      if(P.num == 1)
        app.delay(random(FAILURE_MIN_DELAY, FAILURE_MAX_DELAY+ONE), [](){
            addFailure(P1, OS1);
        });
      if(P.num == 2)
        app.delay(random(FAILURE_MIN_DELAY, FAILURE_MAX_DELAY+ONE), [](){
            addFailure(P2, OS2);
        });
    }
}

void refuel(PlayerState &P, LabelWidget &FS){
  P.fuel += PITSTOP_REFUEL_AMMOUNT;
  P.fuel = P.fuel <= 100 ? P.fuel : 100;
  serialSend("FUEL", P.num, P.fuel);
  FS.screen->clearBuffer();
  writeFuel(P.fuel, FS);
  FS.screen->updateDisplayArea(0, 0, 16, 3);
  P.justRefueled = true;
  if(P.fuel >= 100) {
      if(P.p_laps >= P.fail_lap)
        P.fail_lap = MAX_LAPS + ONE; // Disable failure
      tone(NOTE_E5, 1000);
      ASemOff(SEMA[P.num-ONE]);
      analogWrite(SEMA[P.num-ONE][2], 255);
      app.disable(P.pitstop);
  }
}



void resetPlayers(){
  lastEventTime = millis();
  p1sState = SLEFT;
  p1Time = 0;
  P1sensorHandled = true;
  P1.num = 1;
  P1.p_laps = LAP_START;
  P1.pbltime = 0;
  P1.fuel = 100;
  P1.justRefueled = false;
  P1.isOnPit = false;

  p2sState = SLEFT;
  p2Time = 0;
  p2Time = 0;
  P2sensorHandled = true;
  P2.num = 2;
  P2.p_laps = LAP_START;
  P2.pbltime = 0;
  P2.fuel = 100;
  P2.justRefueled = false;
  P2.isOnPit = false;
}

void freePitStop(PlayerState &P, bool force = false){
  if(P.isOnPit || force){ 
    debug("Freeing pit stop");
    P.isOnPit = false;
    app.free(P.pitstop);
  } 
  if(P.isOnPitDelay || force){
    debug("Freeing pit stop delay");
    P.isOnPitDelay = false;
    app.free(P.pitDelay);
  }
}

/////////////////////////////////////////////////////////////////////////////////
reaction qdr1, qdr2;

void race(){
  rman.free();
  serialSend("RACE");
  resetPlayers();
  start = millis();
  bindKey('D', [](){
    freePitStop(P1);
    freePitStop(P2);
    game();
  });

  LAPCOUNT(LAPP1, REACT(handleSensorInput(LAPP1, p1sState, p1Time, P1sensorHandled, 1)));
  LAPCOUNT(LAPP2, REACT(handleSensorInput(LAPP2, p2sState, p2Time, P2sensorHandled, 2)));

  // CHECK-WRITE LOOP
  rman.add(app.repeat(RACE_LOOP_DELAY, [](){
    if(!P1sensorHandled && !P2sensorHandled){
      debug("Simultaneous pass");
      if (p1Time<p2Time){
        sensorHandleChange(onSensor1Change, P1sensorHandled, p1sState);
        sensorHandleChange(onSensor2Change, P2sensorHandled, p2sState);
      }else{
        sensorHandleChange(onSensor2Change, P2sensorHandled, p2sState);
        sensorHandleChange(onSensor1Change, P1sensorHandled, p1sState);
      }
    }
    if(!P1sensorHandled){
      debug("1 pass");
      sensorHandleChange(onSensor1Change, P1sensorHandled, p1sState);
    }
    if(!P2sensorHandled){
      debug("2 pass");
      sensorHandleChange(onSensor2Change, P2sensorHandled, p2sState);
    }
    checkHold(LAPP1, p1sState, p1Time, P1sensorHandled);
    checkHold(LAPP2, p2sState, p2Time, P2sensorHandled);
  }));

}

void generateFailures(){
  debug("Player 1 failures");
  generateRandomNumbers(P1.fail_lap, options.fails, options.n_laps);
  debug(P1.fail_lap);
  debug("Player 2 failures");
  generateRandomNumbers(P2.fail_lap, options.fails, options.n_laps);
  debug(P2.fail_lap);
}

void restart_countdown(int num){
  app.free(qdr1);
  app.free(qdr2);
  rman.free();

  matrix1.clear();
  matrix2.clear();
  matrix_cross(num==1? &matrix1 : &matrix2);
  bindKey('D', [](){
    matrix1.clear();
    matrix2.clear();
    startup();
  });
  negativeSound();
}

void countdown(){
  debug("Countdown");
  generateFailures();
  const int offset = 1500;
  const int delays[4] = {10, offset, 2*offset, 3*offset};
  screen1.clear();
  screen2.clear();

  rman.add(app.delay(delays[0],[](){
      analogWrite(SEMA[0][0], 255);
      analogWrite(SEMA[1][0], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 5, 8, 8);
      mirrowedPrint("3");
  }));

  rman.add(app.delay(delays[1],[](){
      analogWrite(SEMA[0][1], 255);
      analogWrite(SEMA[1][1], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 3, 8, 8);
      mirrowedPrint("2");
  }));

  rman.add(app.delay(delays[2],[](){
      analogWrite(SEMA[0][2], 255);
      analogWrite(SEMA[1][2], 255);
      tone(NOTE_E4, 400);
      matrixMirrowedDrawBox(0, 0, 8, 8);
      mirrowedPrint("1");
  }));

  rman.add(app.delay(delays[3],[](){
      app.free(qdr1);
      app.free(qdr2);
      matrix1.clear();
      matrix2.clear();
      U2SemOff(SEMA[0]);
      U2SemOff(SEMA[1]);
      tone(NOTE_E5, 1000);
      mirrowedPrint("Go!");
      matrixMirrowedPrint("0");
      race();
  }));
}

void startup(){
    rman.free();
    ASemOff(SEMA[0]);
    ASemOff(SEMA[1]);
    print(TEXT_STARTUP_READY, &screen1, screen1_cursor, FONT_BIG);
    print(TEXT_STARTUP_READY, &screen2, screen2_cursor, FONT_BIG);
    rman.add(app.delay(1000, REACT(countdown())));

    // Setup sensors
    qdr1 = app.onPinFalling(LAPP1, [](){
      screen1_cursor.x=TEXT_X_OFFSET;
      screen1_cursor.y=40;
      print(TEXT_STARTUP_BURNED, &screen1, screen1_cursor, FONT_BIG);
      print("", &screen2, screen2_cursor, FONT_BIG);
      ASemOff(SEMA[0]);
      ASemOff(SEMA[1]);
      restart_countdown(1);

      rman.add(app.repeat(BLINK_TIME_SLOW, [](){
            static bool state = true;
            analogWrite(SEMA[0][0], 255*state);
            state = !state;
      }));
    });

    qdr2 = app.onPinFalling(LAPP2, [](){
      screen2_cursor.x=TEXT_X_OFFSET;
      screen2_cursor.y=40;
      print(TEXT_STARTUP_BURNED, &screen2, screen2_cursor, FONT_BIG);
      print("", &screen1, screen1_cursor, FONT_BIG);
      ASemOff(SEMA[0]);
      ASemOff(SEMA[1]);
      restart_countdown(2);

      rman.add(app.repeat(BLINK_TIME_SLOW, [](){
            static bool state = true;
            analogWrite(SEMA[1][0], 255*state);
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
String last_text_menu = TEXT_MENU_LAPS;
String last_value_menu = "";
void printMenu(String msg, String num){
  FS1.screen->clear();
  iwrite(TEXT_MENU_HEADER, FS1);
  OS1.cursor.y+=10;
  OS1.cursor.x=0;
  iwrite(msg, OS1);
  OS1.cursor.y-=10;
  OS1.screen->sendBuffer();
  print(num, OS2);
  write(String(TEXT_MENU_CONFIRM) + ( mute ? " M" : "" ), FS2);
  last_text_menu = msg;
  last_value_menu = num;
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
  FS1.screen->clearBuffer();
  iwrite(last, &screen2, screen2_cursor, FONT_BIG);
  screen2.updateDisplayArea(0, 2, 8, 6);
  last_value_menu = last;
}

void menu_input(){
  static String laps(options.n_laps);
  static String autonomy(options.autonomy);
  static String fails(options.fails);
  static int menu_opt = 0;
  char key = receiveKey(); 
  switch (key){
    case '*': 
      mute = !mute;
      log("Muting");
      FS1.screen->clearBuffer();
      iwrite(String(TEXT_MENU_CONFIRM) + ( mute ? " M" : "" ), FS2);
      screen2.updateDisplayArea(0, 0, 16, 2);
      break;
    case 'A': 
      printMenu(TEXT_MENU_LAPS, laps);
      menu_opt = 0;
      break;
    case 'B': 
      printMenu(TEXT_MENU_AUTONOMY, autonomy.toInt() == 0 ? TEXT_MENU_NOTUSE : autonomy );
      menu_opt = 1;
      break;
    case 'C': 
      printMenu(TEXT_MENU_FAILURE, fails.toInt() == 0 ? TEXT_MENU_NOTUSE : TEXT_MENU_USE);
      menu_opt = 2;
      break;
    case 'D': 
      if(laps.toInt()>0)
        options.n_laps = laps.toInt();
      serialSend("SET", "laps", String(options.n_laps));
      if(autonomy.toInt()>=0)
        options.autonomy = autonomy.toInt();
      serialSend("SET", "autonomy", String(options.autonomy));
      if(fails.toInt()>=0)
        options.fails = fails.toInt();
      serialSend("SET", "fails", String(options.fails));
      startup();
      break;
  }
  if(key != '-' && key != ' ' && key && isdigit(key)){
    switch (menu_opt){
      case 0: // Laps
        laps = touched[0] ? laps : "";
        handleNumberInput(key, laps, 3, 1, MAX_LAPS);
        printMenuValue(laps);
        touched[0] = 1;
        break;
      case 1: // Autonomy
        autonomy = touched[1] ? autonomy : "";
        handleNumberInput(key, autonomy, laps.length(), 0, laps.toInt());
        if(autonomy.toInt() == 0){
          printMenuValue(TEXT_MENU_NOTUSE);
          touched[1] = 0;
        }
        else {
          printMenuValue(autonomy);
          touched[1] = 1;
        }
        break;
      case 2: // Failures
        fails = touched[2] ? fails : "";
        handleNumberInput(key, fails, 1, 0, min(laps.toInt(), MAX_FAILURES));
        if(fails.toInt() == 0){ 
          printMenuValue(TEXT_MENU_NOTUSE);
        }
        else{ 
          printMenuValue(TEXT_MENU_USE);
        }
        if(autonomy == "0"){
          debug("Setting autonomy to n_laps+1");
          autonomy = String(laps.toInt());
        }
        break;
    }
  }
}

void game(){
  rman.free();
  serialSend("MENU");
  touched[0] = 0;
  touched[1] = 0;
  touched[2] = 0;
  OS1.cursor.x=TEXT_X_OFFSET;
  OS2.cursor.x=TEXT_X_OFFSET;
  ASemOff(SEMA[1]);
  ASemOff(SEMA[0]);
  printMenu(last_text_menu, last_value_menu == "" ? String(options.n_laps) : last_value_menu);
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
    screen2_cursor.x=0;
    screen2_cursor.y=30;
    print(last, &screen2, screen2_cursor, FONT_SMALL);
  }
}

void testMode(){
  app.delay(0, REACT(tone(NOTE_C4, 100)));
  app.delay(200, REACT(tone(NOTE_C4, 500)));
//  m1r=app.repeat(1000, REACT(inc_matrix1(TEST_START)()));
//  m2r=app.repeat(800, REACT(inc_matrix2(TEST_START + 2)()));
  app.repeat(KEYBOARD_DELAY, REACT(test_process_input()));
  app.repeat(500, [](){
      static int count = 0;
      screen1_cursor.x=0;
      print("test1 "+String(count), OS1);
      count++;
  });
  t2r=app.repeat(500, [](){
      static int count = 0;
      screen2_cursor.x=0;
      print("test2 "+String(count), OS2);
      count++;
  });
  app.onPinFalling(LAPP1, [](){
     static bool freed = false;
     if (!freed)
        app.free(m1r);
      inc_matrix1(90)();
      freed=true;
  });
  app.onPinFalling(LAPP2, [](){
     static bool freed = false;
     if (!freed)
        app.free(m2r);
      inc_matrix2(90)();
      freed=true;
  });
}

reaction br, tr;
void boot(){

  // Logo
  screen1.clear();
  screen1.drawXBMP( 0, 6, logo_car1_width, logo_car1_height, logo_car1_bits);
  screen1.sendBuffer();
  screen2.clear();
  screen2.drawXBMP( 0, 6, logo_car2_width, logo_car2_height, logo_car2_bits);
  screen2.sendBuffer();

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
  randomSeed(analogRead(10));
  matrix1.begin();
  matrix2.begin();
  screen1.begin();
  screen1.enableUTF8Print();	
  screen2.begin();
  screen2.enableUTF8Print();	
  ENTRY_FUNC;
}

Reactduino app(app_main);
