
#define SSD1306_NO_SPLASH

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define FPS 25
#define JOYSTICK_PIN A3

unsigned long stamp;

// ST_MENU -> ST_HS -> ST_MENU
// ST_MENU -> ST_LSTART -> ST_LEVEL
// ST_LEVEL -> ST_GO -> ST_HS
// ST_LEVEL -> ST_LEND -> ST_GG -> ST_HS
typedef enum {
  ST_MENU,
  ST_HS,
  ST_LSTART,
  ST_LEVEL,
  ST_LEND,
  ST_GO,
  ST_GG,
} gamestate_t;
gamestate_t state = ST_MENU;

unsigned int highscore[6] = {1000, 900, 800, 700, 600, 500};

unsigned int button_state = 0;
unsigned long button_stamp = 0;
bool short_press = false;
bool long_press = false;

float x  = 0;
float y  = SCREEN_HEIGHT/2;
float dx = 0;
float dy = 0;

unsigned int progress = 0;
unsigned int score    = 0;

#define LEVEL_LEN 256
static const unsigned char level[LEVEL_LEN] = {
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x22, 0x03, 0x05, 0x26, 6, 5, 4, 5,
  6, 8, 9, 10, 10, 11, 11, 11,
  12, 13, 15, 0x2f, 0x0f, 0x3f, 0x0e, 0x2d,
  0x0d, 0x0b, 0x2c, 12, 10, 9, 10, 12,
  12, 11, 9, 5, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x27, 0x27, 0x27, 0x27,
  0x28, 0x28, 0x27, 0x27, 0x10, 0x10, 0x10, 0x10,
  // 64
  0x10, 6, 6, 5, 4, 4, 3, 4,
  4, 5, 6, 8, 9, 10, 10, 11,
  11, 12, 13, 15, 15, 14, 12, 10,
  6, 2, 0x10, 0x10, 0x10, 1, 2, 3,
  4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 14, 13, 12, 11,
  10, 9, 8, 7, 6, 5, 4, 3,
  2, 1, 0x10, 0x10, 0x10, 0x10, 0x10,
  // 128 FIXME
  0x10, 6, 6, 5, 4, 4, 3, 4,
  4, 5, 6, 8, 9, 10, 10, 11,
  11, 12, 13, 15, 15, 14, 12, 10,
  6, 2, 0x10, 0x10, 0x10, 1, 2, 3,
  4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 14, 13, 12, 11,
  10, 9, 8, 7, 6, 5, 4, 3,
  2, 1, 0x10, 0x10, 0x10, 0x10, 0x10,
  // 192 FIXME
  0x10, 6, 6, 5, 4, 4, 3, 4,
  4, 5, 6, 8, 9, 10, 10, 11,
  11, 12, 13, 15, 15, 14, 12, 10,
  6, 2, 0x10, 0x10, 0x10, 1, 2, 3,
  4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 14, 13, 12, 11,
  10, 9, 8, 7, 6, 5, 4, 3,
  2, 1, 0x10, 0x10, 0x10, 0x10, 0x10,
  // 256
};

unsigned int bird_ticks  = 5;
unsigned int bird_frames = 8;
unsigned int bird_frame  = 0;
unsigned int bird_tick   = 0;
static const unsigned char PROGMEM right_bird[8][8] = {
  {
    0b00000000,
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00001100,
    0b00000100,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00000100,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00011000,
    0b01111110,
    0b00001100,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00110000,
    0b00011100,
    0b00111110,
    0b01011000,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00110000,
    0b00011100,
    0b00111110,
    0b01011000,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00011000,
    0b01111110,
    0b00001100,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00000100,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00001100,
    0b00000100,
    0b00000000,
  },
};
/*
static const unsigned char PROGMEM left_bird[8][8] = {
  {
    0b00000000,
    0b00000000,
    0b00000010,
    0b01111110,
    0b00111100,
    0b00110000,
    0b00100000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00000010,
    0b01111110,
    0b00111100,
    0b00100000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00011000,
    0b01111110,
    0b00110000,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00001100,
    0b00111000,
    0b01111100,
    0b00011010,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00001100,
    0b00111000,
    0b01111100,
    0b00011010,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00011000,
    0b01111110,
    0b00110000,
    0b00000000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00000010,
    0b01111110,
    0b00111100,
    0b00100000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00000000,
    0b00000010,
    0b01111110,
    0b00111100,
    0b00110000,
    0b00100000,
    0b00000000,
  },
};
*/

static const unsigned char PROGMEM cloud_left[9] = {
  0b00000011,
  0b00001111,
  0b00001111,
  0b00110111,
  0b01111111,
  0b11111111,
  0b11111111,
  0b01111111,
  0b00011111,
};

static const unsigned char PROGMEM lv_water[3] = {
  0b00010000,
  0b00101000,
  0b11000111,
};
static const unsigned char PROGMEM lv_grass[6] = {
  0b01000000,
  0b10100000,
  0b00100100,
  0b10101000,
  0b01111010,
  0b01111100,
};

/*
typedef struct level_object_t {
  unsigned char  x;
  unsigned char PROGMEM *bitmap;
};

static const level_object_t object[2] = {
  {
    3,
    lv_water,
  },
  {
    4,
    lv_grass,
  },
};
*/
void draw_screen ()
{
    display.clearDisplay();

    // bird
    display.drawBitmap(int(x), int(SCREEN_HEIGHT-y), right_bird[bird_frame], 8, 8, 1);

    // ground
    for (unsigned int i = 0; i <= int(SCREEN_WIDTH/8); i++) {
      unsigned char lv  = level[int(progress/8)+i];
      unsigned char obj = lv >> 4;
      lv = lv & 0x0F;

      if (lv)
        display.fillRect(i*8-progress%8, SCREEN_HEIGHT-lv, 8, lv, 1);
      if (obj > 0 && obj <= 2) {
        const static unsigned char *bmp;
        unsigned char bmx;
        switch (obj) {
          case 1:  bmp = lv_water;  bmx = 3;
            break;
          case 2:  bmp = lv_grass;  bmx = 6;
            break;
        }
        display.drawBitmap(i*8-progress%8, SCREEN_HEIGHT-lv-bmx, bmp, 8, bmx, 1);
      }
    }

    // score
    char buf[16];
    display.fillRect(SCREEN_WIDTH-26, 0, 26, 7, 0);
    display.drawBitmap(SCREEN_WIDTH-26-8, 0, cloud_left, 8, 9, 1);
    display.drawLine(SCREEN_WIDTH-26, 8, SCREEN_WIDTH, 8, 1);
    display.setCursor(SCREEN_WIDTH-24, 0);
    sprintf(buf, "%04d", score);
    display.println(buf);

    // draw
    display.display();
}

void do_bird_tick ()
{
  bird_tick += 1;

  if (bird_tick > bird_ticks) {
    bird_tick = 0;
    bird_frame += 1;
    if (bird_frame >= 8)
      bird_frame = 0;
  }
}

void setup()
{
  // serial console
  Serial.begin(9600);

  // input button
  pinMode(JOYSTICK_PIN, INPUT);

  // display
  delay(500);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display allocation failed");
    for(;;);
  }

  display.setTextSize(1);
  display.setTextColor(1);

  state = ST_MENU;
}

void loop ()
{

  // process input
  int cmd = digitalRead(JOYSTICK_PIN);
  if (cmd != button_state) {
    button_state = cmd;
    if (!button_state) {
      Serial.print("Buttonpress for ");
      Serial.println(millis() - button_stamp);
      if (millis() - button_stamp > 200) {
        Serial.println("longpress");
        long_press = true;
      } else {
        Serial.println("shortpress");
        short_press = true;
      }
    }
    button_stamp = millis();
  }


  // gametick
  if (millis() - stamp > 1000 / FPS) {

    if (state == ST_MENU) {
      
      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-10, 1);
      display.setCursor(int(SCREEN_WIDTH/2)-30, int(SCREEN_HEIGHT/2)-12);
      display.setTextSize(2);
      display.println("START");
      display.setTextSize(1);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("short: next  long: ok");
      display.display();

      if (short_press) {
        state = ST_HS;
      }
      if (long_press) {
        state = ST_LSTART;
        x = 0;
        y = SCREEN_HEIGHT/2;
      }

    } else if (state == ST_HS) {

      unsigned char lv  = level[5];
      unsigned char obj = lv & 0xF0;
      lv = lv & 0x0F;
      Serial.print("lv[5] = ");
      Serial.print(lv);
      Serial.print(", obj[5] = ");
      Serial.print(obj);

      display.clearDisplay();
      for (int s = 0; s < 6; s++) {
        char buf[16];
        display.setCursor(0+int(s/3)*(SCREEN_WIDTH/2), 0+8*(s%3));
        sprintf(buf, "%d: %04d", s+1, highscore[s]);
        display.println(buf);

      }
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("short: next");
      display.display();

      if (short_press) {
        state = ST_MENU;
      }

    } else if (state == ST_LSTART) {

      dx       = 1;
      dy       = 0;
      progress = 0;
      score    = 0;

      x += dx;
      y += dy;

      do_bird_tick();

      if (x >= 16) {
        state = ST_LEVEL;
      }

      draw_screen();
      return;

    } else if (state == ST_LEVEL) {
      dx = 0;

      dy = -1;
      if (button_state > 0) {
        Serial.println("flap");
        dy = 1;
      }

      x += dx;
      y += dy;
      progress += 1;
      score    += 1;

      do_bird_tick();

      if (progress >= LEVEL_LEN*8 - SCREEN_WIDTH) {
        state = ST_LEND;
      }


      if (x > SCREEN_WIDTH - 8) {
        x  = SCREEN_WIDTH-8;
        dx = 0;
      }
      if (x < 0) {
        x = 0;
        dx = 0;
      }
      if (y > SCREEN_HEIGHT) {
        y = SCREEN_HEIGHT;
        dy = 0;
      }
      if (y < 8) {
        y = 8;
        dy = 0;
      }

      draw_screen();
    } else if (state == ST_LEND) {
      dx = 1;
      dy = 0;
      progress = LEVEL_LEN*8-SCREEN_WIDTH;

      // level-off our bird
      if (y - SCREEN_HEIGHT/2 > 0) {
        dy = -1;
      } else if (y - SCREEN_HEIGHT/2 < 0) {
        dy = 1;
      }

      x += dx;
      y += dy;

      do_bird_tick();

      if (x >= SCREEN_WIDTH-8) {
        state = ST_GG;
      }

      draw_screen();
      return;
    } else if (state == ST_GO) {
      // FIXME
      state = ST_HS;
      return;
    } else if (state == ST_GG) {
      // FIXME
      state = ST_HS;
      return;
    }
        
    short_press = false;
    long_press  = false;
    stamp = millis();
  }
}

