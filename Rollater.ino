#include <TaskScheduler.h>
#include <urc10.h>

// Hardware
const int yPin = A1;
#define xPin A0
const int forwardButtonPin = 2;
const int boostButtonPin = 3;
const int boostLED = 8;
int direction = MOTOR1_LEFT_MOTOR2_RIGHT;

// Task method prototypes
void fwdButton();
void boostButton();
void sensorRead();
void rampUp();
void boundSpeed();
void coastStop();
void turningLeft();
void turningRight();
void center();
//void reverseCallback();

// Tasks
Task forwardButtonTask(0, TASK_ONCE, &fwdButton);
Task boostButtonTask(0, TASK_ONCE, &boostButton);
Task sensorReadTask(500, TASK_FOREVER, &sensorRead);
Task rampUpTask(100, TASK_FOREVER, &rampUp);
Task boundSpeedTask(100, TASK_FOREVER, &boundSpeed);
Task coastStopTask(100, TASK_FOREVER, &coastStop);
Task turnLeftTask(100, TASK_FOREVER, &turningLeft);
Task turnRightTask(100, TASK_FOREVER, &turningRight);
Task centerTask(100, TASK_FOREVER, &center);
//Task reverseTask(100, TASK_FOREVER, &reverseCallback);
Scheduler schedule;

const int rampUpDelta = 2;
const int fastSlowDownDelta = 2;
const int slowDownDelta = 1;
const int coastStopDelta = 1;
const int minSpeed = 0;
const int slowSpeed = 30;
const int cruiseSpeed = 25;//50;
const int boostSpeed = 100;
const int slowDownFasterRange = 20;
const int turnSpeedDelta = 5;
const int turnSpeedDiff = 15;//30;
int maxmSpeed = cruiseSpeed;
int leftSpeed = 0;
int rightSpeed = 0;

//joystick vals
const int middle = 513;
const int turnThresh = 100;
int lastDir = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Start up");
  URC10.begin();
  // set motor direction
  URC10.motor.setDirection(direction);

  schedule.init();
  schedule.addTask(forwardButtonTask);
  schedule.addTask(boostButtonTask);
  schedule.addTask(sensorReadTask);
  schedule.addTask(rampUpTask);
  schedule.addTask(boundSpeedTask);
  schedule.addTask(coastStopTask);
  schedule.addTask(turnLeftTask);
  schedule.addTask(turnRightTask);
  schedule.addTask(centerTask);
  sensorReadTask.enable();

  pinMode(xPin, INPUT);
  pinMode(boostLED, OUTPUT);
  digitalWrite(boostLED, HIGH);

  pinMode(forwardButtonPin, INPUT_PULLUP);
  pinMode(boostButtonPin, INPUT_PULLUP);
  // forwardButtonInterrupt() is called when the forwardButtonPin changes from HIGH to LOW or vise versa
  attachInterrupt(digitalPinToInterrupt(forwardButtonPin), forwardButtonInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(boostButtonPin), boostButtonInterrupt, RISING);
}

void loop() {
  schedule.execute();
  //  Serial.println(digitalRead(forwardButtonPin));
}

void forwardButtonInterrupt() {
  //  Serial.println(digitalRead(forwardButtonPin));
  // When the button is LOW it is pressed, when HIGH released
  forwardButtonTask.setInterval(TASK_ONCE);
  forwardButtonTask.restart();
  forwardButtonTask.enable();
}

void fwdButton() {
  //  Serial.println(digitalRead(forwardButtonPin));
  // When the button is LOW it is pressed, when HIGH released
  if (digitalRead(forwardButtonPin) == LOW) {
    coastStopTask.disable();
    rampUpTask.enable();
  } else {
    //     TODO: disable all other tasks!!!
    rampUpTask.disable();
    boundSpeedTask.disable();
    coastStopTask.enable();
  }
}

void boostButtonInterrupt() {
  // When the button is LOW it is pressed, when HIGH released
  boostButtonTask.setInterval(TASK_ONCE);
  boostButtonTask.restart();
  boostButtonTask.enable();
}

void boostButton() {
  if (maxmSpeed == boostSpeed) {
    maxmSpeed = cruiseSpeed;
    digitalWrite(boostLED, LOW);
    //    Serial.println("offf");
  } else {
    maxmSpeed = boostSpeed;
    digitalWrite(boostLED, HIGH);
    //    Serial.println("on");
  }
  forwardButtonTask.setInterval(TASK_ONCE);
  forwardButtonTask.restart();
  forwardButtonTask.enable();
}

void sensorRead() {
  int x = analogRead(xPin);
  Serial.println(x);
  if (x > middle + turnThresh) {
    turnLeftTask.disable();
    centerTask.disable();
    turnRightTask.enable();
  } else if (x < middle - turnThresh) {
    turnRightTask.disable();
    centerTask.disable();
    turnLeftTask.enable();
  } else {
    // center
    turnLeftTask.disable();
    turnRightTask.disable();
    centerTask.enable();
  }
}
void rampUp() {
  if (leftSpeed >= maxmSpeed || rightSpeed >= maxmSpeed) {
    rampUpTask.disable();
    boundSpeedTask.enable();
    return;
  }
  leftSpeed += rampUpDelta;
  rightSpeed += rampUpDelta;
  URC10.motor.forward(leftSpeed, rightSpeed);
  //  Serial.print("Ramping up: ");
  //  Serial.print(leftSpeed);
  //  Serial.print(", ");
  //  Serial.println(rightSpeed);
}

void boundSpeed() {
  if (leftSpeed <= maxmSpeed || rightSpeed <= maxmSpeed) {
    boundSpeedTask.disable();
    return;
  }
  if (maxmSpeed - leftSpeed > slowDownFasterRange || maxmSpeed - leftSpeed > slowDownFasterRange) {
    leftSpeed -= fastSlowDownDelta;
    rightSpeed -= fastSlowDownDelta;
  } else {
    leftSpeed -= slowDownDelta;
    rightSpeed -= slowDownDelta;
  }
  URC10.motor.forward(leftSpeed, rightSpeed);
}

void coastStop() {
  if (leftSpeed <= minSpeed && rightSpeed <= minSpeed) {
    coastStopTask.disable();
    // Want to make sure we aren't zooming on next start up
    maxmSpeed = cruiseSpeed;
    digitalWrite(boostLED, LOW);
    return;
  }
  if (leftSpeed > minSpeed) {
    leftSpeed -= coastStopDelta;
  }
  if (rightSpeed > minSpeed) {
    rightSpeed -= coastStopDelta;
  }
  URC10.motor.forward(leftSpeed, rightSpeed);
  //  Serial.print("Slowing down: ");
  //  Serial.print(leftSpeed);
  //  Serial.print(", ");
  //  Serial.println(rightSpeed);
}

// goal is rightSpeed = leftSpeed + turnSpeedDiff
void turningLeft() {
  int diff = rightSpeed - leftSpeed;
  if (diff < turnSpeedDiff) {
    // need to boost right/slow left
    if (rightSpeed + turnSpeedDelta <= maxmSpeed) {
      rightSpeed += turnSpeedDelta;
    }else if(leftSpeed >= turnSpeedDelta){
      leftSpeed -= turnSpeedDelta;
    }//else bad numbers
  } else if (diff > turnSpeedDiff) {
    // need to boost left/slow right
    // bad? should center first?
  } else {
    // already turning left :D
    turnLeftTask.disable();
    return;
  }
  URC10.motor.forward(leftSpeed, rightSpeed);
}

// goal is leftSpeed = rightSpeed + turnSpeedDiff
void turningRight() {
  int diff = leftSpeed - rightSpeed;
  if (diff < turnSpeedDiff) {
    // need to boost left/slow right
    if (leftSpeed + turnSpeedDelta <= maxmSpeed) {
      leftSpeed += turnSpeedDelta;
    } else if(rightSpeed >= turnSpeedDelta){
      rightSpeed -= turnSpeedDelta;
    }
  } else if (diff > turnSpeedDiff) {
    // need to boost right/slow left
    // bad? should center first?
  } else {
    // already turning left :D
    turnLeftTask.disable();
    return;
  }
  URC10.motor.forward(leftSpeed, rightSpeed);
}

void center() {
  int diff = leftSpeed - rightSpeed;
  if (diff > 0 && leftSpeed > turnSpeedDelta) {
    leftSpeed -= turnSpeedDelta;
  } else if (diff < 0 && rightSpeed > turnSpeedDelta) {
    rightSpeed -= turnSpeedDelta;
  } else {
    centerTask.disable();

    forwardButtonTask.setInterval(TASK_ONCE);
    forwardButtonTask.restart();
    forwardButtonTask.enable();
  }
  URC10.motor.forward(leftSpeed, rightSpeed);
}
