/* TODO 
 * Best lap
 * Fuel bar 
 * fuel logic
 * Menu with pit stop and failure
 * pit stop detection
 * refuel
 * failure
*/

// PINS
#define LAPP1 2
#define LAPP2 3
#define P1LED 12
#define P2LED 11
#define BUZZER2 9
#define BUZZER 8
const int SEMA1[3] = {45, 47, 49};
const int SEMA2[3] = {48, 46, 44};


// Constants
#define MAX_LAPS 255
#define BITS_LAPS 8
#define DEFAULT_LAPS "4"

#define DEBUG true
#define BLINK_TIME 100
#define BLINK_TIME_SLOW 100
#define N_PINS 24
#define DEFAULT_BLINK_DURATION 2000
#define KEYBOARD_DELAY 20
#define KEYBOARD_RESET_DELAY 1500
#define TEST_DELAY 500
#define MIN_LAP_TIME 600.0
#define RESET_TIME 3000

#define FONT_BIG u8g2_font_logisoso18_tf
#define FONT_MATRIX u8g2_font_trixel_square_tn
#define FONT_SMALL  u8g2_font_7x13B_tf
#define FONT_RATIO 4

#define MATRIX_Y_CENTER 7
#define MATRIX_ANIMATION_PAUSE 2
#define MATRIX_ANIMATION_DELAY 300

#define TEST_START 0
#define LAP_START -2 //-1

#define MAX_SIMUL_REACTIONS 8
#define ONE 1
#define _THOUSAND 1000

// Macros
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


void debug(String msg) {
  if (DEBUG) {
    Serial.println(String("DEBUG: ") + msg);
  }
}

void log(String msg){
  Serial.println("--> " + msg);
}
