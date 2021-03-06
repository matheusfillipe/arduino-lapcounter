reaction matrix1_print_reaction;
reaction matrix2_print_reaction;

void(* resetProgram) (void) = 0;

const char* str2char(String str){
  int str_len = str.length() + 1; 
  char char_array[str_len];
  return char_array;
}

void iwrite(String msg, U8G2 *screen, Cursor &cursor = screen1_cursor, const uint8_t *font = FONT_SMALL){
  int str_len = msg.length() + 1; 
  u8g2_uint_t char_width = screen->getMaxCharWidth();
  screen->setFont(font);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
  char char_array[str_len];
  msg.toCharArray(char_array, str_len);
  screen->drawStr(cursor.x, cursor.y, char_array);	// write something to the internal memory
}

void write(String msg, U8G2 *screen, Cursor &cursor = screen1_cursor, const uint8_t *font = FONT_SMALL){
  iwrite(msg, screen, cursor, font);
  screen->sendBuffer();					// transfer internal memory to the display
}

void iwrite(String msg, LabelWidget output) {
  iwrite(msg, output.screen, output.cursor, output.font);
}

void write(String msg, LabelWidget output) {
  write(msg, output.screen, output.cursor, output.font);
}

void print(String msg, U8G2 *screen, Cursor &cursor = screen1_cursor, const uint8_t *font = FONT_SMALL) {
  screen->clearBuffer();
  write(msg, screen, cursor, font);
}

void print(String msg, LabelWidget &output) {
  print(msg, output.screen, output.cursor, output.font);
}

int Gfreq, Gtime;
void tone(int freq, int time){
  if(!mute && !MUTE){
    Gfreq = freq;
    Gtime = time;
    analogWrite(A0, 255);
    tone(BUZZER, Gfreq, Gtime);
    app.delay(time, REACT(analogWrite(A0, 0)));
  }
  if(mute || MUTE){
    analogWrite(A0, 0);
  }
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
  matrix1_cursor.y=MATRIX_Y_CENTER;
  matrix2_cursor.x=0;
  matrix2_cursor.y=MATRIX_Y_CENTER;
  print(n, &matrix1, matrix1_cursor, FONT_MATRIX);
  print(n, &matrix2, matrix2_cursor, FONT_MATRIX);
}

void mirrowedPrint(String msg){
  static bool toggle = true;
  screen1_cursor.x=30;
  screen1_cursor.y=40;
  screen2_cursor.x=30;
  screen2_cursor.y=40;

  int str_len = msg.length() + 1; 
  char char_array[str_len];
  msg.toCharArray(char_array, str_len);
  screen1.setFont(FONT_BIG);  
  screen2.setFont(FONT_BIG);  
  screen1.clearBuffer();
  screen2.clearBuffer();
  screen1.drawStr(screen1_cursor.x, screen1_cursor.y, char_array);	
  screen2.drawStr(screen2_cursor.x, screen2_cursor.y, char_array);	
  if(toggle){
    screen1.updateDisplayArea(3, 2, 12, 4);
    screen2.updateDisplayArea(3, 2, 12, 4);
    // screen1.sendBuffer();					
    // screen2.sendBuffer();					
  }
  else{
    screen1.updateDisplayArea(3, 2, 12, 4);
    screen2.updateDisplayArea(3, 2, 12, 4);
    // screen2.sendBuffer();					
    // screen1.sendBuffer();					
  }
  toggle = !toggle;
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
    analogWrite(sema[i], 255);
  }
}

int mstart1;
int mstart2;

bool matrix1_print(int num);
bool matrix2_print(int num);

react_callback inc_matrix1(int start = 0){
  mstart1 = start;
  return [](){
      static int count = mstart1;
      static bool animating = false;
      if(count >= MAX_LAPS)
        count = 0;
      if (animating)
        app.free(matrix1_print_reaction);
      animating = matrix1_print(count);
      count++;
  };
}

react_callback inc_matrix2(int start = 0){
  mstart2 = start;
  return [](){
      static int count = mstart2;
      static bool animating = false;
      if(count >= MAX_LAPS)
        count = 0;
      if (animating)
        app.free(matrix2_print_reaction);
      animating = matrix2_print(count);
      count++;
  };
}

String matrix1_number;
bool matrix1_print(int num){ 
  matrix1_number = String(num);
  String msg = String(num);
  matrix1.setFont(FONT_MATRIX);
  const int w = matrix1.getStrWidth(msg.c_str());
  const int W = matrix1.getDisplayWidth();
  const bool repeat = w > W;
  if(!repeat){ 
    matrix1_cursor.x = 0;
    matrix1_cursor.y = MATRIX_Y_CENTER;
    print(msg, &matrix1, matrix1_cursor, FONT_MATRIX);
    return false;
  }

  matrix1_print_reaction = app.repeat(MATRIX_ANIMATION_DELAY, [](){       
      static const int w = matrix1.getStrWidth(matrix1_number.c_str());
      static const int W = matrix1.getDisplayWidth();
      static int xOffset = 0;
      static int delay = 0;
      static bool firstTime = true;

      if (!firstTime && delay < MATRIX_ANIMATION_PAUSE) {
        delay++;
        return;
      }
      matrix1_cursor.x = 0 - xOffset;
      matrix1_cursor.y = MATRIX_Y_CENTER;
      print(matrix1_number, &matrix1, matrix1_cursor, FONT_MATRIX);

      if (!firstTime && xOffset == 0){
        delay = 0;
      }
      xOffset = w-xOffset+MATRIX_ANIMATION_END_XOFFSET <= W ? 0 : xOffset + 1;
      delay = xOffset == 0 ? 0 : delay;
      firstTime = false;
  });
  return repeat;
}


String matrix2_number;
bool matrix2_print(int num){ 
  matrix2_number = String(num);
  String msg = String(num);
  matrix2.setFont(FONT_MATRIX);
  const int w = matrix2.getStrWidth(msg.c_str());
  const int W = matrix2.getDisplayWidth();
  const bool repeat = w > W;
  if(!repeat){ 
    matrix2_cursor.x = 0;
    matrix2_cursor.y = MATRIX_Y_CENTER;
    print(msg, &matrix2, matrix2_cursor, FONT_MATRIX);
    return false;
  }
  
  matrix2_print_reaction = app.repeat(MATRIX_ANIMATION_DELAY, [](){       
      static const int w = matrix2.getStrWidth(matrix2_number.c_str());
      static const int W = matrix2.getDisplayWidth();
      static int xOffset = 0;
      static int delay = 0;
      static bool firstTime = true;

      if (!firstTime && delay < MATRIX_ANIMATION_PAUSE) {
        delay++;
        return;
      }
      matrix2_cursor.x = 0 - xOffset;
      matrix2_cursor.y = MATRIX_Y_CENTER;
      print(matrix2_number, &matrix2, matrix2_cursor, FONT_MATRIX);

      if (!firstTime && xOffset == 0){
        delay = 0;
      }
      xOffset = w-xOffset+MATRIX_ANIMATION_END_XOFFSET <= W ? 0 : xOffset + 1;
      delay = xOffset == 0 ? 0 : delay;
      firstTime = false;
  });
  return repeat;
}

String timestamp(unsigned long dt){
    String sec = String(dt / _THOUSAND); 
    String ms = String(dt % _THOUSAND); 
    if (ms.length() == 0)
      ms = "000";
    if (ms.length() == 1)
      ms += "00";
    if (ms.length() == 2)
      ms += "0";
    return String(sec+"."+ms);
}

void generateRandomNumbers(int &laps, int opt, int n_laps){
  if(opt)
    laps = random(1, n_laps-1);
  else
    laps = -1;
}
