/* TODO 
 * bug: simultaneous rpitsopt 2 playres. Only freezes refueling if the lap from
 * the other player is valid, otherwise (if invalid) the problem doesn't happen
 
 * Not clear player screen glitch artifact on fefule + lap
*/

#define DEBUG true
#define MUTE true
#define _IST_SENSOR_ 1

// PINS
#define LAPP1 2
#define LAPP2 3
#define P1LED 12
#define P2LED 11
#define BUZZER 8
#define BUZZER2 9
const uint16_t SEMA[][3] = {{45, 47, 49}, // Player 1
                            {48, 46, 44}         // 2 
                          };
const uint16_t SCREEN1[] = {/* clock=*/ 14, /* data=*/ 16};
const uint16_t SCREEN2[] = {/* clock=*/ 18, /* data=*/ 17};
const uint16_t MATRIX1[] = {/* clock=*/ 27, /* data=*/ 23, /* cs=*/ 25};
const uint16_t MATRIX2[] = {/* clock=*/ 43, /* data=*/ 39, /* cs=*/ 41};


// Constants
#define MAX_LAPS 255
#define MAX_FAILURES 3
#define BITS_LAPS 8
#define DEFAULT_LAPS 20
#define DEFAULT_AUTONOMY 5
#define DEFAULT_FAILURE 0

#define RACE_LOOP_DELAY 50
#define SHOLD_DELAY 250
#define BLINK_TIME 100
#define BLINK_TIME_SLOW 100
#define N_PINS 24
#define DEFAULT_BLINK_DURATION 2000
#define KEYBOARD_DELAY 20
#define KEYBOARD_RESET_DELAY 1500
#define TEST_DELAY 1500
#define MIN_LAP_TIME 900.0
#define RESET_TIME 3000

#define PITSTOP_TIME 1000
#define PITSTOP_REFUEL_AMMOUNT 15

#define FAILURE_MAX_DELAY 1100
#define FAILURE_MIN_DELAY 400

#define FONT_BIG u8g2_font_logisoso18_tf
#define FONT_MATRIX u8g2_font_trixel_square_tn
#define FONT_SMALL  u8g2_font_7x13B_tf
#define FONT_RATIO 4

#define MATRIX_Y_CENTER 7
#define MATRIX_ANIMATION_PAUSE 2
#define MATRIX_ANIMATION_DELAY 300

#define TEST_START 0
#define LAP_START -1 //-1

#define MAX_SIMUL_REACTIONS 8
#define ONE 1
#define _THOUSAND 1000

// TEXT & TRANSLATIONS
#define T_PORTUGUESE 1

#define TEXT_FUEL "F"
#define TEXT_WIN "Win!"
#define TEXT_LOOSE "Lost!"
#define TEXT_BESTLAP "BL: "

#define TEXT_STARTUP_BURNED "False!"
#define TEXT_STARTUP_READY "Ready?"

#define TEXT_MENU_LAPS "Laps: "
#define TEXT_MENU_HEADER "A)LAP B)PIT C)FAIL"
#define TEXT_MENU_CONFIRM "D)START  *)MUTE"
#define TEXT_MENU_PITSTOP "Pit Stop"
#define TEXT_MENU_AUTONOMY "Autonomy: "
#define TEXT_MENU_FAILURE "Failure: "
#define TEXT_MENU_NOTUSE "OFF"
#define TEXT_MENU_USE "ON"

#define TEXT_RACE_PITSTOP "PIT STOP"

#define TEXT_X_OFFSET 30

#ifdef T_PORTUGUESE
#define TEXT_FUEL "F"
#define TEXT_WIN "Vencedor!"
#define TEXT_LOOSE "Perdedor!"
#define TEXT_BESTLAP "MV: "

#define TEXT_STARTUP_BURNED "Queimou!"
#define TEXT_STARTUP_READY "Preparar!"

#define TEXT_MENU_LAPS "Voltas: "
#define TEXT_MENU_HEADER "A)LAP B)PIT C)FAIL"
#define TEXT_MENU_CONFIRM "D)START  *)MUTE"
#define TEXT_MENU_PITSTOP "Pit Stop"
#define TEXT_MENU_AUTONOMY "Autonomia: "
#define TEXT_MENU_FAILURE "Falhas: "

#define TEXT_RACE_PITSTOP "PIT STOP"

#define TEXT_X_OFFSET 20
#endif

// Macros
#define LAPCOUNT(pin, func) rman.add(app.onPinChangeNoInt(pin, func))
#ifdef _IST_SENSOR_
#define LAPCOUNT(pin, func) rman.add(app.onPinChange(pin, func))
#endif

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
  {'*','0','#','D'}
};

byte rowPins[rows] = {22, 24, 26, 28}; //connect to the row pinouts of the keypad
byte colPins[cols] = {30, 32, 34, 36}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


template<typename T>
void debug(T msg) {
  if (DEBUG) {
    Serial.println(String("DEBUG: ") + String(msg));
  }
}

template<typename T>
void log(T msg){
  Serial.println("--> " + String(msg));
}

template<typename T>
void serialSend(T msg){
  Serial.println("--" + String(msg));
}

template<typename T>
void serialSend(String name, T msg){
  Serial.println("--" + name + " " + String(msg));
}

template<typename T, typename T2>
void serialSend(String name, T n, T2 msg){
  Serial.println("--" + name + " " + String(n) + " "+ String(msg));
}

template<typename T, typename T2, typename T3>
void serialSend(String name, T n, T2 msg, T3 n3){
  Serial.println("--" + name + " " + String(n) + " "+ String(msg) + " " + String(n3));
}

template<typename T>
void loopDebug(T *things, uint8_t size){
  for(int i=0; i < size; i++){
    debug(things[i]);
  }
}

