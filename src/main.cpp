// =============================================================================
// Aura v1.6 - Autonomous Mobile Companion Robot Production Firmware
// Hardware: ESP32-S3 DevKit-C (N16R8) on Aura v1.6 Carrier PCB
// Author: Shravan Mali (Hack Club Fallen - Aura v1)
// =============================================================================

#include <Arduino.h>
#include <DHT.h>

// -----------------------------------------------------------------------------
// Pin Definitions (Directly mapped to Aura v1.6 Carrier PCB)
// -----------------------------------------------------------------------------
// L298N 4WD Motor Driver (J3 / J_MOTOR)
const int PIN_MOTOR_IN1    = 10; // J1 Pin 18 (Left Forward)
const int PIN_MOTOR_IN2    = 11; // J1 Pin 19 (Left Reverse)
const int PIN_MOTOR_IN3    = 12; // J1 Pin 20 (Right Forward)
const int PIN_MOTOR_IN4    = 13; // J1 Pin 21 (Right Reverse)

// Pan-Tilt Gimbal Servos (LEDC PWM on ESP32-S3)
const int PIN_SERVO_PAN    = 40; // J2 Pin 8
const int PIN_SERVO_TILT   = 39; // J2 Pin 9
const int SERVO_PAN_CH     = 0;
const int SERVO_TILT_CH    = 1;

// Actuator & Diagnostic Outputs
const int PIN_BUZZER       = 38; // J2 Pin 10 (via Q5 2N2222)
const int PIN_LIGHT_CTRL   = 42; // J2 Pin 6  (via Q1 2N2222)
const int PIN_PI_RESET     = 37; // J2 Pin 11 (via Q4 2N2222 to Pi GLOBAL_EN)

// Sensors
const int PIN_DHT          = 4;  // J1 Pin 4  (DHT11)
const int PIN_PIR          = 14; // J1 Pin 22 (HC-SR501)
const int PIN_BATT_SENSE   = 1;  // J2 Pin 4  (ADC1_CH0 via 100k/33k divider)

#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE);

// State Variables
float battery_voltage = 0.0;
float temperature = 0.0;
float humidity = 0.0;
bool pir_detected = false;

// Timers
unsigned long last_command_time = 0;
unsigned long last_telemetry_time = 0;
unsigned long last_pi_heartbeat = 0;
const unsigned long WATCHDOG_TIMEOUT_MS = 600;       // Auto-stop motors if Pi disconnects
const unsigned long PI_HARD_RESET_TIMEOUT_MS = 30000; // 30 sec freeze triggers reboot

// -----------------------------------------------------------------------------
// Servo Control using ESP32-S3 Hardware PWM (50Hz, 16-bit resolution)
// -----------------------------------------------------------------------------
void write_servo_angle(int channel, int angle_deg) {
  angle_deg = constrain(angle_deg, 0, 180);
  // 50Hz period = 20ms. 0 deg = 0.5ms (1638 counts), 180 deg = 2.5ms (8192 counts)
  uint32_t duty = 1638 + (angle_deg * (8192 - 1638) / 180);
  ledcWrite(channel, duty);
}

// -----------------------------------------------------------------------------
// Audible Buzzer Chirp Helper
// -----------------------------------------------------------------------------
void beep(int duration_ms, int freq_hz = 2000) {
  tone(PIN_BUZZER, freq_hz, duration_ms);
  delay(duration_ms);
  noTone(PIN_BUZZER);
}

// -----------------------------------------------------------------------------
// Read Battery Voltage via 100kΩ / 33kΩ Resistor Divider (R10 / R11)
// Ratio = (100k + 33k) / 33k = 4.0303
// -----------------------------------------------------------------------------
float read_battery_voltage() {
  uint32_t raw_adc = analogRead(PIN_BATT_SENSE);
  float pin_voltage = (raw_adc / 4095.0) * 3.30;
  return pin_voltage * 4.0303;
}

// -----------------------------------------------------------------------------
// 4WD Motor Skid-Steer Controller
// -----------------------------------------------------------------------------
void drive_motors(int left_cmd, int right_cmd) {
  // Left Motors (IN1 / IN2)
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

  // Right Motors (IN3 / IN4)
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

// -----------------------------------------------------------------------------
// Hardware Raspberry Pi Emergency Reboot
// -----------------------------------------------------------------------------
void trigger_pi_hardware_reboot() {
  Serial.println("[SAFETY] Raspberry Pi frozen! Pulling GLOBAL_EN low...");
  beep(300, 1000);
  
  // Pull Pi GLOBAL_EN to GND for 250ms via Q4
  digitalWrite(PIN_PI_RESET, HIGH);
  delay(250);
  digitalWrite(PIN_PI_RESET, LOW);

  last_pi_heartbeat = millis(); // Reset timer to allow Pi 30 seconds to boot
  beep(100, 2200);
}

// -----------------------------------------------------------------------------
// Arduino Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  // Configure Motor Outputs
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  drive_motors(0, 0);

  // Configure Actuator Pins
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LIGHT_CTRL, OUTPUT);
  pinMode(PIN_PI_RESET, OUTPUT);
  digitalWrite(PIN_LIGHT_CTRL, LOW);
  digitalWrite(PIN_PI_RESET, LOW);

  // Sensors & ADC
  pinMode(PIN_PIR, INPUT);
  analogReadResolution(12);
  dht.begin();

  // Configure Hardware PWM for Pan-Tilt Servos (50Hz, 16-bit)
  ledcSetup(SERVO_PAN_CH, 50, 16);
  ledcAttachPin(PIN_SERVO_PAN, SERVO_PAN_CH);
  ledcSetup(SERVO_TILT_CH, 50, 16);
  ledcAttachPin(PIN_SERVO_TILT, SERVO_TILT_CH);

  write_servo_angle(SERVO_PAN_CH, 90);  // Center forward
  write_servo_angle(SERVO_TILT_CH, 90); // Center level

  // Aura Power-On Chime
  beep(80, 1800);
  delay(50);
  beep(150, 2400);

  last_command_time = millis();
  last_pi_heartbeat = millis();
  Serial.println("AURA_V1.6_ONLINE");
}

// -----------------------------------------------------------------------------
// Main Execution Loop
// -----------------------------------------------------------------------------
void loop() {
  // 1. Process Serial Commands from Raspberry Pi
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

  // 2. Safety Timeout: Auto-stop motors if no drive packet received in 600ms
  if (millis() - last_command_time > WATCHDOG_TIMEOUT_MS) {
    drive_motors(0, 0);
  }

  // 3. Periodic Telemetry Stream (Every 1000ms)
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

    // Low battery warning (< 10.0V on 3S pack)
    if (battery_voltage < 10.0 && battery_voltage > 7.0) {
      beep(80, 800);
    }
  }

  // 4. Raspberry Pi Hardware Watchdog
  if (millis() - last_pi_heartbeat > PI_HARD_RESET_TIMEOUT_MS) {
    trigger_pi_hardware_reboot();
  }

  delay(5);
}
