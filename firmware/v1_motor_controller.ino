// Aura v1 - Motor and Telemetry Controller (Arduino IDE sketch)
// Board: ESP32-S3 DevKit-C
//
// Wiring on my desk:
//   Left Motors:  GPIO 10, GPIO 11
//   Right Motors: GPIO 12, GPIO 13
//   Status LED:   GPIO 2
//   HC-SR04 Trig: GPIO 5
//   HC-SR04 Echo: GPIO 18 (with 10k resistor)

#include <Arduino.h>

// Motor pins
const int motor_left_fwd  = 10;
const int motor_left_rev  = 11;
const int motor_right_fwd = 12;
const int motor_right_rev = 13;
const int pin_led         = 2;

// Ultrasonic sensor pins
const int pin_trig = 5;
const int pin_echo = 18;
const float brake_distance_cm = 20.0;

// Watchdog timer variables
unsigned long last_cmd_time = 0;
const unsigned long watchdog_timeout = 1500;
char current_cmd = 'S';

// Distance variables
unsigned long last_ping_time = 0;
float distance_cm = 999.0;
bool brake_active = false;

// Telemetry timer
unsigned long last_telemetry_time = 0;

// Read distance from HC-SR04 sensor
float readDistance() {
    digitalWrite(pin_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(pin_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(pin_trig, LOW);

    long duration = pulseIn(pin_echo, HIGH, 25000);
    if (duration == 0) {
        return 999.0;
    }
    return (duration * 0.0343) / 2.0;
}

// Stop all four motors
void stopMotors() {
    digitalWrite(motor_left_fwd, LOW);
    digitalWrite(motor_left_rev, LOW);
    digitalWrite(motor_right_fwd, LOW);
    digitalWrite(motor_right_rev, LOW);
    digitalWrite(pin_led, brake_active ? HIGH : LOW);
    current_cmd = 'S';
}

// Drive forward
void driveForward() {
    if (brake_active) {
        stopMotors();
        Serial.print("Brake active! Obstacle at: ");
        Serial.print(distance_cm);
        Serial.println(" cm");
        return;
    }
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

    pinMode(pin_trig, OUTPUT);
    pinMode(pin_echo, INPUT);
    digitalWrite(pin_trig, LOW);

    stopMotors();
    delay(500);

    Serial.println("Aura v1 Motor Controller Online");
}

void loop() {
    unsigned long now = millis();

    // Measure distance every 60ms
    if (now - last_ping_time >= 60) {
        last_ping_time = now;
        float measured = readDistance();

        if (measured > 0.0 && measured < 400.0) {
            distance_cm = (distance_cm * 0.4) + (measured * 0.6);
        } else if (measured >= 999.0) {
            distance_cm = 999.0;
        }

        if (distance_cm < brake_distance_cm) {
            if (!brake_active) {
                brake_active = true;
                if (current_cmd == 'F') {
                    stopMotors();
                    Serial.print("Emergency Stop: Obstacle at ");
                    Serial.print(distance_cm);
                    Serial.println(" cm");
                }
            }
        } else {
            brake_active = false;
        }
    }

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
            Serial.print(current_cmd);
            Serial.print(" | Dist: ");
            Serial.print(distance_cm);
            Serial.print(" cm | Brake: ");
            Serial.println(brake_active ? "ON" : "OFF");
        }
    }

    // Send periodic telemetry every 500ms
    if (now - last_telemetry_time >= 500) {
        last_telemetry_time = now;
        Serial.print("Dist: ");
        Serial.print(distance_cm);
        Serial.print(" cm | Cmd: ");
        Serial.print(current_cmd);
        Serial.print(" | Brake: ");
        Serial.println(brake_active ? "ON" : "OFF");
    }

    // Stop motors if serial signal is lost for more than 1.5 seconds
    if (current_cmd != 'S' && (now - last_cmd_time > watchdog_timeout)) {
        stopMotors();
        Serial.println("Watchdog: Serial signal lost, stopping motors");
    }

    delay(5);
}
