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
const int DURATION_LOW    = 1000; // Throttle duration (in millisec)
const int DURATION_HIGH   = 7000; // Throttle duration (in millisec)
const int THROTTLE_LOW    = 50;   // Throttle setting  (in full percentage)
const int THROTTLE_HIGH   = 100;  // Throttle setting  (in full percentage)
const int SPEED_LOW       = 5;    // Speed when to use low throttle/duration (in kmph)
const int SPEED_HIGH      = 15;   // Speed when to use high throttle/duration (in kmph)
const int THROTTLE_DELAY  = 1000; // Time before accepting a new boost (in millisec)
const int READINGS_COUNT  = 32;   // Amount of speed readings
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
    setThrottle(0);
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
                        setThrottle(0);
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
    // 10% (Keep throttle to disable KERS). best for Essential.
    // 0% (Close trottle). best for Pro 2 & 1S.
    setThrottle(SCOOTERMODEL==0 ? 10 : 0);
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
        setThrottle(0);
        state = READY;
        timer_m.cancel();
    } else if (state == READY && speedCurrentAverage - speedLastAverage > 0 && speedCurrentAverage > 5) {
        // ready, increasing speed and above 5 kmh
        if (speedCurrentAverage <= SPEED_LOW) {
            setThrottle(THROTTLE_LOW);
            timer_m.in(DURATION_LOW, release_throttle);
        } else if (speedCurrentAverage >= SPEED_HIGH) {
            setThrottle(THROTTLE_HIGH);
            timer_m.in(DURATION_HIGH, release_throttle);
        } else {
            int speedDiff = SPEED_HIGH - SPEED_LOW;
            int throttleDiff = THROTTLE_HIGH - THROTTLE_LOW;
            int durationDiff = DURATION_HIGH - DURATION_LOW;
            setThrottle(THROTTLE_LOW + throttleDiff/speedDiff * (speedCurrentAverage - SPEED_LOW));
            timer_m.in(DURATION_LOW + durationDiff/speedDiff * (speedCurrentAverage - SPEED_LOW), release_throttle);
        }
        state = BOOST;
    }
    speedLastAverage = speedCurrentAverage;
}

/**
 * @brief Set trottle speed in percentage
 *
 * @param percentage whole numbers: [0, 100]
 * @return int value of [45, 233]
 */
void setThrottle(int percentage)
{
    analogWrite(THROTTLE_PIN, percentage * 1.88 + 45);
}
