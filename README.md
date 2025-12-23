# Flappy bird on arduino

This is a simple game, that I wrote in a week to refresh my C skills and to get
used to working with arduino.  It uses 128x32 SSD1306-based OLED screen and one
button for inupt.

# Features

* animated bird
* level intro/outro
* animated objects on level with a degree of interactivity
* simple collision detection
* linear up/down movement
* bird energy limiter
* score counter
* main menu
* highscores with optional storage to EEPROM
* godmode toggle
* screen turns off on idle

# TODO

* more objects on the level
* better level design
* improve all animations
* non-linear movement and energy usage
* consumable objects on level (+energy)
* interactions with non-fatal objects (+score, -energy)
* add interaction animations with objects

# MAYBE

* replace Adafruit lib with own code
* detect double-click, use it for eg short-range teleportation
