// Aura v1 - Motor Controller (Arduino IDE sketch)
// Board: ESP32-S3 DevKit-C
//
// Wiring on my desk:
//   Left Motors:  GPIO 10, GPIO 11
//   Right Motors: GPIO 12, GPIO 13
//   Status LED:   GPIO 2

#include <Arduino.h>

// Motor pins
const int motor_left_fwd  = 10;
const int motor_left_rev  = 11;
const int motor_right_fwd = 12;
const int motor_right_rev = 13;
const int pin_led         = 2;

// Watchdog timer variables
unsigned long last_cmd_time = 0;
const unsigned long watchdog_timeout = 1500;
char current_cmd = 'S';

// Stop all four motors
void stopMotors() {
    digitalWrite(motor_left_fwd, LOW);
    digitalWrite(motor_left_rev, LOW);
    digitalWrite(motor_right_fwd, LOW);
    digitalWrite(motor_right_rev, LOW);
    digitalWrite(pin_led, LOW);
    current_cmd = 'S';
}

// Drive forward
void driveForward() {
    digitalWrite(motor_left_fwd, HIGH);
    digitalWrite(motor_left_rev, LOW);
    digitalWrite(motor_right_fwd, HIGH);
    digitalWrite(motor_right_rev, LOW);
    digitalWrite(pin_led, HIGH);
    current_cmd = 'F';
}

// Drive backward
void driveBackward() {
    digitalWrite(motor_left_fwd, LOW);
    digitalWrite(motor_left_rev, HIGH);
    digitalWrite(motor_right_fwd, LOW);
    digitalWrite(motor_right_rev, HIGH);
    digitalWrite(pin_led, HIGH);
    current_cmd = 'B';
}

// Pivot turn left
void pivotLeft() {
    digitalWrite(motor_left_fwd, LOW);
    digitalWrite(motor_left_rev, HIGH);
    digitalWrite(motor_right_fwd, HIGH);
    digitalWrite(motor_right_rev, LOW);
    digitalWrite(pin_led, HIGH);
    current_cmd = 'L';
}

// Pivot turn right
void pivotRight() {
    digitalWrite(motor_left_fwd, HIGH);
    digitalWrite(motor_left_rev, LOW);
    digitalWrite(motor_right_fwd, LOW);
    digitalWrite(motor_right_rev, HIGH);
    digitalWrite(pin_led, HIGH);
    current_cmd = 'R';
}

void setup() {
    Serial.begin(115200);

    pinMode(motor_left_fwd, OUTPUT);
    pinMode(motor_left_rev, OUTPUT);
    pinMode(motor_right_fwd, OUTPUT);
    pinMode(motor_right_rev, OUTPUT);
    pinMode(pin_led, OUTPUT);

    stopMotors();
    delay(500);

    Serial.println("Aura v1 Motor Controller Online");
}

void loop() {
    unsigned long now = millis();

    // Read commands from serial
    while (Serial.available() > 0) {
        char cmd = (char)Serial.read();

        if (cmd == 'F' || cmd == 'B' || cmd == 'L' || cmd == 'R' || cmd == 'S') {
            last_cmd_time = now;

            if (cmd == 'F') driveForward();
            else if (cmd == 'B') driveBackward();
            else if (cmd == 'L') pivotLeft();
            else if (cmd == 'R') pivotRight();
            else if (cmd == 'S') stopMotors();

            Serial.print("Cmd: ");
            Serial.println(current_cmd);
        }
    }

    // Stop motors if serial signal is lost for more than 1.5 seconds
    if (current_cmd != 'S' && (now - last_cmd_time > watchdog_timeout)) {
        stopMotors();
        Serial.println("Watchdog: Serial signal lost, stopping motors");
    }

    delay(5);
}
