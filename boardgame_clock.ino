/*
File: boardgame_clock.ino
Author: Pontus Martinsson
Date: 2023-12-10
Description: This program uses an oled-display and a joystick to create a tool intended for use with board games.
*/

#include <U8g2lib.h>
#include <RtcDS3231.h>
#include "bitmaps.h"

#define joystickX A0
#define joystickY A1
#define joystickBtn 2
#define ledPin 13

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE); // display object

// variables used for navigation in menus
int selectedItem = 0;
int previousItem;
int nextItem;
bool buttonState;
bool buttonToggle;
int currentMenuItems;

// constants for object position, for convenience
const int previousItemYPosition = 15;
const int selectedItemYPosition = 37;
const int nextItemYPosition = 59;
const int labelXPosition = 25;

// additional variables
int currentPlayer;
int dice = random(1, 6);
int currentScreen = 0;
int timeWarning = 20;

// additional arrays
int settingValue[mainMenuItems] { 0, 2, 0, 5, 0 }; // default setting values
int playerLives[10] {};

// fonts
const uint16_t regularFont = u8g2_font_7x13_mf;
const uint16_t boldFont = u8g2_font_7x13B_mf;

// variables used to keep track of time
unsigned long lastUpdateMillis;
unsigned long gameStartMillis;
unsigned long turnStartMillis;

void setup() {
  Serial.begin(9600);

  pinMode(joystickBtn, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.setColorIndex(1);
  u8g2.setPowerSave(0);

  // start in the main menu
  menu();
  drawMenu();
}

// help function for reading the joystick's x-value
int readJoystickX() {
  if (analogRead(joystickX) >= 823) {
    return (1);
  } 
  else if (analogRead(joystickX) <= 200) {
    return (-1);
  } 
}

// help function for reading the joystick's y-value
int readJoystickY() {
  if (analogRead(joystickY) >= 823) {
    return (-1);
  } 
  else if (analogRead(joystickY) <= 200) {
    return (1);
  }
}

// read the joystick button and apply changes accordingly
void joystickButton() {
  buttonState = digitalRead(joystickBtn);

  if (buttonState == false && buttonToggle == false) {
    if (currentScreen == 0 && selectedItem == 0) { // start game
      gameStartMillis = millis();
      turnStartMillis = millis();
      currentPlayer = 1;
      buttonToggle = true;
      currentScreen = 1;

      for (int i = 1; i <= settingValue[1]; i++) {
        playerLives[i] = settingValue[4];
      }
    }
    else if (currentScreen == 1) { // pause
      currentScreen = 2;
      buttonToggle = true;
      menu();
      drawMenu();
    }
    else if (currentScreen == 2) { // game menu
      if (selectedItem == 0) { // resume
        currentScreen = 1;
        buttonToggle = true;
      }
      else if (selectedItem == 1) { // life menu
        currentScreen = 3;
        buttonToggle = true;
        selectedItem = 0;
        menu();
        drawMenu();
      }
      else if (selectedItem == 2) { // main menu
        currentScreen = 0;
        buttonToggle = true;
        selectedItem = 0;
        menu();
        drawMenu();
      }
      else if (selectedItem == 3) { // roll dice
        dice = 0;
        drawMenu();
        dice = random(1, 7);
        drawMenu();
      }
    }
    else if (currentScreen == 3) { // life menu
      if (selectedItem == 0) { // back
        currentScreen = 2;
        buttonToggle = true;
        selectedItem = 0;
        menu();
        drawMenu();
      }
    }
  }
  else if (buttonToggle == true && buttonState == true) {
    buttonToggle = false;
  }  
}

// navigate menu up and down
void scrollMenu() {
  if (readJoystickY() == 1) {
    selectedItem = selectedItem - 1;

    if (selectedItem < 0) {
      selectedItem = currentMenuItems - 1;
    }
    drawMenu();
  } 
  else if (readJoystickY() == -1) {
    selectedItem = selectedItem + 1;

    if (selectedItem >= currentMenuItems) {
      selectedItem = 0;
    }
    drawMenu();
  }
}

// change the appropriate value
void changeValue() {
  const int settingValueIncrement[mainMenuItems] { 0, 1, 10, 1, 1 };
  const int settingValueMaximum[mainMenuItems] { 0, 10, 720, 20, 999 };
  const int settingValueMinimum[mainMenuItems] { 0, 2, 0, 0, 0};

   if (currentScreen == 0) { // main menu

    if (readJoystickX() == -1) {
      settingValue[selectedItem] += settingValueIncrement[selectedItem];

      if (settingValue[selectedItem] > settingValueMaximum[selectedItem]) {
        settingValue[selectedItem] = settingValueMinimum[selectedItem];
      }
      drawMenu();
    } 
    else if (readJoystickX() == 1) {
      settingValue[selectedItem] -= settingValueIncrement[selectedItem];

      if (settingValue[selectedItem] < settingValueMinimum[selectedItem]) {
        settingValue[selectedItem] = settingValueMaximum[selectedItem];
      }
      drawMenu();
    }
  }
  else if (currentScreen == 1) { // game screen
    if (readJoystickX() == -1) {
      currentPlayer += 1;
      turnStartMillis = millis();

      if (currentPlayer > settingValue[1]) {
        currentPlayer = 1;
      }
      drawGame();
    } 
    else if (readJoystickX() == 1) {
      currentPlayer -= 1;
      turnStartMillis = millis();

      if (currentPlayer <= 0) {
        currentPlayer = settingValue[1];
      }
      drawGame();
    }
    else if (readJoystickY() == 1) {
      playerLives[currentPlayer] += 1;

      if (playerLives[currentPlayer] > settingValueMaximum[4]) {
        playerLives[currentPlayer] = settingValueMinimum[4];
      }
      drawGame();
    } 
    else if (readJoystickY() == -1) {
      playerLives[currentPlayer] -= 1;

      if (playerLives[currentPlayer] < settingValueMinimum[4]) {
        currentPlayer = settingValueMaximum[4];
      }
      drawGame();
    }
  }
  else if (currentScreen == 3) { // life menu
    if (readJoystickX() == -1) {
      playerLives[selectedItem] += settingValueIncrement[4];

      if (playerLives[selectedItem] > settingValueMaximum[4]) {
        playerLives[selectedItem] = settingValueMinimum[4];
      }
      drawMenu();
    }
    else if (readJoystickX() == 1) {
      playerLives[selectedItem] -= settingValueIncrement[4];

      if (playerLives[selectedItem] < settingValueMinimum[4]) {
        playerLives[selectedItem] = settingValueMaximum[4];
      }
      drawMenu();
    }
  }
}

// operate the active menu
void menu() {
  if (currentScreen == 0) { 
    currentMenuItems = mainMenuItems; 
  } 
  else if (currentScreen == 2) {
    currentMenuItems = pauseMenuItems;
  }
  else if (currentScreen == 3) {
    currentMenuItems = settingValue[1] + 1;
  }

  joystickButton();
  scrollMenu();
  changeValue();
}

// draw the appropriate menu
void drawMenu() {
  if (selectedItem == 0) { previousItem = currentMenuItems - 1; }  // if the first item is selected - previous item is the last item
  else { previousItem = selectedItem - 1; }

  if (selectedItem == currentMenuItems - 1) { nextItem = 0; }  // if the last item is selected - next item is the first item
  else { nextItem = selectedItem + 1; }

  int currentMenuArray;
  if (currentScreen == 0) { currentMenuArray = 0; }
  else if (currentScreen == 2) { currentMenuArray = 1; }
  else if (currentScreen == 3) { currentMenuArray = 2; }

  u8g2.clearBuffer();

  u8g2.drawXBMP(0, 22, 128, 21, bitmapItemSelectionOutline); // draw selection outline
  drawMenuItem(currentMenuArray, previousItem, regularFont, previousItemYPosition); // draw previous item
  drawMenuItem(currentMenuArray, selectedItem, boldFont, selectedItemYPosition); // draw selected item
  drawMenuItem(currentMenuArray, nextItem, regularFont, nextItemYPosition); // draw next item

  u8g2.sendBuffer();
}

// draw menu item
void drawMenuItem(int menu, int item, uint16_t font, int yPosition) {
  char visibleValue[3]; // empty array for converting int to char

  // draw outline and label
  u8g2.setFont(font);
  if (menu == 2) {
    if (item == 0) { 
      u8g2.drawStr(labelXPosition, yPosition, "Back"); 
    }
    else {
      u8g2.drawStr(labelXPosition, yPosition, "Player");
      itoa(item, visibleValue, 10);
      u8g2.drawStr(69, yPosition, visibleValue);
    }
  }
  else {
    u8g2.drawStr(labelXPosition, yPosition, labels[menu][item]);
  }

  // draw the appropriate icon
  if (menu == 1 && item == pauseMenuItems -1) {
    u8g2.drawXBMP(4, yPosition - 13, 16, 16, bitmapDiceIcons[dice]); // draw dice icon
  }
  else if (menu == 2) {
    if (item == 0) { u8g2.drawXBMP(4, yPosition - 13, 16, 16, bitmapBackIcon); }
    else {u8g2.drawXBMP(4, yPosition - 13, 16, 16, bitmapLifeIcon); } // draw life icon
  }
  else {
    u8g2.drawXBMP(4, yPosition - 13, 16, 16, bitmapMenuIcons[menu][item]); // draw icon
  }

  // draw value
  if (menu == 0) {
    if (settingValue[item] != 0) {
      itoa(settingValue[item], visibleValue, 10); // convert int to char
      u8g2.drawStr(105, yPosition, visibleValue);
    } 
    else if (item != 0) { // do not draw a setting value for the first item (start/resume)
      u8g2.drawStr(105, yPosition, "N/A");
    }
  }
  else if (menu == 2) {
    if (playerLives[item] != 0) {
      itoa(playerLives[item], visibleValue, 10); // convert int to char
      u8g2.drawStr(105, yPosition, visibleValue);
    } 
    else if (item != 0) { // do not draw a setting value for the first item (start/resume)
      u8g2.drawStr(105, yPosition, "ded");
    }
  }
}

// method to calculate how much time is left and apply that to a char in the HH:MM:SS format
void timeLeft(int settingValNum, char* result, unsigned long startMillis) {
  long secondsLeft = (settingValue[settingValNum] * 60UL) - ((millis() - startMillis) / 1000); // calculate number of seconds left

  if (secondsLeft < timeWarning && settingValNum == 3) { // if almost out of time, light up led
    digitalWrite(ledPin, true);
  }
  else if (settingValNum == 3) {
    digitalWrite(ledPin, false);
  }

  if (secondsLeft > 0) {
    // calculate remaining time
    int remainingHours = secondsLeft / 3600; 
    int leftoverSeconds = secondsLeft % 3600;
    int remainingMinutes = leftoverSeconds / 60;
    int remainingSeconds = leftoverSeconds % 60;

    sprintf(result, "%02d:%02d:%02d", remainingHours, remainingMinutes, remainingSeconds); // apply the remaining time to a char in HH:MM:SS format
  } 
  else {
    strcpy(result, "00:00:00"); // copy "00:00:00" to a char
  }
}

// operate the game screen
void game() {
  if (lastUpdateMillis + 350 < millis()) { // only update time once every 350 ms
    drawGame();
  lastUpdateMillis = millis();
  }
  joystickButton();
  changeValue();
}

// draw the game screen
void drawGame() {
  char numberChar[8]; // empty char
  
  u8g2.clearBuffer();

  // whose turn it is
  u8g2.setFont(boldFont);
  u8g2.drawStr(4, previousItemYPosition, "Player");
  itoa(currentPlayer, numberChar, 10);
  u8g2.drawStr(50, previousItemYPosition, numberChar);

  // how many lives the current player has
  u8g2.setFont(regularFont);
  u8g2.drawXBMP(83, previousItemYPosition - 13, 16, 16, bitmapLifeIcon);
  itoa(playerLives[currentPlayer], numberChar, 10);
  u8g2.drawStr(103, previousItemYPosition, numberChar);

  // how much time is left on this turn
  u8g2.drawXBMP(4, selectedItemYPosition - 13, 16, 16, bitmapMenuIcons[0][3]);
  timeLeft(3, numberChar, turnStartMillis);
  u8g2.drawStr(labelXPosition, selectedItemYPosition, numberChar);

  // how much time is left in total
  u8g2.drawXBMP(4, nextItemYPosition - 13, 16, 16, bitmapMenuIcons[0][2]);
  timeLeft(2, numberChar, gameStartMillis);
  u8g2.drawStr(labelXPosition, nextItemYPosition, numberChar);

  u8g2.sendBuffer();
}

// no explanation needed
void loop() {
  if (currentScreen == 1) { game(); } 
  else { menu(); }
}