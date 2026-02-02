#include <Servo.h>

#define servoPin 9
#define dacPin DAC0  

Servo myservo;
int currentPos = 90;
unsigned long lastInputTime = 0;
const unsigned long timeoutLimit = 5000;

const int motorMinDAC = 410;
const int motorMaxDAC = 1023;

void setup() {
  myservo.attach(servoPin);
  analogWriteResolution(10); 
  
  myservo.write(currentPos);
  analogWrite(dacPin, 0); 

  Serial.begin(9600);
  Serial.println("Init.");
  lastInputTime = millis();
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int separatorIndex = input.indexOf(';');

    if (separatorIndex != -1) {
      int targetAngle = input.substring(0, separatorIndex).toInt();
      int speedInput = input.substring(separatorIndex + 1).toInt();

      targetAngle = constrain(targetAngle, 0, 180);
      speedInput = constrain(speedInput, 0, 100);

      executeMovement(targetAngle, speedInput);
      lastInputTime = millis();
    }
  }

  if (millis() - lastInputTime > timeoutLimit) {
    Serial.println("Timeout: Resetting to Idle...");
    executeMovement(90, 0);
    lastInputTime = millis(); 
  }
}

void executeMovement(int angle, int speedPercent) {
  currentPos = angle;
  myservo.write(angle);

  int dacValue = 0;
  
  if (speedPercent > 0) {
    dacValue = map(speedPercent, 1, 100, motorMinDAC, motorMaxDAC);
  } else {
    dacValue = 0;
  }

  analogWrite(dacPin, dacValue);
  
  Serial.print("Angle: "); Serial.print(angle);
  Serial.print(" | Input Speed: "); Serial.print(speedPercent);
  Serial.print(" | Actual DAC: "); Serial.println(dacValue);
}