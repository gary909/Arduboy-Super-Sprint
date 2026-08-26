// V5 - Added Title Screen & Initials Entry System
// V4 - Added Timer, lap counter
// V3 - Starting to add courses / Added Up button for Brake and Down button for reverse
// V2 added better car physics (b-button now adds drift)
#include <Arduboy2.h>
#include <avr/pgmspace.h>

Arduboy2 arduboy;

// --- Game States ---
enum GameState {
  STATE_TITLE,
  STATE_INITIALS,
  STATE_GAME
};

GameState currentState = STATE_TITLE;

// --- Initials Entry State ---
char playerInitials[4] = "AAA";
uint8_t currentInitialIdx = 0;
uint8_t blinkTimer = 0;

// --- 3x5 Pixel Font Data (0-9, A-Z, space, colon, slash) ---
const uint8_t PROGMEM font3x5[][3] = {
  {0x1F, 0x11, 0x1F}, // 0
  {0x00, 0x1F, 0x00}, // 1
  {0x1D, 0x15, 0x17}, // 2
  {0x15, 0x15, 0x1F}, // 3
  {0x07, 0x04, 0x1F}, // 4
  {0x17, 0x15, 0x1D}, // 5
  {0x1F, 0x15, 0x1D}, // 6
  {0x01, 0x01, 0x1F}, // 7
  {0x1F, 0x15, 0x1F}, // 8
  {0x17, 0x15, 0x1F}, // 9
  {0x00, 0x0A, 0x00}, // : (10)
  {0x18, 0x04, 0x03}, // / (11)
  {0x00, 0x00, 0x00}, // Space (12)
  {0x1F, 0x05, 0x1F}, // A (13)
  {0x1F, 0x15, 0x0A}, // B (14)
  {0x0E, 0x11, 0x11}, // C (15)
  {0x1F, 0x11, 0x0E}, // D (16)
  {0x1F, 0x15, 0x11}, // E (17)
  {0x1F, 0x05, 0x01}, // F (18)
  {0x0E, 0x11, 0x1D}, // G (19)
  {0x1F, 0x04, 0x1F}, // H (20)
  {0x11, 0x1F, 0x11}, // I (21)
  {0x08, 0x10, 0x0F}, // J (22)
  {0x1F, 0x04, 0x1B}, // K (23)
  {0x1F, 0x10, 0x10}, // L (24)
  {0x1F, 0x02, 0x1F}, // M (25)
  {0x1F, 0x02, 0x1C}, // N (26)
  {0x0E, 0x11, 0x0E}, // O (27)
  {0x1F, 0x05, 0x02}, // P (28)
  {0x0E, 0x13, 0x1E}, // Q (29)
  {0x1F, 0x05, 0x1A}, // R (30)
  {0x12, 0x15, 0x09}, // S (31)
  {0x01, 0x1F, 0x01}, // T (32)
  {0x0F, 0x10, 0x0F}, // U (33)
  {0x07, 0x18, 0x07}, // V (34)
  {0x1F, 0x08, 0x1F}, // W (35)
  {0x1B, 0x04, 0x1B}, // X (36)
  {0x03, 0x1C, 0x03}, // Y (37)
  {0x19, 0x15, 0x13}  // Z (38)
};

void drawChar3x5(int16_t x, int16_t y, char c) {
  uint8_t idx = 12; // default space
  if (c >= '0' && c <= '9') idx = c - '0';
  else if (c == ':') idx = 10;
  else if (c == '/') idx = 11;
  else if (c >= 'A' && c <= 'Z') idx = c - 'A' + 13;
  else if (c >= 'a' && c <= 'z') idx = c - 'a' + 13;

  for (uint8_t col = 0; col < 3; col++) {
    uint8_t line = pgm_read_byte(&font3x5[idx][col]);
    for (uint8_t row = 0; row < 5; row++) {
      if (line & (1 << row)) {
        arduboy.drawPixel(x + col, y + row, WHITE);
      }
    }
  }
}

void drawString3x5(int16_t x, int16_t y, const char* str) {
  while (*str) {
    drawChar3x5(x, y, *str);
    x += 4; // 3 pixels wide + 1 pixel spacing
    str++;
  }
}

// --- Track Data Structures ---
struct Wall {
  int8_t x1, y1, x2, y2;
};

const Wall PROGMEM track1_walls[] = {
  // Outer boundary
  { 7,   1, 121,   1},
  {121,  1, 127,   7},
  {127,  7, 127,  57},
  {127, 57, 121,  63},
  {121, 63,   7,  63},
  {  7, 63,   1,  57},
  {  1, 57,   1,   7},
  {  1,  7,   7,   1},

  // Inner island
  { 38, 22,  90, 22},
  { 90, 22,  96, 28},
  { 96, 28,  96, 36},
  { 96, 36,  90, 42},
  { 90, 42,  38, 42},
  { 38, 42,  32, 36},
  { 32, 36,  32, 28},
  { 32, 28,  38, 22}
};

const uint8_t NUM_WALLS = sizeof(track1_walls) / sizeof(Wall);

// --- Checkpoint & Start/Finish Lines ---
const int8_t checkPointX1 = 64, checkPointY1 = 42, checkPointX2 = 64, checkPointY2 = 63;
const int8_t startLineX1  = 67, startLineY1  = 1,  startLineX2  = 67, startLineY2  = 22;

// --- Race & Timing State ---
uint8_t currentLap = 1;
const uint8_t TOTAL_LAPS = 5;
bool checkpointPassed = false;
bool raceStarted = false;
bool raceFinished = false;

uint32_t totalRaceFrames = 0;

// --- Car Physics State ---
float carX = 64.0;
float carY = 12.0;
float angle = 0.0;
float angularVelocity = 0.0;

float moveX = 0.0;
float moveY = 0.0;

bool wasHandbraking = false;

// --- Helpers ---
bool linesIntersect(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy) {
  float denom = (dy - cy) * (bx - ax) - (dx - cx) * (by - ay);
  if (denom == 0) return false;

  float ua = ((dx - cx) * (ay - cy) - (dy - cy) * (ax - cx)) / denom;
  float ub = ((bx - ax) * (ay - cy) - (by - ay) * (ax - cx)) / denom;

  return (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0);
}

void formatTime(uint32_t frames, char* buffer) {
  uint32_t seconds = frames / 60;
  uint32_t hundredths = (frames % 60) * 100 / 60;

  buffer[0] = '0' + (seconds / 10);
  buffer[1] = '0' + (seconds % 10);
  buffer[2] = ':';
  buffer[3] = '0' + (hundredths / 10);
  buffer[4] = '0' + (hundredths % 10);
  buffer[5] = '\0';
}

void resetRace() {
  currentLap = 1;
  checkpointPassed = false;
  raceStarted = false;
  raceFinished = false;
  totalRaceFrames = 0;
  carX = 64.0;
  carY = 12.0;
  angle = 0.0;
  angularVelocity = 0.0;
  moveX = 0.0;
  moveY = 0.0;
}

void drawTrackAndHUD() {
  for (uint8_t i = 0; i < NUM_WALLS; i++) {
    Wall w;
    memcpy_P(&w, &track1_walls[i], sizeof(Wall));
    arduboy.drawLine(w.x1, w.y1, w.x2, w.y2, WHITE);
  }
  
  arduboy.drawFastVLine(67, 1, 20, WHITE);

  // Render 3x5 HUD at the bottom of the screen (Y = 57)
  char timeBuf[6];
  formatTime(totalRaceFrames, timeBuf);

  if (raceFinished) {
    drawString3x5(38, 57, "FINISHED ");
    drawString3x5(78, 57, timeBuf);
  } else if (!raceStarted) {
    drawString3x5(30, 57, "PRESS A TO START");
  } else {
    // Lap counter left-aligned, Timer right-aligned
    char lapBuf[8];
    lapBuf[0] = 'L'; lapBuf[1] = 'A'; lapBuf[2] = 'P'; lapBuf[3] = ' ';
    lapBuf[4] = '0' + currentLap; lapBuf[5] = '/'; lapBuf[6] = '0' + TOTAL_LAPS; lapBuf[7] = '\0';

    drawString3x5(38, 57, lapBuf);
    drawString3x5(74, 57, timeBuf);
  }
}

bool resolveWallCollision(float &x, float &y, float &vx, float &vy) {
  bool collided = false;

  for (uint8_t i = 0; i < NUM_WALLS; i++) {
    Wall w;
    memcpy_P(&w, &track1_walls[i], sizeof(Wall));

    float A = x - w.x1;
    float B = y - w.y1;
    float C = w.x2 - w.x1;
    float D = w.y2 - w.y1;

    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    float param = (len_sq != 0) ? (dot / len_sq) : -1;

    float closestX, closestY;

    if (param < 0) {
      closestX = w.x1;
      closestY = w.y1;
    } else if (param > 1) {
      closestX = w.x2;
      closestY = w.y2;
    } else {
      closestX = w.x1 + param * C;
      closestY = w.y1 + param * D;
    }

    float dx = x - closestX;
    float dy = y - closestY;
    float distSq = dx * dx + dy * dy;
    
    float radius = 3.0;

    if (distSq < (radius * radius) && distSq > 0.0001) {
      collided = true;
      float dist = sqrt(distSq);

      float nx = dx / dist;
      float ny = dy / dist;

      float overlap = radius - dist;
      x += nx * overlap;
      y += ny * overlap;

      float dotVel = vx * nx + vy * ny;
      if (dotVel < 0) {
        vx -= 1.4 * dotVel * nx;
        vy -= 1.4 * dotVel * ny;
        vx *= 0.6;
        vy *= 0.6;
      }
    }
  }
  return collided;
}

// --- Title Screen Handler ---
void updateTitleScreen() {
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON) ||
      arduboy.justPressed(UP_BUTTON) || arduboy.justPressed(DOWN_BUTTON) ||
      arduboy.justPressed(LEFT_BUTTON) || arduboy.justPressed(RIGHT_BUTTON)) {
    currentState = STATE_INITIALS;
    currentInitialIdx = 0;
  }

  arduboy.drawRect(4, 4, 120, 56, WHITE);
  drawString3x5(28, 18, "MOTODROME DRIFTER");
  
  blinkTimer++;
  if ((blinkTimer / 30) % 2 == 0) {
    drawString3x5(30, 42, "PRESS ANY BUTTON");
  }
}

// --- Initials Input Screen Handler ---
void updateInitialsScreen() {
  // Letter navigation (Up/Down)
  if (arduboy.justPressed(UP_BUTTON)) {
    playerInitials[currentInitialIdx]--;
    if (playerInitials[currentInitialIdx] < 'A') playerInitials[currentInitialIdx] = 'Z';
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    playerInitials[currentInitialIdx]++;
    if (playerInitials[currentInitialIdx] > 'Z') playerInitials[currentInitialIdx] = 'A';
  }

  // Select current letter and advance
  if (arduboy.justPressed(A_BUTTON)) {
    currentInitialIdx++;
    if (currentInitialIdx >= 3) {
      resetRace();
      currentState = STATE_GAME;
      return;
    }
  }

  // Backspace / Delete previous letter
  if (arduboy.justPressed(B_BUTTON) && currentInitialIdx > 0) {
    currentInitialIdx--;
  }

  // Render UI
  drawString3x5(34, 14, "ENTER INITIALS");

  // Display the 3 initials spaced out
  for (uint8_t i = 0; i < 3; i++) {
    int16_t charX = 52 + (i * 10);
    drawChar3x5(charX, 30, playerInitials[i]);

    // Draw active cursor / underline for active slot
    if (i == currentInitialIdx) {
      if ((blinkTimer / 15) % 2 == 0) {
        arduboy.drawFastHLine(charX, 37, 3, WHITE);
      }
    }
  }

  drawString3x5(22, 50, "UP/DN: CHOOSE  A: NEXT");
  blinkTimer++;
}

// --- Main Race Loop Handler ---
void updateGameScreen() {
  bool isHandbraking = arduboy.pressed(B_BUTTON);
  bool isReversing = arduboy.pressed(UP_BUTTON);
  bool isAccelerating = arduboy.pressed(A_BUTTON);

  // Use justPressed to demand a discrete new press for the starting trigger
  if (!raceStarted && arduboy.justPressed(A_BUTTON)) {
    raceStarted = true;
  }

  if (raceStarted && !raceFinished) {
    totalRaceFrames++;
  }

  // Steering Controls
  if (isHandbraking && !wasHandbraking) {
    moveX *= 0.85;
    moveY *= 0.85;
  }
  wasHandbraking = isHandbraking;

  float turnAccel = isHandbraking ? 0.028 : 0.012; 
  if (arduboy.pressed(LEFT_BUTTON))  angularVelocity -= turnAccel;
  if (arduboy.pressed(RIGHT_BUTTON)) angularVelocity += turnAccel;

  float maxAngularVel = isHandbraking ? 0.075 : 0.09;
  if (angularVelocity > maxAngularVel)  angularVelocity = maxAngularVel;
  if (angularVelocity < -maxAngularVel) angularVelocity = -maxAngularVel;

  angle += angularVelocity;

  float rotationalDamping = isHandbraking ? 0.80 : 0.75;
  if (isHandbraking && !arduboy.pressed(LEFT_BUTTON) && !arduboy.pressed(RIGHT_BUTTON)) {
    rotationalDamping = 0.65;
  }
  angularVelocity *= rotationalDamping;

  // Acceleration & Braking
  if (isAccelerating && raceStarted) {
    float accel = 0.05;
    moveX += cos(angle) * accel;
    moveY += sin(angle) * accel;
  } else if (isReversing && raceStarted) {
    float revAccel = 0.03;
    moveX -= cos(angle) * revAccel;
    moveY -= sin(angle) * revAccel;
  }

  if (arduboy.pressed(DOWN_BUTTON)) {
    moveX *= 0.85;
    moveY *= 0.85;
  }

  // Drift Physics
  if (isHandbraking) {
    moveX *= 0.998;
    moveY *= 0.998;
  } else {
    float forwardSpeed = sqrt(moveX * moveX + moveY * moveY);
    moveX *= 0.96;
    moveY *= 0.96;

    float dotProduct = moveX * cos(angle) + moveY * sin(angle);
    float direction = (dotProduct >= 0) ? 1.0 : -1.0;

    float targetMoveX = cos(angle) * forwardSpeed * direction;
    float targetMoveY = sin(angle) * forwardSpeed * direction;
    
    moveX = (moveX * 0.985) + (targetMoveX * 0.015);
    moveY = (moveY * 0.985) + (targetMoveY * 0.015);
  }

  // Velocity Cap
  float currentSpeed = sqrt(moveX * moveX + moveY * moveY);
  float maxAllowedSpeed = isReversing ? 0.5 : 1.0;

  if (currentSpeed > maxAllowedSpeed) {
    moveX = (moveX / currentSpeed) * maxAllowedSpeed;
    moveY = (moveY / currentSpeed) * maxAllowedSpeed;
  }

  // Collision & Position
  float prevX = carX;
  float prevY = carY;

  carX += moveX;
  carY += moveY;
  resolveWallCollision(carX, carY, moveX, moveY);

  // Checkpoints & Laps
  if (raceStarted && !raceFinished) {
    if (!checkpointPassed) {
      if (linesIntersect(prevX, prevY, carX, carY, checkPointX1, checkPointY1, checkPointX2, checkPointY2)) {
        checkpointPassed = true;
      }
    }

    if (checkpointPassed) {
      if (linesIntersect(prevX, prevY, carX, carY, startLineX1, startLineY1, startLineX2, startLineY2)) {
        checkpointPassed = false;
        currentLap++;

        if (currentLap > TOTAL_LAPS) {
          raceFinished = true;
        }
      }
    }
  }

  // Render Frame
  drawTrackAndHUD();
  
  int endX = carX + cos(angle) * 6;
  int endY = carY + sin(angle) * 6;
  arduboy.drawLine((int)carX, (int)carY, endX, endY, WHITE);
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
}

void loop() {
  if (!arduboy.nextFrame()) return;

  arduboy.pollButtons();
  arduboy.clear();

  switch (currentState) {
    case STATE_TITLE:
      updateTitleScreen();
      break;
    case STATE_INITIALS:
      updateInitialsScreen();
      break;
    case STATE_GAME:
      updateGameScreen();
      break;
  }

  arduboy.display();
}