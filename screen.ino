
#define SSD1306_NO_SPLASH

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define JOYSTICK_PIN A3
#define DISPLAY_POWER_PIN PD2

#define FPS 25
#define MAX_ENERGY (SCREEN_HEIGHT-2)
#define SLEEP_TIMEOUT 30

unsigned long stamp;

// ST_MENU -> ST_HS -> ST_GM -> ST_MENU
// ST_MENU -> ST_LSTART -> ST_LEVEL
// ST_LEVEL -> ST_GO -> ST_HS
// ST_LEVEL -> ST_LEND -> ST_GG -> ST_HS
// (ST_MENU | ST_HS | ST_GM | ST_GO | ST_GG) <-> ST_SLEEP
typedef enum {
  ST_MENU,
  ST_HS,
  ST_GM,
  ST_SLEEP,
  ST_LSTART,
  ST_LEVEL,
  ST_LEND,
  ST_GO,
  ST_GG,
} gamestate_t;
gamestate_t state          = ST_MENU;
gamestate_t presleep_state = ST_MENU;

#define DEFAULT_CONFIG {0, {1000, 900, 800, 700, 600, 500}}
typedef struct gamedata_t {
  bool godmode;
  unsigned int highscore[6];
};

gamedata_t config = DEFAULT_CONFIG;

unsigned int button_state = 0;
unsigned long button_stamp = 0;
bool short_press     = false;
bool long_press      = false;
bool input_inactive  = false;

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
  0x30, 0x21, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, // 0
  0x22, 0x03, 0x05, 0x26, 0x06, 0x05, 0x04, 0x05,
  0x56, 0x58, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b,
  0x0c, 0x0d, 0x0f, 0x2f, 0x0f, 0x2f, 0x0e, 0x2d,
  0x0d, 0x0b, 0x2c, 0x0c, 0x0a, 0x09, 0x0a, 0x0c,
  0x0c, 0x0b, 0x09, 0x05, 0x61, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x27, 0x07, 0x27, 0x07,
  0x28, 0x08, 0x27, 0x07, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x06, 0x66, 0x05, 0x04, 0x04, 0x53, 0x04, // 64
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x06, 0x06, 0x05, 0x04, 0x04, 0x03, 0x04, // 128
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x06, 0x06, 0x05, 0x04, 0x04, 0x03, 0x04, // 192
  0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
  0x0b, 0x0c, 0x0d, 0x0f, 0x0f, 0x0e, 0x0c, 0x0a,
  0x06, 0x02, 0x10, 0x10, 0x10, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b,
  0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
  0x02, 0x01, 0x10, 0x10, 0x10, 0x01, 0x21, 0x40, // 256
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

//
//  Level objects
//

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
  0b11010111, // 8
  0b10101110,
  0b11100000,
  0b11000000,
  0b11000000,
  0b10000000,
  0b10000000,
  0b10000000,
  0b10000000, // 16
  0b10000000,
  0b10000000,
  0b10010001,
  0b10011111,
  0b10001111,
  0b11111000,
  0b11000000,
  0b10000000, // 24
  0b10000000,
  0b10000000,
  0b10000000,
  0b11000000,
  0b11110000,
  0b11011100,
  0b11100111, // 32
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
  0b01010110, // 8
  0b01011110,
  0b01110110,
  0b01101110,
  0b01101110,
  0b01110110,
  0b01110110,
  0b11011011, // 16
};
static const unsigned char PROGMEM lv_updraft[4][16] = {
  {
    0b01000001,
    0b01000001,
    0b00000000,
    0b00010000,
    0b00010000,
    0b00000000,
    0b00000100,
    0b10000100,
    0b10000000, // 8
    0b00000000,
    0b00001000,
    0b00000000,
    0b00011010,
    0b11100111,
    0b01011010,
    0b00011000, // 16
  },
  {
    0b00010000,
    0b00010000,
    0b00000000,
    0b00000100,
    0b10000100,
    0b10000000,
    0b00000000,
    0b00001000,
    0b01000001, // 8
    0b01000001,
    0b00000000,
    0b00000000,
    0b00011100,
    0b01100110,
    0b00111000,
    0b00011000, // 16
  },
  {
    0b00000100,
    0b10000100,
    0b10000000,
    0b00000000,
    0b00001000,
    0b01000001,
    0b01000001,
    0b00000000,
    0b00010000, // 8
    0b00010000,
    0b00000000,
    0b00000000,
    0b01011000,
    0b11100111,
    0b00011010,
    0b00011000, // 16
  },
  {
    0b00000000,
    0b00001000,
    0b01000001,
    0b01000001,
    0b00000000,
    0b00010000,
    0b00010000,
    0b00000000,
    0b00000100, // 8
    0b10000100,
    0b10000000,
    0b00000000,
    0b00111000,
    0b01100110,
    0b01011100,
    0b00011000, // 16
  },
};
static const unsigned char PROGMEM lv_balloon[12] = {
  0b00111100,
  0b11011011,
  0b11011011,
  0b11011011,
  0b11100111,
  0b10011001,
  0b10011001,
  0b01000010, 
  0b01000010, // 8
  0b00111100,
  0b00111100,
  0b00111100,
};

#define LV_OBJECTS 6

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
      if (obj > 0 && obj <= LV_OBJECTS) {
        const static unsigned char *bmp;
        unsigned char bmh;
        unsigned char bmy = lv;
        switch (obj) {
          case 1:  bmp = lv_water;                   bmh = 3;   break;
          case 2:  bmp = lv_grass;                   bmh = 6;   break;
          case 3:  bmp = lv_nest_right;              bmh = 32;  break;
          case 4:  bmp = lv_nest_left;               bmh = 16;  break;
          case 5:  bmp = lv_updraft[progress/2%4];   bmh = 16;  break;
          case 6:  bmp = lv_balloon;                 bmh = 12;  bmy = SCREEN_HEIGHT-bmh-bool(progress%16>7);  break;
        }
        display.drawBitmap(EBAR_WIDTH+i*8-progress%8, SCREEN_HEIGHT-bmy-bmh, bmp, 8, bmh, 1);
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

bool load_scores(gamedata_t *target)
{
  unsigned char version = EEPROM.read(0);

  if (version == 2) {
    Serial.println("Loading v2 data from EEPROM");
    for (int i = 0; i < sizeof(gamedata_t); i++)
      ((byte *)target)[i] = EEPROM.read(1+i);

  } else if (version == 1) {
    Serial.println("Loading v1 data from EEPROM");
    for (int i = 0; i < sizeof(target->highscore); i++)
      ((byte *)(target->highscore))[i] = EEPROM.read(1+i);
  
  } else {
    Serial.println("Data were not yet stored in EEPROM");
    return false;
  }

  return true;
}

void add_score(unsigned int new_hs)
{
  for (int i = 5; i >= 0; i--)
    if (new_hs < config.highscore[i])
      break;
    else {
      if (i < 5)
        config.highscore[i+1] = config.highscore[i];
      config.highscore[i] = new_hs;
    }
}

void write_scores()
{
  Serial.println("Writing data v2 to EEPROM");
  for (int i = 0; i < sizeof(gamedata_t); i++)
    EEPROM.write(1+i, ((byte *)&config)[i]);
  EEPROM.write(0, 2); // version
}

bool scores_are_in_sync()
{
  gamedata_t stored = DEFAULT_CONFIG;

  load_scores(&stored);

  for (int i = 0; i < 6; i++)
    if (config.highscore[i] != stored.highscore[i])
      return false;

  return true;
}

void display_on()
{
  Serial.println("Initializing display");
  pinMode(DISPLAY_POWER_PIN, OUTPUT);
  digitalWrite(DISPLAY_POWER_PIN, HIGH);

  delay(500);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display allocation failed");
    for(;;);
  }

  display.setTextSize(1);
  display.setTextColor(1);
}

void display_off()
{
  Serial.println("Inactivity, turning display off");
  digitalWrite(DISPLAY_POWER_PIN, LOW);
}

void setup()
{
  Serial.begin(9600);

  pinMode(JOYSTICK_PIN, INPUT);

  display_on();

  load_scores(&config);

  state = ST_MENU;
}

void loop ()
{

  // process input
  int cmd = digitalRead(JOYSTICK_PIN);
  if (cmd != button_state) {
    button_state = cmd;
    input_inactive = false;
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
  } else if (!button_state && millis() - button_stamp > SLEEP_TIMEOUT*1000) {
    input_inactive = true;
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
      display.println("long: play  short: ->");
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
      if (input_inactive) {
        presleep_state = state;
        state          = ST_SLEEP;
      }

    } else if (state == ST_HS) {

      bool need_save = !scores_are_in_sync();

      display.clearDisplay();
      for (int s = 0; s < 6; s++) {
        char buf[16];
        display.setCursor(0+int(s/3)*(SCREEN_WIDTH/2), 0+8*(s%3));
        sprintf(buf, "%d: %04d", s+1, config.highscore[s]);
        display.println(buf);

      }
      display.setCursor(0, SCREEN_HEIGHT-8);
      if (need_save)
        display.println("long: save  short: ->");
      else
        display.println("[v] saved   short: ->");
      display.display();

      if (short_press) {
        state = ST_GM;
      }
      if (need_save && long_press) {
        write_scores();
      }
      if (input_inactive) {
        presleep_state = state;
        state          = ST_SLEEP;
      }

    } else if (state == ST_GM) {

      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-10, 1);
      display.setCursor(4, int(SCREEN_HEIGHT/2)-12);
      display.setTextSize(2);
      if (config.godmode) {
        display.println("GODMODE: +");
      } else {
        display.println("GODMODE: -");
      }
      display.setTextSize(1);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("long: chng  short: ->");
      display.display();

      if (short_press) {
        state = ST_MENU;
      }
      if (long_press) {
        config.godmode = !config.godmode;
      }
      if (input_inactive) {
        presleep_state = state;
        state          = ST_SLEEP;
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

      unsigned char lv  = level[int((progress+x)/8)];
      unsigned char obj = lv >> 4;
      lv = lv & 0x0F;

      if (y - 3 < lv) {
        if (!config.godmode) {
          state = ST_GO;
          return;
        } else {
          dy = 0;
          y  = lv + 3;
        }
      }

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

      switch (obj) {
        // vent
        case 5:  dy += 2;  break;
        // balloon
        case 6:
                 if (y+3 > SCREEN_HEIGHT-12)
                   if (!config.godmode) {
                     state = ST_GO;
                     return;
                   } else {
                     y = SCREEN_HEIGHT-12-3;
                     dy = 0;
                   }
                 break;
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

      // shouldn't happen, but to be sure
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
        if (!config.godmode) {
          state = ST_GO;
          return;
        } else {
          y  = 3;
          dy = 0;
        }
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
        return;
      }

      draw_screen();
    } else if (state == ST_GO) {
      char buf[16];
      unsigned int pos;

      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-8, 1);
      display.setCursor(int(SCREEN_WIDTH/2)-30, 4);
      display.println("GAME OVER");

      for (pos = 0; pos <6; pos++)
        if (config.highscore[pos] < score)
          break;

      sprintf(buf, "score: #%d %04d", pos+1, score);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 14);
      display.println(buf);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("            short: ->");
      display.display();

      if (short_press) {
        if (!config.godmode)
          add_score(score);

        state = ST_HS;
      }
      if (input_inactive) {
        presleep_state = state;
        state          = ST_SLEEP;
      }

    } else if (state == ST_GG) {
      char buf[16];
      unsigned int pos;

      display.clearDisplay();
      display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2-8, 1);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 4);
      display.println("LEVEL CLEAR");

      for (pos = 0; pos <6; pos++) {
        if (config.highscore[pos] < score) {
          break;
        }
      }
      sprintf(buf, "score: #%d %04d", pos+1, score);
      display.setCursor(int(SCREEN_WIDTH/2)-38, 14);
      display.println(buf);
      display.setCursor(0, SCREEN_HEIGHT-8);
      display.println("            short: ->");
      display.display();

      if (short_press) {
        if (!config.godmode)
          add_score(score);

        state = ST_HS;
      }
      if (input_inactive) {
        presleep_state = state;
        state          = ST_SLEEP;
      }

    } else if (state == ST_SLEEP) {

      if (short_press || long_press) {
        display_on();

        state = presleep_state;
      } else
        display_off();
    } // state switch
        
    short_press = false;
    long_press  = false;
    stamp = millis();
  }
}

