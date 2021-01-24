#define FONT_RATIO 2
#include <U8g2lib.h>
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


class LabelWidget{

public:
  Cursor &cursor;
  U8G2 *screen;
  const uint8_t *font;
  String text;

  LabelWidget(Cursor &cursor, U8G2 *screen, const uint8_t *font): cursor(cursor), screen(screen), font(font){}
  
  void write(String msg){
    int str_len = msg.length() + 1; 
    screen->setFont(this->font);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
    char char_array[str_len];
    msg.toCharArray(char_array, str_len);
    screen->drawStr(this->cursor.x, this->cursor.y, char_array);	// write something to the internal memory
    screen->sendBuffer();					// transfer internal memory to the display
    this->text += msg;
  }

  void print(String msg){
    screen->clearBuffer();
    this->write(msg);
  }

  void clear() {
    this->screen->clear();
    this->text="";
  }

};



