// Aura v1 - Robot Firmware
// Hardware: ESP32-S3 DevKit-C + L298N Motor Driver + DHT11 + PIR Sensor
// Controls 4 BO motors (skid-steer) and streams sensor data over USB serial

#include <Arduino.h>
#include <DHT.h>

// L298N motor driver pins
const int PIN_MOTOR_LEFT_FWD  = 10; // IN1
const int PIN_MOTOR_LEFT_REV  = 11; // IN2
const int PIN_MOTOR_RIGHT_FWD = 12; // IN3
const int PIN_MOTOR_RIGHT_REV = 13; // IN4
const int PIN_STATUS_LED      = 2;  // Onboard blue LED

// Sensor pins
#define PIN_DHT 4
#define PIN_PIR 14
#define DHTTYPE DHT11

DHT dht(PIN_DHT, DHTTYPE);

// Robot movement states
enum MotorState { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, BRAKED };
MotorState motorState = STOPPED;

// Sensor variables
float temp = 0.0;
float hum = 0.0;
bool pir_active = false;

// Timers for non-blocking loop
unsigned long last_dht_read = 0;
unsigned long last_telemetry_send = 0;
unsigned long last_command_time = 0;

// Motor control functions
void stopMotors() {
    digitalWrite(PIN_MOTOR_LEFT_FWD, LOW);
    digitalWrite(PIN_MOTOR_LEFT_REV, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_FWD, LOW);
    digitalWrite(PIN_MOTOR_RIGHT_REV, LOW);
    digitalWrite(PIN_STATUS_LED, pir_active ? HIGH : LOW);
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

    // Start PIR and DHT sensors
    pinMode(PIN_PIR, INPUT);
    dht.begin();

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

    // Check PIR motion sensor
    if (digitalRead(PIN_PIR) == HIGH) {
        pir_active = true;
        digitalWrite(PIN_STATUS_LED, HIGH);
    } else {
        pir_active = false;
        if (motorState == STOPPED || motorState == BRAKED) {
            digitalWrite(PIN_STATUS_LED, LOW);
        }
    }

    // Read temperature and humidity every 2 seconds
    if (millis() - last_dht_read >= 2000) {
        last_dht_read = millis();
        float t = dht.readTemperature();
        float h = dht.readHumidity();

        if (!isnan(t) && !isnan(h)) {
            temp = t;
            hum = h;
        }
    }

    // Send telemetry to Python dashboard every 100ms
    // Note: DIST is set to -1.0 because ultrasonic sensor is damaged
    if (millis() - last_telemetry_send >= 100) {
        last_telemetry_send = millis();

        const char* motor_name = "STOPPED";
        if (motorState == FORWARD)  motor_name = "FORWARD";
        if (motorState == BACKWARD) motor_name = "BACKWARD";
        if (motorState == LEFT)     motor_name = "LEFT";
        if (motorState == RIGHT)    motor_name = "RIGHT";
        if (motorState == BRAKED)   motor_name = "BRAKED";

        Serial.print("[TELEM] T:");
        Serial.print(temp, 1);
        Serial.print("|H:");
        Serial.print(hum, 1);
        Serial.print("|PIR:");
        Serial.print(pir_active ? 1 : 0);
        Serial.print("|DIST:-1.0|MOT:");
        Serial.println(motor_name);
    }

    delay(10);
}
