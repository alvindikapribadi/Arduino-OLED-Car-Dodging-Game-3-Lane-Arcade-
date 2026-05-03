/*
  =========================================================
  TITLE   : Arduino OLED Car Dodging Game (3 Lane Arcade)

  DESCRIPTION:
  A simple arcade-style car dodging game using a 0.96" OLED.
  The player moves left/right across 3 lanes to avoid enemies.
  Difficulty increases gradually with smooth level scaling.

  FEATURES:
  - 3-Lane System (clean & fair gameplay)
  - Moving Road Animation (lane markers)
  - Multi Enemy System (based on level)
  - Smooth Auto Level & Speed Increase
  - Start → Play → Game Over → Start Flow
  - "Press Any Button" control
  - Sound Effects:
      • Move
      • Score
      • Crash
      • Game Over Melody


  COMPONENTS:
  - Arduino UNO / Nano
  - OLED 0.96" I2C (SSD1306)
  - 2x Push Button
  - Buzzer

 
  WIRING:

  OLED (I2C):
  - VCC → 5V
  - GND → GND
  - SDA → A4
  - SCL → A5

  BUTTON:
  - LEFT  → D2 to GND
  - RIGHT → D4 to GND
  (using INPUT_PULLUP)

  BUZZER:
  - + → D3
  - - → GND

  LIBRARIES:
  - Adafruit GFX
  - Adafruit SSD1306
  =========================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= PIN =================
#define BTN_LEFT   2
#define BTN_RIGHT  4
#define BUZZER     3

// ================= GAME STATE =================
#define STATE_START 0
#define STATE_PLAY  1
#define STATE_OVER  2
int gameState = STATE_START;

// ================= ROAD =================
int roadX = 14;
int roadW = 100;
int laneWidth = roadW / 3;
int roadOffset = 0;

// ================= PLAYER =================
int lane = 1;
int playerW = 10;
int playerH = 8;
int playerY = 54;
int playerX;
int lanePos[3];

// ================= ENEMY =================
#define MAX_ENEMY 5
int enemyLane[MAX_ENEMY];
int enemyX[MAX_ENEMY];
int enemyY[MAX_ENEMY];
int activeEnemies = 1;

int enemyW = 10;
int enemyH = 8;

// ================= GAME =================
int score = 0;
int level = 1;
int enemySpeed = 2;

// ================= CONTROL =================
unsigned long lastMove = 0;
int moveDelay = 140;

// ================= BUTTON =================
bool lastBtnState = HIGH;

// ================= SOUND =================
void soundMove()  { tone(BUZZER, 800, 40); }
void soundScore() { tone(BUZZER, 1100, 60); }
void soundCrash() { tone(BUZZER, 500, 100); }

// Game Over Melody
void soundGameOver() {
  tone(BUZZER, 800, 120); delay(130);
  tone(BUZZER, 600, 120); delay(130);
  tone(BUZZER, 400, 200); delay(220);
}

// ================= INIT LANE =================
void initLane() {
  for (int i = 0; i < 3; i++) {
    lanePos[i] = roadX + (laneWidth * i) + (laneWidth / 2) - (playerW / 2);
  }
}

// ================= BUTTON HELPER =================
bool isAnyButtonPressed() {
  return (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW);
}

bool isAnyButtonClicked() {
  bool current = isAnyButtonPressed();

  if (current && !lastBtnState) {
    lastBtnState = current;
    return true;
  }

  lastBtnState = current;
  return false;
}

// ================= SETUP =================
void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  randomSeed(analogRead(0));
  initLane();
}

// ================= RESET GAME =================
void resetGame() {
  lane = 1;
  playerX = lanePos[lane];

  score = 0;
  level = 1;
  enemySpeed = 2;
  activeEnemies = 1;
  roadOffset = 0;

  for (int i = 0; i < MAX_ENEMY; i++) {
    enemyLane[i] = random(0, 3);
    enemyX[i] = lanePos[enemyLane[i]];
    enemyY[i] = random(-60, 0);
  }
}

// ================= INPUT =================
void handleInput() {
  if (millis() - lastMove < moveDelay) return;

  if (digitalRead(BTN_LEFT) == LOW && lane > 0) {
    lane--;
    soundMove();
    lastMove = millis();
  }

  if (digitalRead(BTN_RIGHT) == LOW && lane < 2) {
    lane++;
    soundMove();
    lastMove = millis();
  }

  playerX = lanePos[lane];
}

// ================= UPDATE =================
void updateGame() {

  // Smooth level scaling
  level = (score / 10) + 1;

  // Speed increase (limited)
  enemySpeed = 2 + (level / 2);
  if (enemySpeed > 8) enemySpeed = 8;

  // Enemy count scaling
  activeEnemies = 1 + (level / 2);
  if (activeEnemies > MAX_ENEMY) activeEnemies = MAX_ENEMY;

  // Road animation
  roadOffset += enemySpeed;
  if (roadOffset > 10) roadOffset = 0;

  for (int i = 0; i < activeEnemies; i++) {

    enemyY[i] += enemySpeed;

    // Respawn enemy
    if (enemyY[i] > 64) {
      enemyY[i] = random(-60, 0);
      enemyLane[i] = random(0, 3);
      enemyX[i] = lanePos[enemyLane[i]];

      score++;
      soundScore();
    }

    // Collision detection
    if (playerX < enemyX[i] + enemyW &&
        playerX + playerW > enemyX[i] &&
        playerY < enemyY[i] + enemyH &&
        playerY + playerH > enemyY[i]) {

      soundCrash();
      soundGameOver();
      gameState = STATE_OVER;
    }
  }
}

// ================= DRAW ROAD =================
void drawRoad() {
  display.drawLine(roadX, 0, roadX, 64, WHITE);
  display.drawLine(roadX + roadW, 0, roadX + roadW, 64, WHITE);

  for (int y = -roadOffset; y < 64; y += 10) {
    display.drawLine(roadX + laneWidth, y, roadX + laneWidth, y + 5, WHITE);
    display.drawLine(roadX + laneWidth * 2, y, roadX + laneWidth * 2, y + 5, WHITE);
  }
}

// ================= DRAW GAME =================
void drawGame() {
  display.clearDisplay();

  drawRoad();

  display.fillRect(playerX, playerY, playerW, playerH, WHITE);

  for (int i = 0; i < activeEnemies; i++) {
    display.fillRect(enemyX[i], enemyY[i], enemyW, enemyH, WHITE);
  }

  display.display();
}

// ================= START SCREEN =================
void drawStart() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.print("CAR DODGE");

  display.setTextSize(1);
  display.setCursor(15, 45);
  display.print("PRESS ANY BUTTON");

  display.display();
}

// ================= GAME OVER SCREEN =================
void drawGameOver() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(10, 10);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(35, 35);
  display.print("Score:");
  display.print(score);

  display.setCursor(15, 55);
  display.print("PRESS ANY BUTTON");

  display.display();
}

// ================= MAIN LOOP =================
void loop() {

  switch (gameState) {

    case STATE_START:
      drawStart();
      if (isAnyButtonClicked()) {
        resetGame();
        gameState = STATE_PLAY;
      }
      break;

    case STATE_PLAY:
      handleInput();
      updateGame();
      drawGame();
      break;

    case STATE_OVER:
      drawGameOver();
      if (isAnyButtonClicked()) {
        gameState = STATE_START;
      }
      break;
  }
}