
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
#define MAX_ENERGY (SCREEN_HEIGHT-2)

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
float y  = 0;
float dx = 0;
float dy = 0;
float energy = MAX_ENERGY;

unsigned int progress = 0;
unsigned int score    = 0;

#define EBAR_WIDTH 5
#define LEVEL_SWIDTH (SCREEN_WIDTH-EBAR_WIDTH)
#define LEVEL_LEN 256
static const unsigned char level[LEVEL_LEN] = {
  0x30, 0x21, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x22, 0x03, 0x05, 0x26, 0x06, 0x05, 0x04, 0x05,
  0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b,
  0x0c, 0x0d, 0x0f, 0x2f, 0x0f, 0x2f, 0x0e, 0x2d,
  0x0d, 0x0b, 0x2c, 0x0c, 0x0a, 0x09, 0x0a, 0x0c,
  0x0c, 0x0b, 0x09, 0x05, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x27, 0x07, 0x27, 0x07,
  0x28, 0x08, 0x27, 0x07, 0x10, 0x10, 0x10, 0x10,
  // 64
  0x10, 0x06, 0x06, 0x05, 0x04, 0x04, 0x03, 0x04,
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  // 128 FIXME
  0x10, 0x06, 0x06, 0x05, 0x04, 0x04, 0x03, 0x04,
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  // 192 FIXME
  0x10, 0x06, 0x06, 0x05, 0x04, 0x04, 0x03, 0x04,
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x01, 0x21, 0x40,
  // 256
};

#define BIRD_START_X 5
#define BIRD_START_Y (SCREEN_HEIGHT/2)

unsigned int bird_ticks  = 5;
unsigned int bird_frames = 8;
unsigned int bird_frame  = 0;
unsigned int bird_tick   = 0;
static const unsigned char PROGMEM right_bird[8][6] = {
  {
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00001100,
    0b00000100,
  },
  {
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00000100,
    0b00000000,
  },
  {
    0b00011000,
    0b01111110,
    0b00001100,
    0b00000000,
    0b00000000,
  },
  {
    0b00110000,
    0b00011100,
    0b00111110,
    0b01011000,
    0b00000000,
    0b00000000,
  },
  {
    0b00110000,
    0b00011100,
    0b00111110,
    0b01011000,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b00011000,
    0b01111110,
    0b00001100,
    0b00000000,
    0b00000000,
  },
  {
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00000100,
    0b00000000,
  },
  {
    0b00000000,
    0b01000000,
    0b01111110,
    0b00111100,
    0b00001100,
    0b00000100,
  },
};

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
static const unsigned char PROGMEM lv_nest_right[32] = {
  0b01111000,
  0b11111110,
  0b01111111,
  0b10111111,
  0b10111110,
  0b01111000,
  0b11010110,
  0b00101111,

  0b11010111,
  0b10101110,
  0b11100000,
  0b11000000,
  0b11000000,
  0b10000000,
  0b10000000,
  0b10000000,

  0b10000000,
  0b10000000,
  0b10000000,
  0b10010001,
  0b10011111,
  0b10001111,
  0b11111000,
  0b11000000,

  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b11000000,
  0b11110000,
  0b11011100,
  0b11100111,
};

static const unsigned char PROGMEM lv_nest_left[16] = {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00100010,
  0b10111110,
  0b01100010,
  0b01111010,
  0b01110110,

  0b01010110,
  0b01011110,
  0b01110110,
  0b01101110,
  0b01101110,
  0b01110110,
  0b01110110,
  0b11011011,
};

void draw_screen ()
{
    display.clearDisplay();

    // bird
    display.drawBitmap(EBAR_WIDTH+int(x)-4, int(SCREEN_HEIGHT-y)-3, right_bird[bird_frame], 8, 6, 1);

    // ground
    for (unsigned int i = 0; i <= int(LEVEL_SWIDTH/8)+1; i++) {
      unsigned char lv  = level[int(progress/8)+i];
      unsigned char obj = lv >> 4;
      lv = lv & 0x0F;

      if (lv)
        display.fillRect(EBAR_WIDTH+i*8-progress%8, SCREEN_HEIGHT-lv, 8, lv, 1);
      if (obj > 0 && obj <= 4) {
        const static unsigned char *bmp;
        unsigned char bmy;
        switch (obj) {
          case 1:  bmp = lv_water;  bmy = 3;
            break;
          case 2:  bmp = lv_grass;  bmy = 6;
            break;
          case 3:  bmp = lv_nest_right;  bmy = 32;
            break;
          case 4:  bmp = lv_nest_left;   bmy = 16;
            break;
        }
        display.drawBitmap(EBAR_WIDTH+i*8-progress%8, SCREEN_HEIGHT-lv-bmy, bmp, 8, bmy, 1);
      }
    }

    // energy
    display.fillRect(0, 0, 4, SCREEN_HEIGHT, 1);
    display.fillRect(1, 1, 2, MAX_ENERGY-int(energy), 0);
    display.drawLine(4, 0, 4, SCREEN_HEIGHT, 0);

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
        state    = ST_LSTART;
        x        = BIRD_START_X;
        y        = BIRD_START_Y;
        dx       = 0;
        dy       = 0;
        score    = 0;
        progress = 0;
      }

    } else if (state == ST_HS) {

      unsigned char lv  = level[5];
      unsigned char obj = lv & 0xF0;
      lv = lv & 0x0F;

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

      if (x >= 20) {
        state = ST_LEVEL;
      }

      draw_screen();

    } else if (state == ST_LEVEL) {

      dx = 0;
      dy = -1;

      if (button_state > 0) {
        if (energy > 0) {
          dy = 1;
          energy -= 0.5;
        }
      } else {
        if (energy < MAX_ENERGY) {
          energy += 0.5;
        }
      }

      x        += dx;
      y        += dy;
      progress += 1;
      score    += 1;

      do_bird_tick();

      if (progress >= LEVEL_LEN*8 - LEVEL_SWIDTH) {
        progress = LEVEL_LEN*8-LEVEL_SWIDTH;
        state    = ST_LEND;
      }


      if (x > LEVEL_SWIDTH - 4) {
        x  = LEVEL_SWIDTH-4;
        dx = 0;
      }
      if (x < 4) {
        x  = 4;
        dx = 0;
      }
      if (y > SCREEN_HEIGHT-3) {
        y  = SCREEN_HEIGHT-3;
        dy = 0;
      }
      if (y < 3) {
        y  = 3;
        dy = 0;
      }

      draw_screen();
    } else if (state == ST_LEND) {
      dx = 1;
      dy = 0;

      // level-off our bird
      if (y - BIRD_START_Y > 0) {
        dy = -1;
      } else if (y - BIRD_START_Y < 0) {
        dy = 1;
      }

      x += dx;
      y += dy;

      do_bird_tick();

      if (x >= LEVEL_SWIDTH-4) {
        state = ST_GG;
      }

      draw_screen();
    } else if (state == ST_GO) {
      char buf[16];
      unsigned int pos;

      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-8, 1);
      display.setCursor(int(SCREEN_WIDTH/2)-30, 4);
      display.println("GAME OVER");

      for (pos = 0; pos <6; pos++) {
        if (highscore[pos] < score) {
          break;
        }
      }
      sprintf(buf, "score: #%d %04d", pos+1, score);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 14);
      display.println(buf);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("short: next");
      display.display();

      if (short_press) {
        if (pos < 6) {
          for (int i = 5; i > pos; i--)
            highscore[i] = highscore[i-1];
          highscore[pos] = score;
        }
        state = ST_HS;
      }

    } else if (state == ST_GG) {
      char buf[16];
      unsigned int pos;

      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-8, 1);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 4);
      display.println("LEVEL CLEAR");

      for (pos = 0; pos <6; pos++) {
        if (highscore[pos] < score) {
          break;
        }
      }
      sprintf(buf, "score: #%d %04d", pos+1, score);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 14);
      display.println(buf);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("short: next");
      display.display();

      if (short_press) {
        if (pos < 6) {
          for (int i = 5; i > pos; i--)
            highscore[i] = highscore[i-1];
          highscore[pos] = score;
        }
        state = ST_HS;
      }

    } // state switch
        
    short_press = false;
    long_press  = false;
    stamp = millis();
  }
}

