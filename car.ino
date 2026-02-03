#include <Servo.h>

#define servoPin 9
#define reversePin 8
#define dacPin DAC0

Servo myservo;
int currentPos = 90;
int currentSpeed = 0;
int currentDir = 0;

unsigned long lastInputTime = 0;
unsigned long shiftTimer = 0;
const unsigned long timeoutLimit = 5000;
const unsigned long shiftDelay = 1000;

const int motorMinDAC = 410;
const int motorMaxDAC = 1023;

String bleBuffer = "";
String usbBuffer = "";

void setup() {
  myservo.attach(servoPin);
  pinMode(reversePin, OUTPUT);
  digitalWrite(reversePin, LOW);

  myservo.write(currentPos);
  analogWriteResolution(10);
  analogWrite(dacPin, currentSpeed);

  Serial.begin(9600);
  Serial1.begin(9600);
  lastInputTime = millis();

  Serial.println("Init");
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') { processInput(usbBuffer); usbBuffer = ""; }
    else { usbBuffer += c; }
  }

  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == '\n') { processInput(bleBuffer); bleBuffer = ""; }
    else { bleBuffer += c; }
  }

  if (millis() - lastInputTime > timeoutLimit) {
    if (currentPos != 90 || currentSpeed != 0) {
      executeMovement(90, 0, 1);
      Serial.println("Idling");
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
  int targetDir = (direction <= -1) ? -1 : 1;

  if (targetDir != currentDir) {
    analogWrite(dacPin, 0);
    digitalWrite(reversePin, (targetDir == 1) ? HIGH : LOW);
    currentDir = targetDir;
    shiftTimer = millis();
  }

  if (millis() - shiftTimer < shiftDelay) {
    speedPercent = 0;
  }

  if (angle != currentPos) {
    myservo.write(angle);
    currentPos = angle;
  }

  int dacValue = (speedPercent > 0) ? map(speedPercent, 1, 100, motorMinDAC, motorMaxDAC) : 0;
  analogWrite(dacPin, dacValue);
  currentSpeed = speedPercent;

  String msg = "A:" + String(angle) + " S:" + String(speedPercent) + " D:" + String(targetDir);
  if (speedPercent == 0 && (millis() - shiftTimer < shiftDelay)) msg += " [SHIFTING]";
  Serial.println(msg);
  Serial1.println(msg);
}
