//
// ==========================================================================
// ============================= DISCLAIMER =================================
// ==========================================================================
//
// THIS SCRIPT, INSTRUCTIONS, INFORMATION AND OTHER SERVICES ARE PROVIDED BY THE DEVELOPER ON AN "AS IS" AND "AS AVAILLABLE" BASIS, UNLESS
// OTHERWISE SPECIFIED IN WRITING. THE DEVELOPER DOES NOT MAKE ANY REPRESENTATIONS OR WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, AS TO THIS
// SCRIPT, INSTRUCTIONS, INFORMATION AND OTHER SERVICES. YOU EXPRESSLY AGREE THAT YOUR USE OF THIS SCRIPT IS AT YOUR OWN RISK.
//
// TO THE FULL EXTEND PERMISSABLE BY LAW, THE DEVELOPER DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED. TO THE FULL EXTEND PERMISSABLE BY LAW,
// THE DEVELOPER WILL NOT BE LIABLE FOR ANY DAMAGES OF ANY KIND ARISING FROM THE USE OF THIS SCRIPT, INSTRUCTIONS, INFORMATION AND OTHER SERVICES,
// INCLUDING, BUT NOT LIMITED TO DIRECT, INDIRECT, INCIDENTAL, PUNITIVE AND CONSEQUENTIAL DAMAGES, UNLESS OTHERWISE SPECIFIED IN WRITING.
//
// ==========================================================================
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <PID_v1.h>

// Debug modes
#define SNIFFER     -1
#define NONE        0
#define EVENT       1
#define ALL         2

// ==========================================================================
// ========================= SERIAL PROTOCOL ================================
// ==========================================================================
// Header: BYTE 1 & 2
#define XIAOMI_H1           0x55
#define XIAOMI_H2           0xAA
#define NINEBOT_H1          0x5A
#define NINEBOT_H2          0xA5
// Destination: BYTE 3 (XIAOMI) / Source: BYTE 3 & Destination: BYTE 4 (NINEBOT)
#define BROADCAST_TO_ALL    0x00
#define ESC                 0x20 // Xiaomi: From BLE / Ninebot: From any
#define BLE                 0x21 // Xiaomi: From ESC / Ninebot: From any
#define BMS                 0x22 // Xiaomi: From BLE / Ninebot: From any
#define BLE_FROM_BMS        0x23 // Xiaomi only
#define BMS_FROM_ESC        0x24 // Xiaomi only
#define ESC_FROM_BMS        0x25 // Xiaomi only
#define WIRED_SERIAL        0x3D // Ninebot only
#define BLUETOOTH_SERIAL    0x3E // Ninebot only
#define UNKNOWN_SERIAL      0x3F // Ninebot only
// Command: BYTE 3 (XIAOMI) / BYTE 4 (NINEBOT)
#define BRAKE               0x65// ONLY AT DESTINATION 0x20 BLE>ESC
#define SPEED               0x64 // ONLY AT DESTINATION 0x21 ESC>BLE

// ==========================================================================
// =============================== CONFIG ===================================
// ==========================================================================
//
const int   DEBUG_MODE          = NONE;  // Debug mode (NONE for no logging, EVENT for event logging, ALL for serial data logging)
const int   RAW_TROTTLE_LO      = 45;
const int   RAW_TROTTLE_HI      = 233;
const float RAW_TROTTLE_SCALAR  = (RAW_TROTTLE_HI - RAW_TROTTLE_LO) / 100;
// Kick detection
const float KICK_DETECT_LOW      = 1.3;   // What speed increase (speed above current speed) to detect as kick at or below 18 kmh
const float KICK_DETECT_HIGH     = 1; // What speed increase (speed above current speed) to detect as kick above 18 kmh
const float KICK_DETECT_DIFF      = KICK_DETECT_HIGH - KICK_DETECT_LOW; // Difference between high and low kick detection
const int   KICK_DURATION        = 5000;  // Duration of boost {1st,2nd,3rd,4th,5th kick during boost and more} (boost is reset by braking or >5s no boost)
// Speed controller (PID)
const int   SPEED_MIN            = 8;   // Speed to start throttling (lower than 8 increases chance of false boost during walking)
const int   SPEED_MAX            = 25;  // Speed to stop throttling (lower than 8 increases chance of false boost during walking)
const int   SPEED_DIF            = SPEED_MAX-SPEED_MIN;   // Speed to start throttling (lower than 8 increases chance of false boost during walking)
const float SPEED_PROPORTION_MIN = 20;   // Power of increase at minimum speed
const float SPEED_PROPORTION_MAX = 50;   // Power of increase at maximum speed
const float SPEED_PROPORTION_DIF = SPEED_PROPORTION_MAX-SPEED_PROPORTION_MIN;   // Difference between min and max power of increase
const float SPEED_INTEGRAL       = 5;   // Time to overcome steady state errors (difference between speed and target target)
const float SPEED_DISTURBANCE    = 1.2; // Time to remove overshoot / oscillations
const float SPEED_INCREASE_LOW   = 1;   // Speed increase beyond target at or below 18 kmh
const float SPEED_INCREASE_HIGH  = 1;  // Speed increase beyond target above 18 kmh
const int   SPEED_READINGS       = 6;   // Amount of speed readings (increases steadiness but decreases reaction speed)

// Throttle and brake
const int   THROTTLE_OFF_PCT   = 0;   // Percentage of throttle when rolling out, 0 with KERS disabled, 10 with KERS enabled
const int   BRAKE_LIMIT        = 48;    // Limit for disabling throttle when pressing brake pedal (we recommend setting this as low as possible)
const int   THROTTLE_PIN       = 10;    // Pin of programming board (9=D9 or 10=D10)
const int   SERIAL_PIN         = 2;     // Pin of serial input (2=D2)

/* CODE BELOW THIS POINT */
SoftwareSerial SoftSerial(SERIAL_PIN, 3); /* RX, TX */

/* Variables */
bool isBraking = true;                    // brake activated
int boostCount = 0;                       // count of boosts after reaching THROTTLE_MIN_KMH
int speedRaw = 0;                         // current raw speed
int speedReadings[SPEED_READINGS] = {0};  // the last 4 readings from the speedometer
int speedIndex = 0;                       // the index of the current reading

const unsigned long boostResetTime = -99999;
unsigned long boostTime = boostResetTime;       // start time of last boost
double speedCurrent = 0;                        // current raw speed
double speedTarget = 0;                         // current target speed
double throttleOut = RAW_TROTTLE_LO;            // the current throttle position

// Protocol variables
uint8_t h1 = 0x00;
uint8_t h2 = 0x00;
int lenOffset = 1;

PID throttlePID(&speedCurrent, &throttleOut, &speedTarget, SPEED_PROPORTION_MIN, SPEED_INTEGRAL, SPEED_DISTURBANCE, DIRECT);

uint8_t readByteBlocking()
{
    while (!SoftSerial.available()) {
        delay(1);
    }
    return SoftSerial.read();
}

void setup()
{
    Serial.begin(115200); SoftSerial.begin(115200); /* Start reading SERIAL_PIN at 115200 baud */
    TCCR1B = TCCR1B & 0b11111001; /* TCCR1B = TIMER 1 (D9 and D10 on Nano) to 32 khz */
    throttlePID.SetOutputLimits(RAW_TROTTLE_LO, RAW_TROTTLE_HI);
    throttlePID.SetMode(AUTOMATIC);
    throttlePID.SetSampleTime(50);
}

uint8_t buff[256];

void loop()
{
    uint8_t len = 0x00;
    uint16_t sum = 0x0000;
    uint16_t checksum = 0x0000;
    uint8_t curr = 0x00;

    if (DEBUG_MODE == SNIFFER) {
        curr = readByteBlocking();
        if (curr==XIAOMI_H1 || curr==NINEBOT_H1) {
            Serial.println("");
        }
        Serial.print(curr,HEX);
        Serial.print(" ");
    } else {
        if (h1 == 0x00){
            switch(readByteBlocking()) {
                case XIAOMI_H1:
                    if(readByteBlocking()==XIAOMI_H2){
                        h1 = XIAOMI_H1;
                        h2 = XIAOMI_H2;
                        if (DEBUG_MODE >= EVENT) {
                            Serial.println((String)"DETECTED XIAOMI");
                        }
                    }
                    break;
                case NINEBOT_H1:
                    if(readByteBlocking()==NINEBOT_H2){
                        h1 = NINEBOT_H1;
                        h2 = NINEBOT_H2;
                        lenOffset = 4;
                        if (DEBUG_MODE >= EVENT) {
                            Serial.println((String)"DETECTED NINEBOT");
                        }
                    }
                    break;
                // we can support more protocols on the future
            }
        } else {
            while (readByteBlocking() != h1) {
                // WAIT FOR BYTE 1
            };
            if (readByteBlocking() != h2) {
                return; /* STOP WHEN INVALID BYTE 2 */
            }
            len = readByteBlocking(); // BYTE 3 = LENGTH
            if (len < 3 || len > 8) {
                return; /* STOP ON INVALID OR TOO LONG LENGTHS */
            }
            buff[0] = len;
            curr = 0x00;
            sum = len;
            for (int i = 0; i < len + lenOffset; i++) { // BYTE 5+
                curr = readByteBlocking();
                buff[i + 1] = curr; // CHECKSUM: BYTE 3 + 4 + 5+
                sum += curr;
            }
            if (DEBUG_MODE==ALL) {
                logBuffer(buff, len);
            }
            /* LAST 2 BYTES IS CHECKSUM */
            checksum = (uint16_t) readByteBlocking() | ((uint16_t) readByteBlocking() << 8);
            if (checksum != (sum ^ 0xFFFF)) { // CHECK XOR OF SUM AGAINST CHECKSUM
                if (DEBUG_MODE == ALL) {
                    Serial.println((String)"CHECKSUM: " + checksum + " == " + (sum ^ 0xFFFF) + " >> FAILED!");
                }
                return; /* STOP ON INVALID CHECKSUM */
            }
            /* When braking */
            if (buff[1] == ESC && buff[2] == BRAKE) {
                isBraking = (buff[len + lenOffset - 2] >= BRAKE_LIMIT);
                if (DEBUG_MODE>=EVENT && isBraking) {
                    Serial.println((String)"BRAKE: " + buff[len + lenOffset - 2]+" (" + (isBraking ? "yes" : "no") + ")");
                }
                if (isBraking) {
                    throttlePID.SetMode(MANUAL);
                    boostTime = boostResetTime;
                    boostCount = 0;
                    throttleOut = RAW_TROTTLE_LO;
                    speedTarget = 0;
                }
            } else if (buff[1] == BLE && buff[2] == SPEED) { /* Each speed reading */
                speedRaw = buff[len + lenOffset - 1];
                speedReadings[speedIndex] = speedRaw;
                speedCurrent = calcSpeedAvg();
                speedIndex = (speedIndex >= SPEED_READINGS ? 0 : speedIndex + 1);
                /* Detect boost */
                float diff = maxDifference(speedCurrent);
                if (speedRaw > 7 &&
                    diff > (KICK_DETECT_LOW + KICK_DETECT_DIFF / SPEED_DIF * (speedRaw - SPEED_MIN)) &&
                    (diff + speedCurrent) > speedTarget) { /* Detect kick */
                    /* Reset PID */
                    throttlePID.SetMode(AUTOMATIC);
                    Serial.println((String)"BOOST");
                    boostTime = millis();
                    boostCount += 1;
                }
                if (millis() - boostTime < 150){ // After kick, detect max kick speed
                    float m = maxReading();
                    m = m + (m < 18 ? SPEED_INCREASE_LOW : SPEED_INCREASE_HIGH);
                    if(m > speedTarget){
                        speedTarget = m;
                        throttlePID.SetTunings(SPEED_PROPORTION_MIN + SPEED_PROPORTION_DIF / SPEED_DIF * (speedTarget-SPEED_MIN), SPEED_INTEGRAL, SPEED_DISTURBANCE);
                    }
                }
                // Calculate duration
                if ((millis() - boostTime) > KICK_DURATION || speedRaw < SPEED_MIN) {
                    throttlePID.SetMode(MANUAL);
                    throttleOut = RAW_TROTTLE_LO + THROTTLE_OFF_PCT * RAW_TROTTLE_SCALAR;
                    speedTarget = 0;
                }
                if ((millis() - boostTime) > (KICK_DURATION + 5000)) {
                    boostCount = 0; /* Reset boost count if last boost ended over 5s ago */
                }
                throttlePID.Compute(); /* Calculate throttle with PID Controller */
                if (DEBUG_MODE >= EVENT && speedCurrent > 0) {
                    Serial.println((String)"PID: " + speedCurrent + "(" + speedRaw + ") " + throttleOut + " " + speedTarget);
                }
            }
            analogWrite(THROTTLE_PIN, (int)throttleOut);
        }
    }
}

void logBuffer(uint8_t buff[256], int len)
{
    uint16_t sum = 0x00;
    Serial.print("DATA: ");
    for (int i = 0; i <= len; i++) {
      Serial.print(buff[i],HEX);
      Serial.print(" ");
    }
    Serial.print("  (HEX) / ");
    for (int i = 0; i <= len; i++) {
      Serial.print(buff[i]);
      sum += buff[i];
      Serial.print(" ");
    }
    Serial.println("  (DEC)");
}

float calcSpeedAvg()
{
  int s = 0;
  for(int i = 0; i<SPEED_READINGS;i++){
      s += speedReadings[i];
  }
  return s / (float)SPEED_READINGS;
}

int maxReading()
{
  int m = 0;
  for(int i = 0; i<SPEED_READINGS;i++){
      m = (speedReadings[i]>m?speedReadings[i]:m);
  }
  return m;
}

float maxDifference(float compare)
{
  int d = 0;
  for (int i = 0; i<SPEED_READINGS;i++) {
      d = (speedReadings[i]-compare>d ? speedReadings[i]-compare : d);
  }
  return d;
}
