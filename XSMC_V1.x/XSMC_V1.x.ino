#include <Arduino.h>
#include <arduino-timer.h>
#include <SoftwareSerial.h>

// ==========================================================================
// ================================ SETUP ===================================
// ==========================================================================
//
// Supported models:
// 0 = Xiaomi Mi Scooter Essential
// 1 = Xiaomi Mi Scooter 1S (Not tested)
// 2 = Xiaomi Mi Scooter Pro 2
//
const int SCOOTERMODEL    = 1;    // Select scooter type
const int DURATION_LOW    = 3000; // Throttle duration (in millisec)
const int DURATION_MID    = 5000; // Throttle duration (in millisec)
const int DURATION_HIGH   = 7000; // Throttle duration (in millisec)
const int THROTTLE_OFF    = 45;   // Throttle setting  (in raw value)
const int THROTTLE_ON     = 80;  // Throttle setting  (in raw value)
const int THROTTLE_LOW    = 140;  // Throttle setting  (in raw value)
const int THROTTLE_MID    = 190;  // Throttle setting  (in raw value)
const int THROTTLE_HIGH   = 233;  // Throttle setting  (in raw value)
const int SPEED_LOW       = 10;    // Speed when to use low throttle/duration (in kmph)
const int SPEED_HIGH      = 15;   // Speed when to use high throttle/duration (in kmph)
const int THROTTLE_DELAY  = 1500; // Time before accepting a new boost (in millisec)
const int READINGS_COUNT  = 30;   // Amount of speed readings
const int THROTTLE_PIN    = 10;   // Pin of programming board (9=D9 or 10=D10)
const int SERIAL_PIN      = 2;    // Pin of serial input (2=D2)

//
// ==========================================================================
// =============================  PROTOCOL ==================================
// ==========================================================================
//
// Message structure:
// + --- + --- + --- + --- + --- + --- + --- + --- + --- +
// | x55 | xAA |  L  |  D  |  T  |  C  | ... | ck0 | ck1 |
// + --- + --- + --- + --- + --- + --- + --- + --- + --- +
//
// x55 = FIXED
// xAA = FIXED
// L   = MESSAGE LENGTH
// D   = DESTINATION (20 = DASH TO MOTOR, 21 = MOTOR TO DASH, 22 = DASH TO BATT, 23 = BATT TO DASH, 24 = MOTOR TO BATT, 25 = BATT TO MOTOR)
// T   = TYPE (1 = READ, 3 = WRITE, 64 = ?, 65 = ?)
// C   = COMMAND (?)
// ... = DATA
// ck0 = CHECKSUM
// ck1 = CHECKSUM
//
// ==========================================================================

// CODE BELOW THIS POINT

SoftwareSerial SoftSerial(SERIAL_PIN, 3); // RX, TX
auto timer_m = timer_create_default();

// States
#define READY 0
#define BOOST 1

// Protocol decoding
#define DASH2MOTOR  0x20
#define MOTOR2DASH  0x21
#define DASH2BATT   0x22
#define BATT2DASH   0x23
#define MOTOR2BATT  0x24
#define BATT2MOTOR  0x25
#define BRAKEVALUE  0x65
#define SPEEDVALUE  0x64
#define SERIALVALUE 0x31

// Variables
bool isBraking = true;                    // brake activated
int speedCurrent = 0;                     // current speed
int speedCurrentAverage = 0;              // the average speed over last X readings
int speedLastAverage = 0;                 // the average speed over last X readings in the last loop
int speedReadings[READINGS_COUNT] = {0};  // the readings from the speedometer
int indexcounter = 0;                     // the indexcounter of the current reading
int speedReadingsSum = 0;                 // the sum of all speed readings
uint8_t state = READY;                    // current state



uint8_t readBlocking()
{
    while (!SoftSerial.available())
    delay(1);
    return SoftSerial.read();
}

void setup()
{
    Serial.begin(115200); SoftSerial.begin(115200); // Start reading SERIAL_PIN at 115200 baud
    TCCR1B = TCCR1B & 0b11111001; // TCCR1B = TIMER 1 (D9 and D10 on Nano) to 32 khz
    ThrottleWrite(THROTTLE_OFF);
}

uint8_t buff[256];
void loop()
{
    while (readBlocking() != 0x55);
    if (readBlocking() != 0xAA) return;

    uint8_t len = readBlocking();
    buff[0] = len;
    if (len > 254) return;

    uint8_t addr = readBlocking();
    buff[1] = addr;
    uint16_t sum = len + addr;
    for (int i = 0; i < len; i++) {
        uint8_t curr = readBlocking();
        buff[i + 2] = curr;
        sum += curr;
    }

    uint16_t checksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
    if (checksum != (sum ^ 0xFFFF)) return;

    switch (buff[1]) {
        case DASH2MOTOR: // Destintion: Dash to motor
            switch (buff[2]) {
                case BRAKEVALUE:
                    isBraking = (buff[6] > 47);
                    if (isBraking) {
                        ThrottleWrite(THROTTLE_OFF);
                    }
                    break;
            }
        case MOTOR2DASH: // Destination: motor to dash
            switch (buff[2]) {
                case SPEEDVALUE:
                    if (buff[8] != 0) {
                        speedCurrent = buff[8];
                    }
                    break;
            }
    }

    speedReadingsSum = speedReadingsSum - speedReadings[indexcounter];
    speedReadings[indexcounter] = speedCurrent;
    speedReadingsSum = speedReadingsSum + speedReadings[indexcounter];
    indexcounter++;
    if (indexcounter >= READINGS_COUNT) {
        indexcounter = 0;
    }
    speedCurrentAverage = speedReadingsSum / READINGS_COUNT;

    motion_control();
    timer_m.tick();
}

bool release_throttle(void *)
{
    // 16% (Keep throttle to disable KERS). best for Essential.
    // 0% (Close trottle). best for Pro 2 & 1S.
    ThrottleWrite(SCOOTERMODEL==0 ? THROTTLE_ON : THROTTLE_OFF);
    // After boost, wait for new boost
    timer_m.in(THROTTLE_DELAY, motion_wait);
    return false;
}

bool motion_wait(void *)
{
    state = READY;
    return false; // false to stop
}

void motion_control()
{
    // If braking or speed is under 5 km/h, stop throttle
    if (speedCurrent < 5 || isBraking) {
        ThrottleWrite(THROTTLE_OFF);
        state = READY;
        timer_m.cancel();
    } else if (state == READY && speedCurrentAverage - speedLastAverage > 0 && speedCurrentAverage > 5) {
        // ready, increasing speed and above 5 kmh
        if (speedCurrentAverage <= SPEED_LOW) {
            ThrottleWrite(THROTTLE_LOW); // 50% throttle
            timer_m.in(DURATION_LOW, release_throttle);
        } else if (speedCurrentAverage >= SPEED_HIGH) {
            ThrottleWrite(THROTTLE_HIGH); // 100% throttle
            timer_m.in(DURATION_HIGH, release_throttle);
        } else {
            ThrottleWrite(THROTTLE_MID); // 77% throttle
            timer_m.in(DURATION_MID, release_throttle);
        }
        state = BOOST;
    }
    speedLastAverage = speedCurrentAverage;
}

/**
 * @brief Set trottle speed raw value
 *
 * @param value the duty cycle: between 0 (always off) and 255 (always on).
 * @return int
 */
void ThrottleWrite(int value)
{
    if (value >= THROTTLE_OFF && value <= THROTTLE_HIGH) {
      analogWrite(THROTTLE_PIN, value);
    }
}
