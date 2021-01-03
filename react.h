#include <Reactduino.h>

class ReactionManager{
  public:
    reaction reactions[MAX_SIMUL_REACTIONS];
    int i;
    ReactionManager(){
      this->i=0;
    }
    add(reaction r){
      reactions[this->i] = r;
      this->i++;
      debug("Adding reaction");
      DEB(r);
    }
    add(reaction r[]){
      int len = sizeof(r)/sizeof(r[0]);
      for(int i=0; i<len; i++){
        this->add(r[i]);
      }
    }
    free(){
      for(int i=0; i<this->i; i++){
        app.free(reactions[i]);
      }
      debug("Freeing");
      this->i = 0;
    }
};


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

reaction bindKey(char key, react_callback cb){
  reaction Rreturn;
  switch (key){
    case 'A':
      RbindKey.a_cb = cb;
      RbindKey.a = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='A') {
        RbindKey.a_cb();
        app.free(RbindKey.a);
      }});
      Rreturn =RbindKey.a;
      break;

    case 'B':
      RbindKey.b_cb = cb;
      RbindKey.b = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='B') {
        RbindKey.b_cb();
        app.free(RbindKey.b);
      }});
      Rreturn =RbindKey.b;
      break;

    case 'C':
      RbindKey.c_cb = cb;
      RbindKey.c = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='C') {
        RbindKey.c_cb();
        app.free(RbindKey.c);
      }});
      Rreturn =RbindKey.c;
      break;

    case 'D':
      RbindKey.d_cb = cb;
      RbindKey.d = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='D') {
        RbindKey.d_cb();
        app.free(RbindKey.d);
      }});
      Rreturn =RbindKey.d;
      break;

    case '*':
      RbindKey.e_cb = cb;
      RbindKey.e = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='*') {
        RbindKey.e_cb();
        app.free(RbindKey.e);
      }});
      Rreturn =RbindKey.e;
      break;

    case '#':
      RbindKey.f_cb = cb;
      RbindKey.f = app.repeat(KEYBOARD_DELAY, [](){
      if (receiveKey()=='#') {
        RbindKey.f_cb();
        app.free(RbindKey.f);
      }});
      Rreturn =RbindKey.f;
      break;

    default:
     log("Wrong character!");
  }
  return Rreturn;
}

