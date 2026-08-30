/*
┏┳┓┏━┓╺┳╸┏━┓╺┳┓┏━┓┏━┓┏┳┓┏━╸
┃┃┃┃ ┃ ┃ ┃ ┃ ┃┃┣┳┛┃ ┃┃┃┃┣╸ 
╹ ╹┗━┛ ╹ ┗━┛╺┻┛╹┗╸┗━┛╹ ╹┗━╸
   ╺┳┓┏━┓╻┏━╸╺┳╸┏━╸┏━┓     
╺━╸ ┃┃┣┳┛┃┣╸  ┃ ┣╸ ┣┳┛╺━╸  
   ╺┻┛╹┗╸╹╹   ╹ ┗━╸╹┗╸     

 V1.0 - A racing game for Arduboy.
 Accelerate with A, Drift with B.
*/

#include <Arduboy2.h>
#include <avr/pgmspace.h>

Arduboy2 arduboy;

// --- Game Modes & States ---
enum GameMode {
  MODE_1PLAYER,
  MODE_2PLAYER,
  MODE_LEVEL_SELECT
};

enum GameState {
  STATE_TITLE,
  STATE_MENU,
  STATE_INITIALS,
  STATE_LEVEL_SELECT,
  STATE_RACE_INTRO,
  STATE_GAME,
  STATE_PASS_PLAY,
  STATE_HIGHSCORE,
  STATE_LEAGUE_COMPLETE
};

GameMode currentMode = MODE_1PLAYER;
GameState currentState = STATE_TITLE;

uint8_t menuCursor = 0;
uint8_t levelSelectCursor = 0;
uint8_t activePlayer = 1; 
uint32_t p1RaceFrames = 0;
uint32_t p2RaceFrames = 0;

// 2-Player Overall Series Score Tracking
uint8_t p1Wins = 0;
uint8_t p2Wins = 0;

// --- Course & Track Management ---
const uint8_t TOTAL_TRACKS = 7;
uint8_t currentTrack = 0;

// --- High Score System Data ---
struct HighScoreEntry {
  char initials[4];
  uint32_t frames;
};

const uint16_t EEPROM_MAGIC_ADDRESS = EEPROM_STORAGE_SPACE_START + 30;
const uint16_t EEPROM_SCORES_ADDRESS = EEPROM_MAGIC_ADDRESS + 2;
const uint16_t EEPROM_MAGIC_VALUE = 0x4D47;

HighScoreEntry topScores[7][5];
int8_t lastPlayerRank = -1;

// --- Initials Entry State ---
char playerInitials[4] = "AAA";
uint8_t currentInitialIdx = 0;
uint8_t blinkTimer = 0;

// --- Post-Race State ---
uint16_t finishDelayFrames = 0;

// --- 3x5 Pixel Font Data ---
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
  {0x19, 0x15, 0x13}, // Z (38)
  {0x14, 0x08, 0x14}, // > (39)
  {0x00, 0x00, 0x00}  // Space default
};

void drawChar3x5(int16_t x, int16_t y, char c) {
  uint8_t idx = 12;
  if (c >= '0' && c <= '9') idx = c - '0';
  else if (c == ':') idx = 10;
  else if (c == '/') idx = 11;
  else if (c >= 'A' && c <= 'Z') idx = c - 'A' + 13;
  else if (c >= 'a' && c <= 'z') idx = c - 'a' + 13;
  else if (c == '>') idx = 39;

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
    x += 4;
    str++;
  }
}

void drawString3x5_P(int16_t x, int16_t y, const char* str) {
  char c;
  while ((c = pgm_read_byte(str++))) {
    drawChar3x5(x, y, c);
    x += 4;
  }
}

#define drawString3x5_F(x, y, str) drawString3x5_P(x, y, PSTR(str))

// --- Track Data Structures ---
struct Wall {
  int8_t x1, y1, x2, y2;
};

const Wall PROGMEM track1_walls[] = {
  { 7,   1, 121,   1}, {121,  1, 127,   7}, {127,  7, 127,  57}, {127, 57, 121,  63},
  {121, 63,   7,  63}, {  7, 63,   1,  57}, {  1, 57,   1,   7}, {  1,  7,   7,   1},
  { 38, 22,  90, 22}, { 90, 22,  96, 28}, { 96, 28,  96, 36}, { 96, 36,  90, 42},
  { 90, 42,  38, 42}, { 38, 42,  32, 36}, { 32, 36,  32, 28}, { 32, 28,  38, 22}
};

const Wall PROGMEM track2_walls[] = {
  { 0, 12, 13, 0 }, { 13, 0, 42, 0 }, { 42, 0, 56, 14 }, { 56, 14, 72, 14 },
  { 72, 14, 84, 2 }, { 84, 2, 109, 2 }, { 109, 2, 127, 20 }, { 127, 20, 127, 49 },
  { 127, 49, 111, 63 }, { 111, 63, 23, 63 }, { 23, 63, 15, 63 }, { 15, 63, 0, 48 },
  { 0, 48, 0, 12 }, { 75, 41, 34, 41 }, { 34, 41, 31, 38 }, { 31, 38, 31, 32 },
  { 31, 32, 34, 29 }, { 34, 29, 39, 29 }, { 39, 29, 42, 32 }, { 42, 32, 84, 32 },
  { 84, 32, 92, 24 }, { 92, 24, 101, 24 }, { 101, 24, 107, 30 }, { 107, 30, 107, 35 },
  { 107, 35, 101, 41 }, { 101, 41, 75, 41 }
};

const Wall PROGMEM track3_walls[] = {
  { 0, 7, 7, 0 }, { 7, 0, 57, 0 }, { 57, 0, 60, 3 }, { 60, 3, 60, 27 },
  { 60, 27, 62, 29 }, { 62, 29, 63, 28 }, { 63, 28, 63, 3 }, { 63, 3, 67, 0 },
  { 67, 0, 120, 0 }, { 120, 0, 127, 7 }, { 127, 7, 127, 57 }, { 127, 57, 121, 63 },
  { 121, 63, 7, 63 }, { 7, 63, 0, 56 }, { 0, 56, 0, 7 }, { 28, 27, 28, 34 },
  { 28, 34, 38, 44 }, { 38, 44, 94, 44 }, { 94, 44, 102, 36 }, { 102, 36, 102, 20 },
  { 102, 20, 99, 17 }
};

const Wall PROGMEM track4_walls[] = {
  { 0, 6, 7, 0 }, { 7, 0, 121, 0 }, { 121, 0, 127, 7 }, { 127, 7, 127, 58 },
  { 127, 58, 122, 63 }, { 122, 63, 7, 63 }, { 7, 63, 0, 56 }, { 0, 56, 0, 6 },
  { 65, 48, 43, 48 }, { 43, 48, 31, 36 }, { 31, 36, 31, 23 }, { 31, 23, 39, 15 },
  { 39, 15, 66, 15 }, { 102, 62, 110, 54 }, { 110, 54, 110, 43 }, { 110, 43, 98, 31 },
  { 98, 31, 66, 31 }, { 66, 31, 98, 31 }, { 98, 31, 109, 20 }, { 109, 20, 109, 8 },
  { 109, 8, 101, 0 }
};

const Wall PROGMEM track5_walls[] = {
  { 2, 27, 30, 0 }, { 30, 0, 96, 0 }, { 96, 0, 127, 31 }, { 127, 31, 127, 42 },
  { 127, 42, 106, 63 }, { 106, 63, 21, 63 }, { 21, 63, 0, 42 }, { 0, 42, 0, 29 },
  { 0, 29, 3, 26 }, { 37, 43, 32, 38 }, { 32, 38, 32, 33 }, { 32, 33, 54, 11 },
  { 54, 11, 70, 11 }, { 70, 11, 90, 31 }, { 90, 31, 90, 38 }, { 90, 38, 84, 44 },
  { 84, 44, 38, 44 }, { 38, 44, 36, 42 }, { 48, 63, 54, 57 }, { 54, 57, 72, 57 },
  { 72, 57, 78, 63 }, { 83, 45, 87, 49 }, { 87, 49, 96, 49 }, { 96, 49, 102, 43 },
  { 102, 43, 102, 38 }, { 102, 38, 99, 35 }, { 99, 35, 91, 35 }
};

const Wall PROGMEM track6_walls[] = {
  { 44, 63, 36, 63 }, { 36, 63, 29, 63 }, { 29, 63, 1, 35 }, { 1, 35, 1, 17 },
  { 1, 17, 17, 1 }, { 17, 1, 39, 1 }, { 39, 1, 55, 17 }, { 55, 17, 65, 17 },
  { 65, 17, 77, 5 }, { 77, 5, 99, 5 }, { 99, 5, 124, 30 }, { 124, 30, 124, 46 },
  { 124, 46, 107, 63 }, { 107, 63, 44, 63 }, { 34, 33, 44, 43 }, { 44, 43, 47, 46 },
  { 47, 46, 74, 46 }, { 74, 46, 87, 33 }
};

const Wall PROGMEM track7_walls[] = {
  { 87, 45, 96, 45 }, { 96, 45, 104, 37 }, { 104, 37, 104, 32 }, { 104, 32, 98, 26 },
  { 98, 26, 93, 26 }, { 93, 26, 81, 38 }, { 81, 38, 40, 38 }, { 40, 38, 32, 30 },
  { 32, 30, 32, 20 }, { 32, 20, 30, 18 }, { 30, 18, 27, 18 }, { 27, 18, 22, 23 },
  { 22, 23, 22, 37 }, { 22, 37, 32, 47 }, { 32, 47, 45, 47 }, { 45, 47, 50, 42 },
  { 50, 42, 79, 42 }, { 79, 42, 83, 46 }, { 83, 46, 95, 46 }, { 55, 64, 60, 59 },
  { 60, 59, 69, 59 }, { 69, 59, 76, 63 }, { 76, 63, 112, 63 }, { 112, 63, 127, 48 },
  { 127, 48, 127, 15 }, { 127, 15, 111, 0 }, { 111, 0, 85, 0 }, { 85, 0, 65, 20 },
  { 65, 20, 60, 20 }, { 60, 20, 55, 15 }, { 55, 15, 55, 8 }, { 55, 8, 47, 0 },
  { 47, 0, 13, 0 }, { 13, 0, 0, 14 }, { 0, 14, 0, 49 }, { 0, 49, 15, 63 },
  { 15, 63, 57, 63 }
};

const Wall* currentWallArray = track1_walls;
uint8_t currentNumWalls = sizeof(track1_walls) / sizeof(Wall);

int8_t checkPointX1 = 64, checkPointY1 = 42, checkPointX2 = 64, checkPointY2 = 63;
int8_t startLineX1  = 67, startLineY1  = 1,  startLineX2  = 67, startLineY2  = 22;

uint8_t currentLap = 1;
const uint8_t TOTAL_LAPS = 5;
bool checkpointPassed = false;
bool raceStarted = false;
bool raceFinished = false;

uint32_t totalRaceFrames = 0;

float carX = 64.0;
float carY = 12.0;
float angle = 0.0;
float angularVelocity = 0.0;
float moveX = 0.0;
float moveY = 0.0;

bool wasHandbraking = false;

void setupTrackData(uint8_t trackIdx) {
  currentTrack = trackIdx;
  switch (currentTrack) {
    case 0:
      currentWallArray = track1_walls;
      currentNumWalls = sizeof(track1_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 42; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 50; checkPointY1 = 44; checkPointX2 = 50; checkPointY2 = 57;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 1:
      currentWallArray = track2_walls;
      currentNumWalls = sizeof(track2_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 42; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 50; checkPointY1 = 44; checkPointX2 = 50; checkPointY2 = 57;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 2: // Now Original Track 6
      currentWallArray = track6_walls;
      currentNumWalls = sizeof(track6_walls) / sizeof(Wall);
      startLineX1 = 50; startLineY1 = 48; startLineX2 = 50; startLineY2 = 63;
      checkPointX1 = 64; checkPointY1 = 17; checkPointX2 = 64; checkPointY2 = 46;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 3: // Now Original Track 3
      currentWallArray = track3_walls;
      currentNumWalls = sizeof(track3_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 45; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 80; checkPointY1 = 42; checkPointX2 = 64; checkPointY2 = 63;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 4:
      currentWallArray = track5_walls;
      currentNumWalls = sizeof(track5_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 45; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 64; checkPointY1 = 0; checkPointX2 = 880; checkPointY2 = 25;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 5: // Now Original Track 4
      currentWallArray = track4_walls;
      currentNumWalls = sizeof(track4_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 45; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 64; checkPointY1 = 42; checkPointX2 = 64; checkPointY2 = 63;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;

    case 6:
      currentWallArray = track7_walls;
      currentNumWalls = sizeof(track7_walls) / sizeof(Wall);
      startLineX1 = 40; startLineY1 = 45; startLineX2 = 40; startLineY2 = 63;
      checkPointX1 = 50; checkPointY1 = 44; checkPointX2 = 50; checkPointY2 = 57;
      carX = 20.0; carY = 52.0; angle = 0.0;
      break;
  }
}

void saveHighScoresToEEPROM() {
  EEPROM.put(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC_VALUE);
  EEPROM.put(EEPROM_SCORES_ADDRESS, topScores);
}

const char PROGMEM defaultInitials[][4] = { "ACE", "MAX", "DRI", "SPD", "BOT" };

void loadHighScoresFromEEPROM() {
  uint16_t magic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDRESS, magic);

  if (magic == EEPROM_MAGIC_VALUE) {
    EEPROM.get(EEPROM_SCORES_ADDRESS, topScores);
  } else {
    for (uint8_t t = 0; t < TOTAL_TRACKS; t++) {
      for (uint8_t i = 0; i < 5; i++) {
        memcpy_P(topScores[t][i].initials, defaultInitials[i], 4);
        topScores[t][i].frames = (5400 + (i * 600)) + (t * 300);
      }
    }
    saveHighScoresToEEPROM();
  }
}

void insertHighScore(const char* initials, uint32_t scoreFrames) {
  lastPlayerRank = -1;
  for (int8_t i = 0; i < 5; i++) {
    if (scoreFrames < topScores[currentTrack][i].frames) {
      for (int8_t j = 4; j > i; j--) {
        topScores[currentTrack][j] = topScores[currentTrack][j - 1];
      }
      topScores[currentTrack][i].initials[0] = initials[0];
      topScores[currentTrack][i].initials[1] = initials[1];
      topScores[currentTrack][i].initials[2] = initials[2];
      topScores[currentTrack][i].initials[3] = '\0';
      topScores[currentTrack][i].frames = scoreFrames;

      lastPlayerRank = i;
      saveHighScoresToEEPROM();
      break;
    }
  }
}

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
  finishDelayFrames = 0;
  angularVelocity = 0.0;
  moveX = 0.0;
  moveY = 0.0;
  setupTrackData(currentTrack);
}

void drawTrackAndHUD() {
  for (uint8_t i = 0; i < currentNumWalls; i++) {
    Wall w;
    memcpy_P(&w, &currentWallArray[i], sizeof(Wall));
    arduboy.drawLine(w.x1, w.y1, w.x2, w.y2, WHITE);
  }
  
  arduboy.drawLine(startLineX1, startLineY1, startLineX2, startLineY2, WHITE);

  // Determine dynamic UI positions based on track layout
  int16_t hudX = 36;
  int16_t hudY = 28;

  switch (currentTrack) {
    case 0: // Track 1
      hudX = 38; hudY = 30;
      break;
    case 1: // Track 2
      hudX = 41; hudY = 34;
      break;
    case 2: // Track 3
      hudX = 60; hudY = 0;
      break;
    case 3: // Track 4
      hudX = 7;  hudY = 2;
      break;
    case 4: // Track 5 (formerly Track 6)
      hudX = 34; hudY = 33;
      break;
    case 5: // Track 6 (formerly Track 7)
      hudX = 65; hudY = 0;
      break;
    case 6: // Track 7 (formerly Track 8)
      hudX = 60; hudY = 0;
      break;
  }

  char timeBuf[6];
  formatTime(totalRaceFrames, timeBuf);

  if (raceFinished) {
    drawString3x5_F(hudX, hudY, "FINISHED");
    drawString3x5(hudX + 36, hudY, timeBuf);
  } else if (!raceStarted) {
    drawString3x5_F(hudX, hudY, "PRESS A TO START");
  } else {
    char hudBuf[16];
    hudBuf[0] = 'L'; hudBuf[1] = 'A'; hudBuf[2] = 'P'; hudBuf[3] = ' ';
    hudBuf[4] = '0' + currentLap; hudBuf[5] = '/'; hudBuf[6] = '0' + TOTAL_LAPS;
    hudBuf[7] = ' '; hudBuf[8] = ' ';
    memcpy(hudBuf + 9, timeBuf, 6);
    hudBuf[15] = '\0';

    drawString3x5(hudX, hudY, hudBuf);
  }
}

bool resolveWallCollision(float &x, float &y, float &vx, float &vy) {
  bool collided = false;

  for (uint8_t i = 0; i < currentNumWalls; i++) {
    Wall w;
    memcpy_P(&w, &currentWallArray[i], sizeof(Wall));

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
    menuCursor = 0;
    currentState = STATE_MENU;
  }

  for (int x = 0; x < 128; x += 4) {
    arduboy.fillRect(x, 0, 2, 2, WHITE);
    arduboy.fillRect(x + 2, 2, 2, 2, WHITE);
    arduboy.fillRect(x, 60, 2, 2, WHITE);
    arduboy.fillRect(x + 2, 62, 2, 2, WHITE);
  }
  for (int y = 0; y < 64; y += 4) {
    arduboy.fillRect(0, y, 2, 2, WHITE);
    arduboy.fillRect(2, y + 2, 2, 2, WHITE);
    arduboy.fillRect(124, y, 2, 2, WHITE);
    arduboy.fillRect(126, y + 2, 2, 2, WHITE);
  }

  arduboy.setTextSize(2);
  arduboy.setCursor(10, 10);
  arduboy.print(F("MOTODROME"));
  arduboy.setCursor(22, 28);
  arduboy.print(F("DRIFTER"));
  arduboy.setTextSize(1);

  blinkTimer++;
  if ((blinkTimer / 30) % 2 == 0) {
    drawString3x5_F(30, 48, "PRESS ANY BUTTON");
  }
}

// --- Mode Menu Screen Handler ---
void updateMenuScreen() {
  if (arduboy.justPressed(UP_BUTTON)) {
    if (menuCursor > 0) menuCursor--;
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    if (menuCursor < 2) menuCursor++;
  }

  if (arduboy.justPressed(A_BUTTON)) {
    if (menuCursor == 0) {
      currentMode = MODE_1PLAYER;
      currentState = STATE_INITIALS;
      currentInitialIdx = 0;
    } else if (menuCursor == 1) {
      currentMode = MODE_2PLAYER;
      activePlayer = 1;
      currentTrack = 0;
      p1Wins = 0;
      p2Wins = 0;
      currentState = STATE_RACE_INTRO;
    } else if (menuCursor == 2) {
      currentMode = MODE_LEVEL_SELECT;
      levelSelectCursor = 0;
      currentState = STATE_LEVEL_SELECT;
    }
  }

  drawString3x5_F(38, 8, "SELECT MODE");
  drawString3x5_F(40, 24, "1 PLAYER");
  drawString3x5_F(40, 36, "2 PLAYER");
  drawString3x5_F(40, 48, "LEVEL SELECT");

  drawString3x5_F(30, 24 + (menuCursor * 12), ">");
}

// --- Level Select Screen Handler ---
void updateLevelSelectScreen() {
  if (arduboy.justPressed(UP_BUTTON)) {
    if (levelSelectCursor > 0) levelSelectCursor--;
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    if (levelSelectCursor < TOTAL_TRACKS - 1) levelSelectCursor++;
  }

  if (arduboy.justPressed(A_BUTTON)) {
    currentTrack = levelSelectCursor;
    currentState = STATE_RACE_INTRO;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    currentState = STATE_MENU;
  }

  drawString3x5_F(38, 4, "SELECT LEVEL");

  uint8_t startIdx = (levelSelectCursor > 4) ? levelSelectCursor - 4 : 0;
  uint8_t endIdx = (startIdx + 5 < TOTAL_TRACKS) ? startIdx + 5 : TOTAL_TRACKS;

  for (uint8_t i = startIdx; i < endIdx; i++) {
    int16_t rowY = 16 + ((i - startIdx) * 9);
    char trackBuf[10];
    trackBuf[0] = 'C'; trackBuf[1] = 'O'; trackBuf[2] = 'U'; trackBuf[3] = 'R'; trackBuf[4] = 'S'; trackBuf[5] = 'E'; trackBuf[6] = ' ';
    trackBuf[7] = '1' + i; trackBuf[8] = '\0';

    if (i == levelSelectCursor) {
      drawString3x5_F(32, rowY, ">");
    }
    drawString3x5(42, rowY, trackBuf);
  }
}

// --- Initials Input Screen Handler ---
void updateInitialsScreen() {
  if (arduboy.justPressed(UP_BUTTON)) {
    playerInitials[currentInitialIdx]--;
    if (playerInitials[currentInitialIdx] < 'A') playerInitials[currentInitialIdx] = 'Z';
  }
  if (arduboy.justPressed(DOWN_BUTTON)) {
    playerInitials[currentInitialIdx]++;
    if (playerInitials[currentInitialIdx] > 'Z') playerInitials[currentInitialIdx] = 'A';
  }

  if (arduboy.justPressed(A_BUTTON)) {
    currentInitialIdx++;
    if (currentInitialIdx >= 3) {
      currentTrack = 0;
      currentState = STATE_RACE_INTRO;
      return;
    }
  }

  if (arduboy.justPressed(B_BUTTON) && currentInitialIdx > 0) {
    currentInitialIdx--;
  }

  drawString3x5_F(34, 14, "ENTER INITIALS");

  for (uint8_t i = 0; i < 3; i++) {
    int16_t charX = 52 + (i * 10);
    drawChar3x5(charX, 30, playerInitials[i]);

    if (i == currentInitialIdx) {
      if ((blinkTimer / 15) % 2 == 0) {
        arduboy.drawFastHLine(charX, 37, 3, WHITE);
      }
    }
  }

  drawString3x5_F(22, 50, "UP/DN: CHOOSE  A: NEXT");
  blinkTimer++;
}

// --- Race Intro Screen Handler ---
void updateRaceIntroScreen() {
  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON) ||
      arduboy.justPressed(UP_BUTTON) || arduboy.justPressed(DOWN_BUTTON) ||
      arduboy.justPressed(LEFT_BUTTON) || arduboy.justPressed(RIGHT_BUTTON)) {
    resetRace();
    currentState = STATE_GAME;
    return;
  }

  char raceBuf[11];
  raceBuf[0] = 'C'; raceBuf[1] = 'O'; raceBuf[2] = 'U'; raceBuf[3] = 'R'; raceBuf[4] = 'S'; raceBuf[5] = 'E'; raceBuf[6] = ' ';
  raceBuf[7] = '1' + currentTrack; raceBuf[8] = '/'; raceBuf[9] = '0' + TOTAL_TRACKS; raceBuf[10] = '\0';

  drawString3x5(40, 16, raceBuf);

  if (currentMode == MODE_2PLAYER) {
    char pBuf[10];
    pBuf[0] = 'P'; pBuf[1] = 'L'; pBuf[2] = 'A'; pBuf[3] = 'Y'; pBuf[4] = 'E'; pBuf[5] = 'R'; pBuf[6] = ' ';
    pBuf[7] = '0' + activePlayer; pBuf[8] = '\0';
    drawString3x5(46, 28, pBuf);
  }

  drawString3x5_F(20, 44, "ACCELERATE A, DRIFT B");

  blinkTimer++;
  if ((blinkTimer / 30) % 2 == 0) {
    drawString3x5_F(26, 56, "PRESS ANY BUTTON");
  }
}

// --- Pass & Play Transition Screen ---
void updatePassPlayScreen() {
  drawString3x5_F(48, 20, "PLAYER 2");
  drawString3x5_F(32, 42, "PRESS A TO START");

  if (arduboy.justPressed(A_BUTTON)) {
    activePlayer = 2;
    currentState = STATE_RACE_INTRO;
  }
}

// --- League Complete Screen Handler ---
void updateLeagueCompleteScreen() {
  if (currentMode == MODE_2PLAYER) {
    drawString3x5_F(34, 16, "SERIES COMPLETE!");

    char scoreBuf[14];
    scoreBuf[0] = 'P'; scoreBuf[1] = '1'; scoreBuf[2] = ':'; scoreBuf[3] = ' ';
    scoreBuf[4] = '0' + p1Wins; scoreBuf[5] = ' '; scoreBuf[6] = ' ';
    scoreBuf[7] = 'P'; scoreBuf[8] = '2'; scoreBuf[9] = ':'; scoreBuf[10] = ' ';
    scoreBuf[11] = '0' + p2Wins; scoreBuf[12] = '\0';
    drawString3x5(38, 28, scoreBuf);

    if (p1Wins > p2Wins) {
      drawString3x5_F(22, 40, "PLAYER 1 IS CHAMPION!");
    } else if (p2Wins > p1Wins) {
      drawString3x5_F(22, 40, "PLAYER 2 IS CHAMPION!");
    } else {
      drawString3x5_F(34, 40, "SERIES ENDS IN A TIE!");
    }
  } else {
    drawString3x5_F(6, 20, "CONGRATULATIONS FOR FINISHING");
    drawString3x5_F(42, 30, "THE LEAGUE!");
  }
  
  blinkTimer++;
  if ((blinkTimer / 30) % 2 == 0) {
    drawString3x5_F(34, 54, "PRESS TO END");
  }

  if (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON)) {
    currentState = STATE_TITLE;
  }
}

// --- High Score & Post-Race Leaderboard Handler ---
void updateHighScoreScreen() {
  if (currentMode == MODE_2PLAYER) {
    char p1ScoreBuf[6];
    p1ScoreBuf[0] = 'P'; p1ScoreBuf[1] = '1'; p1ScoreBuf[2] = ':'; p1ScoreBuf[3] = '0' + p1Wins; p1ScoreBuf[4] = '\0';
    drawString3x5(2, 2, p1ScoreBuf);

    char p2ScoreBuf[6];
    p2ScoreBuf[0] = 'P'; p2ScoreBuf[1] = '2'; p2ScoreBuf[2] = ':'; p2ScoreBuf[3] = '0' + p2Wins; p2ScoreBuf[4] = '\0';
    drawString3x5(108, 2, p2ScoreBuf);

    drawString3x5_F(36, 10, "RACE RESULT");

    char p1Buf[11], p2Buf[11];
    char t1[6], t2[6];
    formatTime(p1RaceFrames, t1);
    formatTime(p2RaceFrames, t2);

    p1Buf[0] = 'P'; p1Buf[1] = '1'; p1Buf[2] = ':'; p1Buf[3] = ' ';
    memcpy(p1Buf + 4, t1, 6);
    p1Buf[10] = '\0';

    p2Buf[0] = 'P'; p2Buf[1] = '2'; p2Buf[2] = ':'; p2Buf[3] = ' ';
    memcpy(p2Buf + 4, t2, 6);
    p2Buf[10] = '\0';

    drawString3x5(38, 22, p1Buf);
    drawString3x5(38, 32, p2Buf);

    bool flashText = ((blinkTimer / 15) % 2 == 0);

    if (p1RaceFrames < p2RaceFrames) {
      if (flashText) drawString3x5_F(34, 44, "PLAYER 1 WINS!");
    } else if (p2RaceFrames < p1RaceFrames) {
      if (flashText) drawString3x5_F(34, 44, "PLAYER 2 WINS!");
    } else {
      if (flashText) drawString3x5_F(46, 44, "TIE RACE!");
    }

    drawString3x5_F(30, 56, "PRESS A TO NEXT");
    blinkTimer++;

    if (arduboy.justPressed(A_BUTTON)) {
      if (currentTrack < TOTAL_TRACKS - 1) {
        currentTrack++;
        activePlayer = 1;
        currentState = STATE_RACE_INTRO;
      } else {
        currentState = STATE_LEAGUE_COMPLETE;
      }
    }
    return;
  }

  char titleBuf[20];
  titleBuf[0] = 'T'; titleBuf[1] = 'R'; titleBuf[2] = 'A'; titleBuf[3] = 'C'; titleBuf[4] = 'K'; titleBuf[5] = ' ';
  titleBuf[6] = '1' + currentTrack; titleBuf[7] = ' '; titleBuf[8] = 'H'; titleBuf[9] = 'I'; titleBuf[10] = 'G';
  titleBuf[11] = 'H'; titleBuf[12] = 'S'; titleBuf[13] = 'C'; titleBuf[14] = 'O'; titleBuf[15] = 'R'; titleBuf[16] = 'E'; titleBuf[17] = 'S'; titleBuf[18] = '\0';

  drawString3x5(24, 4, titleBuf);
  arduboy.drawFastHLine(8, 11, 112, WHITE);

  for (uint8_t i = 0; i < 5; i++) {
    int16_t rowY = 14 + (i * 7);

    char rankStr[3];
    rankStr[0] = '1' + i; rankStr[1] = '.'; rankStr[2] = '\0';
    drawString3x5(24, rowY, rankStr);

    char timeBuf[6];
    formatTime(topScores[currentTrack][i].frames, timeBuf);

    if (i == lastPlayerRank) {
      if ((blinkTimer / 15) % 2 == 0) {
        drawString3x5_F(14, rowY, ">");
        drawString3x5(42, rowY, topScores[currentTrack][i].initials);
        drawString3x5(76, rowY, timeBuf);
      }
    } else {
      drawString3x5(42, rowY, topScores[currentTrack][i].initials);
      drawString3x5(76, rowY, timeBuf);
    }
  }

  drawString3x5_F(16, 56, "A: CONTINUE   B: RETRY");
  blinkTimer++;

  if (arduboy.justPressed(A_BUTTON)) {
    if (currentMode == MODE_LEVEL_SELECT) {
      currentState = STATE_LEVEL_SELECT;
    } else {
      if (currentTrack < TOTAL_TRACKS - 1) {
        currentTrack++;
        currentState = STATE_RACE_INTRO;
      } else {
        currentState = STATE_LEAGUE_COMPLETE;
      }
    }
  } else if (arduboy.justPressed(B_BUTTON)) {
    currentState = STATE_RACE_INTRO;
  }
}

// --- Main Race Loop Handler ---
void updateGameScreen() {
  bool isHandbraking = arduboy.pressed(B_BUTTON);
  bool isReversing = arduboy.pressed(UP_BUTTON);
  bool isAccelerating = arduboy.pressed(A_BUTTON);

  if (!raceStarted && arduboy.justPressed(A_BUTTON)) {
    raceStarted = true;
  }

  if (raceStarted && !raceFinished) {
    totalRaceFrames++;
  }

  if (raceFinished) {
    finishDelayFrames++;
    if (finishDelayFrames >= 120) {
      if (currentMode == MODE_2PLAYER) {
        if (activePlayer == 1) {
          p1RaceFrames = totalRaceFrames;
          currentState = STATE_PASS_PLAY;
        } else {
          p2RaceFrames = totalRaceFrames;
          
          if (p1RaceFrames < p2RaceFrames) {
            p1Wins++;
          } else if (p2RaceFrames < p1RaceFrames) {
            p2Wins++;
          }
          
          currentState = STATE_HIGHSCORE;
        }
      } else {
        insertHighScore(playerInitials, totalRaceFrames);
        blinkTimer = 0;
        currentState = STATE_HIGHSCORE;
      }
      return;
    }
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
  if (isAccelerating && raceStarted && !raceFinished) {
    float accel = 0.05;
    moveX += cos(angle) * accel;
    moveY += sin(angle) * accel;
  } else if (isReversing && raceStarted && !raceFinished) {
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
  delay(500); 
  arduboy.begin();
  arduboy.setFrameRate(60);
  loadHighScoresFromEEPROM();
}

void loop() {
  if (!arduboy.nextFrame()) return;

  arduboy.pollButtons();
  arduboy.clear();

  switch (currentState) {
    case STATE_TITLE:
      updateTitleScreen();
      break;
    case STATE_MENU:
      updateMenuScreen();
      break;
    case STATE_INITIALS:
      updateInitialsScreen();
      break;
    case STATE_LEVEL_SELECT:
      updateLevelSelectScreen();
      break;
    case STATE_RACE_INTRO:
      updateRaceIntroScreen();
      break;
    case STATE_GAME:
      updateGameScreen();
      break;
    case STATE_PASS_PLAY:
      updatePassPlayScreen();
      break;
    case STATE_HIGHSCORE:
      updateHighScoreScreen();
      break;
    case STATE_LEAGUE_COMPLETE:
      updateLeagueCompleteScreen();
      break;
  }

  arduboy.display();
}