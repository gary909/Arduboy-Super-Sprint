#include <Arduboy2.h>

//added better car physics (b-button now adds drift)

Arduboy2 arduboy;

// Car state variables
float carX = 64.0;
float carY = 32.0;
float angle = 0.0;           // Facing direction (radians)
float angularVelocity = 0.0; // Rotational inertia

// World movement velocity vector
float moveX = 0.0;
float moveY = 0.0;

bool wasHandbraking = false; // Track button state for brake-tap speed penalty

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
}

void loop() {
  if (!arduboy.nextFrame()) return;

  arduboy.pollButtons();

  bool isHandbraking = arduboy.pressed(B_BUTTON);
  
  // 1. Handbrake Tap Drag (Cuts speed instantly on drift entry)
  if (isHandbraking && !wasHandbraking) {
    moveX *= 0.85;
    moveY *= 0.85;
  }
  wasHandbraking = isHandbraking;

  // 2. Angular Inertia & Turn Rate
  float turnAccel = isHandbraking ? 0.028 : 0.012; 
  
  if (arduboy.pressed(LEFT_BUTTON))  angularVelocity -= turnAccel;
  if (arduboy.pressed(RIGHT_BUTTON)) angularVelocity += turnAccel;

  // Clamp max spin rate
  float maxAngularVel = isHandbraking ? 0.075 : 0.09;
  if (angularVelocity > maxAngularVel)  angularVelocity = maxAngularVel;
  if (angularVelocity < -maxAngularVel) angularVelocity = -maxAngularVel;

  // Apply rotational inertia
  angle += angularVelocity;

  // Rotational friction while handbraking
  float rotationalDamping = isHandbraking ? 0.80 : 0.75;
  if (isHandbraking && !arduboy.pressed(LEFT_BUTTON) && !arduboy.pressed(RIGHT_BUTTON)) {
    rotationalDamping = 0.65;
  }
  angularVelocity *= rotationalDamping;

  // 3. Linear Acceleration
  if (arduboy.pressed(A_BUTTON)) {
    float accel = 0.05; // Slightly lower acceleration impulse
    moveX += cos(angle) * accel;
    moveY += sin(angle) * accel;
  }

  // // 4. Ultra Slidey Drift Physics
  // if (isHandbraking) {
  //   // 0.995 holds momentum even longer during long slides
  //   moveX *= 0.995;
  //   moveY *= 0.995;
  // } else {
  //   float forwardSpeed = sqrt(moveX * moveX + moveY * moveY);
    
  //   // Friction on total momentum
  //   moveX *= 0.96;
  //   moveY *= 0.96;

  //   // Blend current trajectory towards facing angle
  //   float targetMoveX = cos(angle) * forwardSpeed;
  //   float targetMoveY = sin(angle) * forwardSpeed;
    
  //   // 0.03 re-grip factor makes traction return very gradually
  //   moveX = (moveX * 0.97) + (targetMoveX * 0.03);
  //   moveY = (moveY * 0.97) + (targetMoveY * 0.03);
  // }

  // // 5. Cap Max Velocity (Reduced from 1.8 to 1.2)
  // float currentSpeed = sqrt(moveX * moveX + moveY * moveY);
  // if (currentSpeed > 1.0) {
  //   moveX = (moveX / currentSpeed) * 1.0;
  //   moveY = (moveY / currentSpeed) * 1.0;
  // }

  // 4. Ultra Slidey Drift Physics
  if (isHandbraking) {
    // 0.998 holds momentum almost indefinitely during long slides
    moveX *= 0.998;
    moveY *= 0.998;
  } else {
    float forwardSpeed = sqrt(moveX * moveX + moveY * moveY);
    
    // Friction on total momentum
    moveX *= 0.96;
    moveY *= 0.96;

    // Blend current trajectory towards facing angle
    float targetMoveX = cos(angle) * forwardSpeed;
    float targetMoveY = sin(angle) * forwardSpeed;
    
    // 0.015 re-grip factor gives an extremely low-friction, icy feel
    moveX = (moveX * 0.985) + (targetMoveX * 0.015);
    moveY = (moveY * 0.985) + (targetMoveY * 0.015);
  }

  // 5. Cap Max Velocity
  float currentSpeed = sqrt(moveX * moveX + moveY * moveY);
  if (currentSpeed > 1.0) {
    moveX = (moveX / currentSpeed) * 1.0;
    moveY = (moveY / currentSpeed) * 1.0;
  }

  // 6. Update Position & Screen Wrap
  carX += moveX;
  carY += moveY;

  if (carX < 0) carX = 128;
  if (carX > 128) carX = 0;
  if (carY < 0) carY = 64;
  if (carY > 64) carY = 0;

  // 7. Render Frame
  arduboy.clear();
  
  int endX = carX + cos(angle) * 6;
  int endY = carY + sin(angle) * 6;
  arduboy.drawLine((int)carX, (int)carY, endX, endY, WHITE);

  arduboy.display();
}