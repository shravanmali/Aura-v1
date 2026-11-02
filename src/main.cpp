// Aura v1 - Robot Firmware
// Hardware: ESP32-S3 DevKit-C + L298N Motor Driver
// Controls 4 BO motors (skid-steer) over USB serial

#include <Arduino.h>

// L298N motor driver pins
const int PIN_MOTOR_LEFT_FWD  = 10; // IN1
const int PIN_MOTOR_LEFT_REV  = 11; // IN2
const int PIN_MOTOR_RIGHT_FWD = 12; // IN3
const int PIN_MOTOR_RIGHT_REV = 13; // IN4
const int PIN_STATUS_LED      = 2;  // Onboard blue LED

// Robot movement states
enum MotorState { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, BRAKED };
MotorState motorState = STOPPED;

unsigned long last_command_time = 0;

// Motor control functions
void stopMotors() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, LOW);
    digitalWrite(PIN_MOTOR_LEFT_REV, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_REV, LOW);
    digitalWrite(PIN_STATUS_LED, LOW);
}

void driveForward() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, HIGH);
    digitalWrite(PIN_MOTOR_LEFT_REV, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(PIN_MOTOR_RIGHT_REV, LOW);
    digitalWrite(PIN_STATUS_LED, HIGH);
}

void driveBackward() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, LOW);
    digitalWrite(PIN_MOTOR_LEFT_REV, HIGH);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_REV, HIGH);
    digitalWrite(PIN_STATUS_LED, HIGH);
}

void pivotLeft() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, LOW);
    digitalWrite(PIN_MOTOR_LEFT_REV, HIGH);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(PIN_MOTOR_RIGHT_REV, LOW);
    digitalWrite(PIN_STATUS_LED, HIGH);
}

void pivotRight() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, HIGH);
    digitalWrite(PIN_MOTOR_LEFT_REV, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_REV, HIGH);
    digitalWrite(PIN_STATUS_LED, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Set motor pins as outputs
    pinMode(PIN_MOTOR_LEFT_FWD, OUTPUT);
    pinMode(PIN_MOTOR_LEFT_REV, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_FWD, OUTPUT);
    pinMode(PIN_MOTOR_RIGHT_REV, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);
    stopMotors();

    last_command_time = millis();
    Serial.println("Aura v1 Controller Ready");
}

void loop() {
    // Read steering commands from Python vision dashboard
    while (Serial.available() > 0) {
        char cmd = Serial.read();
        last_command_time = millis();

        if (cmd == 'F' || cmd == 'f') {
            motorState = FORWARD;
            driveForward();
        } else if (cmd == 'B' || cmd == 'b') {
            motorState = BACKWARD;
            driveBackward();
        } else if (cmd == 'L' || cmd == 'l') {
            motorState = LEFT;
            pivotLeft();
        } else if (cmd == 'R' || cmd == 'r') {
            motorState = RIGHT;
            pivotRight();
        } else if (cmd == 'S' || cmd == 's') {
            motorState = STOPPED;
            stopMotors();
        } else if (cmd == 'O' || cmd == 'o') {
            motorState = BRAKED;
            stopMotors();
        }
    }

    // Safety timeout: stop if laptop disconnects or stops sending for 600ms
    if (motorState != STOPPED && motorState != BRAKED) {
        if (millis() - last_command_time > 600) {
            motorState = STOPPED;
            stopMotors();
        }
    }

    delay(10);
}
