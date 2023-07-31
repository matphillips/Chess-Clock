// Chess Clock. MPhillips. https://github.com/matphillips/Chess-Clock
//
#include <TM1637.h> // https://github.com/AKJ7/TM1637
#include <ESP8266WiFi.h>
#include "Buzzer.h" // https://github.com/gmarty2000-ARDUINO/arduino-BUZZER
#include <Button.h> // https://github.com/madleech/Button
#include <Ticker.h>

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF
#define MAX_BRIGHTNESS_LEVELS 4
#define MAX_GAME_LENGTHS 5

// define pins
#define DIO1 12
#define CLK1 0 // 0
#define DIO2 14
#define CLK2 2
#define BUTTON_MODE 3 // 13
#define BUTTON_BRIGHTNESS 1
#define BUTTON_LEFT 13 // 4
#define BUTTON_RIGHT 5
#define BUZZER_PIN 4 // 3

// init
TM1637 display1(CLK1, DIO1);
TM1637 display2(CLK2, DIO2);
Buzzer buzzer(BUZZER_PIN);
Button buttonMode(BUTTON_MODE);
Button buttonBrightness(BUTTON_BRIGHTNESS);
Button buttonLeft(BUTTON_LEFT);
Button buttonRight(BUTTON_RIGHT);
Ticker tickerPlayer1;
Ticker tickerPlayer2;
Ticker tickerFlash;

// vars
int brightnessLevels[MAX_BRIGHTNESS_LEVELS] = {1, 2, 4, 9};
int brightness = 2;
int gameLengths[MAX_GAME_LENGTHS] = {5, 10, 15, 25, 40, 60};
int gameLength = 2;
int gameState = 0; // 0 not started, 1 running, 2 finished
int activePlayer = 0; // 0 not started, 1 left, 2 right
int player1time = 0;
int player2time = 0;
bool playedGameOverSound = false;
bool flashState = false;

// Vars to track elapsed time
unsigned long prevTimePlayer1 = 0;
unsigned long prevTimePlayer2 = 0;

void setup() {
  wifiOff();
  buttonMode.begin();
  buttonBrightness.begin();
  buttonLeft.begin();
  buttonRight.begin();
  tickerPlayer1.attach(1.0, tickerHandler);
  tickerPlayer2.attach(1.0, tickerHandler);
  tickerFlash.attach(0.5, tickerHandler);
  display1.init();
  display2.init();
  display1.setBrightness(brightnessLevels[brightness]);
  display2.setBrightness(brightnessLevels[brightness]);
  display1.colonOn();
  display2.colonOn();
  beepStartup();
}

void loop() {
  handleButtonPress();
  
  if (buttonMode.released() && gameState == 0) { // cycle game time mode
    gameLength = (gameLength == MAX_GAME_LENGTHS-1) ? 0 : gameLength + 1;
  }

  if (buttonBrightness.released()) { // cycle brightness levels
    brightness = (brightness == MAX_BRIGHTNESS_LEVELS-1) ? 0 : brightness + 1;
    display1.setBrightness(brightnessLevels[brightness]);
    display2.setBrightness(brightnessLevels[brightness]);
    display1.refresh();
    display2.refresh();
  }

  if (gameState == 1 && (player1time == 0 || player2time == 0)) { // player out of time, stop game
    gameState = 2;
  }

  switch (gameState) {
    case 0: // Idle, both clocks show max time
      display1.display(formatTime(gameLengths[gameLength]) + formatTime(0).c_str());
      display2.display(formatTime(gameLengths[gameLength]) + formatTime(0).c_str());
    break;

    case 1: // Running, clocks count down
      if (activePlayer == 1) {
        int minutes = player1time / 60; // Integer division to get the whole minutes
        int seconds = player1time % 60; // Remainder will be the seconds
        String timeStr = formatTime(minutes) + formatTime(seconds);
        display1.display(timeStr.c_str());
      }
      if (activePlayer == 2) {
        int minutes = player2time / 60; // Integer division to get the whole minutes
        int seconds = player2time % 60; // Remainder will be the seconds
        String timeStr = formatTime(minutes) + formatTime(seconds);
        display2.display(timeStr.c_str());
      }
    break;

    case 2: // game over
      if (!playedGameOverSound) {
        beepGameover();
        playedGameOverSound = true;
      }
    break;
  }
}

void resetGame() {
  playedGameOverSound = false;
  player1time = gameLengths[gameLength] * 60;
  player2time = gameLengths[gameLength] * 60;
  displayCountdownTime(display1, player1time);
  displayCountdownTime(display2, player2time);
  display1.colonOn();
  display2.colonOn();
  display1.refresh();
  display2.refresh();
}

void handleButtonPress() {
  bool isLeftButtonPressed = buttonLeft.released();
  bool isRightButtonPressed = buttonRight.released();

  if (isLeftButtonPressed || isRightButtonPressed) {
    if (gameState == 0) { // start game
      gameState = 1;
      resetGame();
      beepGameStart();
      activePlayer = (isLeftButtonPressed) ? 2 : 1;
    } else if (gameState == 1 && isLeftButtonPressed && activePlayer == 1) { // switch to other player
      activePlayer = 2;
      display1.colonOn();
      display1.refresh();
      beep();
    } else if (gameState == 1 && isRightButtonPressed && activePlayer == 2) { // switch to other player
      activePlayer = 1;
      display2.colonOn();
      display2.refresh();
      beep();
    } else if (gameState == 2) { // reset game
      gameState = 0;
      resetGame();
    }
  }
}

void displayCountdownTime(TM1637& display, int time) {
  int minutes = time / 60;
  int seconds = time % 60;
  String timeStr = formatTime(minutes) + formatTime(seconds);
  display.display(timeStr.c_str());
}

void tickerHandler() {
  unsigned long currentTime = millis();

  if (tickerPlayer1.active() && gameState == 1 && activePlayer == 1 && player1time > 0) {
    // Calculate elapsed time and update player1time
    if (currentTime - prevTimePlayer1 >= 1000) {
      prevTimePlayer1 = currentTime;
      player1time--;
    }
  }

  if (tickerPlayer2.active() && gameState == 1 && activePlayer == 2 && player2time > 0) {
    // Calculate elapsed time and update player2time
    if (currentTime - prevTimePlayer2 >= 1000) {
      prevTimePlayer2 = currentTime;
      player2time--;
    }
  }

  if (tickerFlash.active() && gameState == 1) {
    if (activePlayer == 1 && player1time > 0) {
      display1.switchColon();
      display1.refresh();
    }
    if (activePlayer == 2 && player2time > 0) {
      display2.switchColon();
      display2.refresh();
    }
  }

  if (tickerFlash.active() && gameState == 2) {
    if (player1time == 0) {
      display1.display((flashState) ? "0000" : "----");
      display1.colonOn();
    } else if (player2time == 0) {
      display2.display((flashState) ? "0000" : "----");
      display2.colonOn();
    }
    flashState = !flashState;
  }
}

String formatTime(int timeValue) { // Function to format time with leading zeros
  if (timeValue < 10) {
    return "0" + String(timeValue);
  } else {
    return String(timeValue);
  }
}

void beep() {
  buzzer.begin(0);
  buzzer.sound(NOTE_A5, 50);
  buzzer.end(10);
}

void beepStartup() {
  buzzer.begin(0);
  buzzer.sound(NOTE_A3, 100);
  buzzer.sound(NOTE_A4, 100);
  buzzer.sound(NOTE_A5, 100);
  buzzer.end(10);
}

void beepGameStart() {
  buzzer.begin(0);
  buzzer.sound(NOTE_A5, 90);
  buzzer.sound(NOTE_A6, 90);
  buzzer.sound(NOTE_A5, 90);
  buzzer.end(10);
}

void beepGameover() {
  buzzer.begin(0);
  buzzer.sound(NOTE_A5, 100);
  buzzer.sound(NOTE_A4, 100);
  buzzer.sound(NOTE_A3, 100);
  buzzer.sound(NOTE_A2, 100);
  buzzer.end(10);
}

void wifiOff() {
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
}
