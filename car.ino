#include <Servo.h>

#define servoPin 9
#define reversePin 8
#define dacPin DAC0
#define relay1 7
#define relay2 6
#define relay3 5

#define timeoutLimit 1500
#define shiftDelay 1000
#define brakeDelay 300

#define motorMinDAC 410
#define motorMaxDAC 1023

#define servoOffset 55
#define servoMin 0
#define servoMax 180

Servo myservo;
int currentPos = 90;
int currentSpeed = 0;
int currentDir = 0;
bool isBraking = true;

unsigned long lastInputTime = 0;
unsigned long shiftTimer = 0;
unsigned long brakeTimer = 0;

String bleBuffer = "";
String usbBuffer = "";

void setup() {
  myservo.attach(servoPin);
  pinMode(reversePin, OUTPUT);
  digitalWrite(reversePin, LOW);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);

  setBraking(isBraking);
  myservo.write(currentPos);
  analogWriteResolution(10);
  analogWrite(dacPin, currentSpeed);

  Serial.begin(9600);
  Serial1.begin(9600);
  lastInputTime = millis();

  Serial.println("Init");
}

void loop() {
  // Read USB
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') { processInput(usbBuffer); usbBuffer = ""; }
    else { usbBuffer += c; }
  }

  // Read BLE
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == '\n') { processInput(bleBuffer); bleBuffer = ""; }
    else { bleBuffer += c; }
  }

  // Safety Timeout
  if (millis() - lastInputTime > timeoutLimit) {
    if (currentPos != 90 || currentSpeed != 0 || !isBraking) {
      executeMovement(90, 0, 0);
      Serial.println("SAFETY TIMEOUT");
    }
    lastInputTime = millis();
  }
}

void processInput(String input) {
  int firstSemi = input.indexOf(';');
  int secondSemi = input.lastIndexOf(';');

  if (firstSemi != -1 && secondSemi != -1 && firstSemi != secondSemi) {
    int angle = input.substring(0, firstSemi).toInt();
    int speed = input.substring(firstSemi + 1, secondSemi).toInt();
    int dir = input.substring(secondSemi + 1).toInt();

    lastInputTime = millis();
    executeMovement(constrain(angle, 0, 180), constrain(speed, 0, 100), dir);
  }
}

void executeMovement(int angle, int speedPercent, int direction) {
  // 1. Direction Shift Safety
  if (direction != 0 && direction != currentDir) {
    analogWrite(dacPin, 0);
    setBraking(true);
    digitalWrite(reversePin, (direction == 1));
    currentDir = direction;
    shiftTimer = millis();
  }

  // 2. Determine if braking
  bool wantBrake = (direction == 0 || speedPercent < 2);
  if (wantBrake) {
    if (brakeTimer == 0) brakeTimer = millis();
    if (millis() - brakeTimer > brakeDelay) {
      setBraking(true);
    }
    speedPercent = 0;
  } else {
    setBraking(false);
    brakeTimer = 0;
  }

  // 3. Apply Time Lockouts
  if (millis() - shiftTimer < shiftDelay) speedPercent = 0;

  // 4. Set Hardware Outputs
  if (angle != currentPos) {
    int calibratedAngle = map(angle, servoMin, servoMax, servoMin + servoOffset, servoMax - servoOffset);
    myservo.write(calibratedAngle);
    currentPos = angle;
  }
  int dacValue = (!isBraking && speedPercent > 0) ? map(speedPercent, 1, 100, motorMinDAC, motorMaxDAC) : 0;
  analogWrite(dacPin, dacValue);
  currentSpeed = speedPercent;

  // 5. Feedback
  char msg[50];
  snprintf(msg, sizeof(msg), "A:%d S:%d D:%d B:%d", angle, speedPercent, direction, (int)isBraking);
  Serial.println(msg);
  Serial1.println(msg);
}

void setBraking(bool active) {
  isBraking = active;
  int state = !active ? HIGH : LOW;
  digitalWrite(relay1, state);
  digitalWrite(relay2, state);
  digitalWrite(relay3, state);
}
