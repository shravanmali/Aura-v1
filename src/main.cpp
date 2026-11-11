// Aura v1.6 rover firmware for ESP32-S3 DevKit-C
// Author: Shravan Mali

#include <Arduino.h>
#include <DHT.h>

// Motor driver pins (L298N)
const int PIN_MOTOR_IN1 = 10;
const int PIN_MOTOR_IN2 = 11;
const int PIN_MOTOR_IN3 = 12;
const int PIN_MOTOR_IN4 = 13;

// Pan-tilt gimbal servos
const int PIN_SERVO_PAN = 40;
const int PIN_SERVO_TILT = 39;
const int SERVO_PAN_CH = 0;
const int SERVO_TILT_CH = 1;

// Actuators
const int PIN_BUZZER = 38;
const int PIN_LIGHT_CTRL = 42;
const int PIN_PI_RESET = 37;

// Sensors
const int PIN_DHT = 4;
const int PIN_PIR = 14;
const int PIN_BATT_SENSE = 1;

#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE);

// State variables
float battery_voltage = 0.0;
float temperature = 0.0;
float humidity = 0.0;
bool pir_detected = false;

// Timers
unsigned long last_command_time = 0;
unsigned long last_telemetry_time = 0;
unsigned long last_pi_heartbeat = 0;
const unsigned long WATCHDOG_TIMEOUT_MS = 600;
const unsigned long PI_HARD_RESET_TIMEOUT_MS = 30000;

void write_servo_angle(int channel, int angle_deg) {
  angle_deg = constrain(angle_deg, 0, 180);
  uint32_t duty = 1638 + (angle_deg * (8192 - 1638) / 180);
  ledcWrite(channel, duty);
}

void beep(int duration_ms, int freq_hz = 2000) {
  tone(PIN_BUZZER, freq_hz, duration_ms);
  delay(duration_ms);
  noTone(PIN_BUZZER);
}

float read_battery_voltage() {
  uint32_t raw_adc = analogRead(PIN_BATT_SENSE);
  float pin_voltage = (raw_adc / 4095.0) * 3.30;
  return pin_voltage * 4.0303;
}

void drive_motors(int left_cmd, int right_cmd) {
  if (left_cmd > 0) {
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  } else if (left_cmd < 0) {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, HIGH);
  } else {
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
  }

  if (right_cmd > 0) {
    digitalWrite(PIN_MOTOR_IN3, HIGH);
    digitalWrite(PIN_MOTOR_IN4, LOW);
  } else if (right_cmd < 0) {
    digitalWrite(PIN_MOTOR_IN3, LOW);
    digitalWrite(PIN_MOTOR_IN4, HIGH);
  } else {
    digitalWrite(PIN_MOTOR_IN3, LOW);
    digitalWrite(PIN_MOTOR_IN4, LOW);
  }
}

void trigger_pi_hardware_reboot() {
  Serial.println("Pi watchdog timeout, resetting Pi...");
  beep(300, 1000);
  
  digitalWrite(PIN_PI_RESET, HIGH);
  delay(250);
  digitalWrite(PIN_PI_RESET, LOW);

  last_pi_heartbeat = millis();
  beep(100, 2200);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  drive_motors(0, 0);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LIGHT_CTRL, OUTPUT);
  pinMode(PIN_PI_RESET, OUTPUT);
  digitalWrite(PIN_LIGHT_CTRL, LOW);
  digitalWrite(PIN_PI_RESET, LOW);

  pinMode(PIN_PIR, INPUT);
  analogReadResolution(12);
  dht.begin();

  ledcSetup(SERVO_PAN_CH, 50, 16);
  ledcAttachPin(PIN_SERVO_PAN, SERVO_PAN_CH);
  ledcSetup(SERVO_TILT_CH, 50, 16);
  ledcAttachPin(PIN_SERVO_TILT, SERVO_TILT_CH);

  write_servo_angle(SERVO_PAN_CH, 90);
  write_servo_angle(SERVO_TILT_CH, 90);

  beep(80, 1800);
  delay(50);
  beep(150, 2400);

  last_command_time = millis();
  last_pi_heartbeat = millis();
  Serial.println("Aura v1.6 ready");
}

void loop() {
  while (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    last_command_time = millis();

    if (cmd == "HEARTBEAT") {
      last_pi_heartbeat = millis();
    } else if (cmd == "F" || cmd == "f") {
      drive_motors(1, 1);
    } else if (cmd == "B" || cmd == "b") {
      drive_motors(-1, -1);
    } else if (cmd == "L" || cmd == "l") {
      drive_motors(-1, 1);
    } else if (cmd == "R" || cmd == "r") {
      drive_motors(1, -1);
    } else if (cmd == "S" || cmd == "s" || cmd == "O" || cmd == "o") {
      drive_motors(0, 0);
    } else if (cmd.startsWith("DRIVE:")) {
      int comma_idx = cmd.indexOf(',');
      if (comma_idx > 6) {
        int l = cmd.substring(6, comma_idx).toInt();
        int r = cmd.substring(comma_idx + 1).toInt();
        drive_motors(l, r);
      }
    } else if (cmd.startsWith("GIMBAL:")) {
      int comma_idx = cmd.indexOf(',');
      if (comma_idx > 7) {
        int p = cmd.substring(7, comma_idx).toInt();
        int t = cmd.substring(comma_idx + 1).toInt();
        write_servo_angle(SERVO_PAN_CH, constrain(p, 15, 165));
        write_servo_angle(SERVO_TILT_CH, constrain(t, 40, 140));
      }
    } else if (cmd == "LIGHT_ON") {
      digitalWrite(PIN_LIGHT_CTRL, HIGH);
    } else if (cmd == "LIGHT_OFF") {
      digitalWrite(PIN_LIGHT_CTRL, LOW);
    } else if (cmd == "BEEP") {
      beep(100, 2200);
    }
  }

  // Safety stop if no command received
  if (millis() - last_command_time > WATCHDOG_TIMEOUT_MS) {
    drive_motors(0, 0);
  }

  // Telemetry stream every second
  if (millis() - last_telemetry_time >= 1000) {
    last_telemetry_time = millis();
    battery_voltage = read_battery_voltage();
    pir_detected = (digitalRead(PIN_PIR) == HIGH);

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;

    Serial.printf("[TELEM] BATT:%.2f|T:%.1f|H:%.1f|PIR:%d\n", 
                  battery_voltage, temperature, humidity, pir_detected ? 1 : 0);

    if (battery_voltage < 10.0 && battery_voltage > 7.0) {
      beep(80, 800);
    }
  }

  if (millis() - last_pi_heartbeat > PI_HARD_RESET_TIMEOUT_MS) {
    trigger_pi_hardware_reboot();
  }

  delay(5);
}
