const char* str2char(String str){
  int str_len = str.length() + 1; 
  char char_array[str_len];
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
  matrix1_cursor.y=MATRIX_Y_CENTER;
  matrix2_cursor.x=0;
  matrix2_cursor.y=MATRIX_Y_CENTER;
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

int mstart1;
int mstart2;

void matrix1_print(int num);
void matrix2_print(int num);

react_callback inc_matrix1(int start = 0){
  mstart1 = start;
  return [](){
      static int count = mstart1;
      if(count >= MAX_LAPS)
        count = 0;
      matrix1_print(count);
      count++;
  };
}

react_callback inc_matrix2(int start = 0){
  mstart2 = start;
  return [](){
      static int count = mstart2;
      if(count >= MAX_LAPS)
        count = 0;
      matrix2_print(count);
      count++;
  };
}



reaction matrix1_print_reaction;
String matrix1_number;
void matrix1_print(int num){ 
  matrix1_number = num;
  String msg = String(num);
  matrix1.setFont(FONT_MATRIX);
  const int w = matrix1.getStrWidth(msg.c_str());
  const int W = matrix1.getDisplayWidth();
  const bool repeat = w > W;
  if(!repeat){ 
    matrix1_cursor.x = 0;
    matrix1_cursor.y = MATRIX_Y_CENTER;
    print(msg, &matrix1, matrix1_cursor, FONT_MATRIX);
    return;
  }
  // app.free(matrix1_print_reaction);
  rman.add(matrix1_print_reaction);
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
      xOffset = w-xOffset <= W ? 0 : xOffset + 1;
      delay = xOffset == 0 ? 0 : delay;
      firstTime = false;
  });
}


reaction matrix2_print_reaction;
String matrix2_number;
void matrix2_print(int num){ 
  matrix2_number = num;
  String msg = String(num);
  matrix2.setFont(FONT_MATRIX);
  const int w = matrix2.getStrWidth(msg.c_str());
  const int W = matrix2.getDisplayWidth();
  const bool repeat = w > W;
  if(!repeat){ 
    matrix2_cursor.x = 0;
    matrix2_cursor.y = MATRIX_Y_CENTER;
    print(msg, &matrix2, matrix2_cursor, FONT_MATRIX);
    return;
  }
  // app.free(matrix2_print_reaction);
  rman.add(matrix2_print_reaction);
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
      xOffset = w-xOffset <= W ? 0 : xOffset + 1;
      delay = xOffset == 0 ? 0 : delay;
      firstTime = false;
  });
}
