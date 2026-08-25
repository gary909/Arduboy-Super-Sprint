#include <Arduboy2.h>
#include <avr/pgmspace.h>

// V3 - Starting to add courses / Added Up button for Brake and Down button for reverse
// V2 added better car physics (b-button now adds drift)

Arduboy2 arduboy;

// Track Data Structures
struct Wall {
  int8_t x1, y1, x2, y2;
};

// Sprint-style track layout with rounded (chamfered) outer and inner corners
const Wall PROGMEM track1_walls[] = {
  // Outer boundary walls (chamfered corners)
  { 7,   1, 121,   1}, // Top
  {121,  1, 127,   7}, // Top-Right Diagonal
  {127,  7, 127,  57}, // Right
  {127, 57, 121,  63}, // Bottom-Right Diagonal
  {121, 63,   7,  63}, // Bottom
  {  7, 63,   1,  57}, // Bottom-Left Diagonal
  {  1, 57,   1,   7}, // Left
  {  1,  7,   7,   1}, // Top-Left Diagonal

  // Inner island with chamfered corners
  { 38, 22,  90, 22}, // Top
  { 90, 22,  96, 28}, // Top-Right Diagonal
  { 96, 28,  96, 36}, // Right
  { 96, 36,  90, 42}, // Bottom-Right Diagonal
  { 90, 42,  38, 42}, // Bottom
  { 38, 42,  32, 36}, // Bottom-Left Diagonal
  { 32, 36,  32, 28}, // Left
  { 32, 28,  38, 22}  // Top-Left Diagonal
};

const uint8_t NUM_WALLS = sizeof(track1_walls) / sizeof(Wall);

// Car State Variables
float carX = 64.0;
float carY = 12.0;
float angle = 0.0;
float angularVelocity = 0.0;

float moveX = 0.0;
float moveY = 0.0;

bool wasHandbraking = false;

void drawTrack() {
  for (uint8_t i = 0; i < NUM_WALLS; i++) {
    Wall w;
    memcpy_P(&w, &track1_walls[i], sizeof(Wall));
    arduboy.drawLine(w.x1, w.y1, w.x2, w.y2, WHITE);
  }
  arduboy.drawFastVLine(67, 1, 20, WHITE);
}

// Wall Collision Resolution
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
    
    float radius = 3.0; // Collision distance threshold

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

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
}

void loop() {
  if (!arduboy.nextFrame()) return;

  arduboy.pollButtons();

  bool isHandbraking = arduboy.pressed(B_BUTTON);
  bool isReversing = arduboy.pressed(UP_BUTTON);
  
  // 1. Handbrake Tap Drag
  if (isHandbraking && !wasHandbraking) {
    moveX *= 0.85;
    moveY *= 0.85;
  }
  wasHandbraking = isHandbraking;

  // 2. Angular Inertia & Turn Rate
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

  // 3. Acceleration, Reverse, and Footbrake
  if (arduboy.pressed(A_BUTTON)) {
    // Forward Acceleration
    float accel = 0.05;
    moveX += cos(angle) * accel;
    moveY += sin(angle) * accel;
  } else if (isReversing) {
    // Reverse Acceleration (opposite vector)
    float revAccel = 0.03;
    moveX -= cos(angle) * revAccel;
    moveY -= sin(angle) * revAccel;
  }

  if (arduboy.pressed(DOWN_BUTTON)) {
    // Footbrake (rapid deceleration)
    moveX *= 0.85;
    moveY *= 0.85;
  }

  // 4. Drift Physics & Friction
  if (isHandbraking) {
    moveX *= 0.998;
    moveY *= 0.998;
  } else {
    float forwardSpeed = sqrt(moveX * moveX + moveY * moveY);
    moveX *= 0.96;
    moveY *= 0.96;

    // Direct alignment toward facing angle (supports negative speed direction)
    float dotProduct = moveX * cos(angle) + moveY * sin(angle);
    float direction = (dotProduct >= 0) ? 1.0 : -1.0;

    float targetMoveX = cos(angle) * forwardSpeed * direction;
    float targetMoveY = sin(angle) * forwardSpeed * direction;
    
    moveX = (moveX * 0.985) + (targetMoveX * 0.015);
    moveY = (moveY * 0.985) + (targetMoveY * 0.015);
  }

  // 5. Cap Max Velocity (1.0 Forward, 0.5 Reverse)
  float currentSpeed = sqrt(moveX * moveX + moveY * moveY);
  float maxAllowedSpeed = isReversing ? 0.5 : 1.0;

  if (currentSpeed > maxAllowedSpeed) {
    moveX = (moveX / currentSpeed) * maxAllowedSpeed;
    moveY = (moveY / currentSpeed) * maxAllowedSpeed;
  }

  // 6. Position Update & Collision Resolution
  carX += moveX;
  carY += moveY;
  resolveWallCollision(carX, carY, moveX, moveY);

  // 7. Render Frame
  arduboy.clear();
  drawTrack();
  
  int endX = carX + cos(angle) * 6;
  int endY = carY + sin(angle) * 6;
  arduboy.drawLine((int)carX, (int)carY, endX, endY, WHITE);

  arduboy.display();
}