#include <Reactduino.h>

char lastKey;
char receiveKey(){
  char key = keypad.getKey();
  if(lastKey == key or key == ' ')
    return '-';
  lastKey = key;
  return key;
}

struct RbindKey_s{
  reaction a;
  react_callback a_cb;
  reaction b;
  react_callback b_cb;
  reaction c;
  react_callback c_cb;
  reaction d;
  react_callback d_cb;
  reaction e;
  react_callback e_cb;
  reaction f;
  react_callback f_cb;
};
RbindKey_s RbindKey;

void bindKey(char key, react_callback cb){
  switch (key){
    case 'A':
      RbindKey.a_cb = cb;
      RbindKey.a = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='A') {
        RbindKey.a_cb();
        app.free(RbindKey.a);
      }});
      break;

    case 'B':
      RbindKey.b_cb = cb;
      RbindKey.b = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='B') {
        RbindKey.b_cb();
        app.free(RbindKey.b);
      }});
      break;

    case 'C':
      RbindKey.c_cb = cb;
      RbindKey.c = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='C') {
        RbindKey.c_cb();
        app.free(RbindKey.c);
      }});
      break;

    case 'D':
      RbindKey.d_cb = cb;
      RbindKey.d = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='D') {
        RbindKey.d_cb();
        app.free(RbindKey.d);
      }});
      break;

    case '*':
      RbindKey.e_cb = cb;
      RbindKey.e = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='*') {
        RbindKey.e_cb();
        app.free(RbindKey.e);
      }});
      break;

    case '#':
      RbindKey.f_cb = cb;
      RbindKey.f = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='#') {
        RbindKey.f_cb();
        app.free(RbindKey.f);
      }});
      break;

    default:
     log("Wrong character!");
  }
}

