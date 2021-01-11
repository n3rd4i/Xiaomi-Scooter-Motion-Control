#ifndef CONFIG_H_
#define CONFIG_H_

// States
#define READY 0
#define BOOST 1

// Debug modes
#define SNIFFER     -1
#define NONE        0
#define EVENT       1
#define ALL         2

// Destination // Source in comments
#define BROADCAST_TO_ALL  0x00
#define ESC               0x20 // Xiaomi: From BLE / Ninebot: From any
#define BLE               0x21 // Xiaomi: From ESC / Ninebot: From any
#define BMS               0x22 // Xiaomi: From BLE / Ninebot: From any

// Command
// 0x20 BLE>ESC
#define BRAKE   0x65
// 0x21 ESC>BLE
#define SPEED   0x64

// ==========================================================================
// =============================== CONFIG ===================================
// ==========================================================================
//
const float LIMIT              = 1.5;   // What speed increase limit in km/h to activate boost
const int   DURATION[3]        = {1000, 3000, 8000};  // Duration of Nth boost {1st,2nd,3rd}
const int   THROTTLE_MIN_KMH   = 5;     // What speed to start throttling
const int   THROTTLE_MAX_KMH   = 18;    // What speed to give max throttle (in km/h, we recommend vMax-5)
const int   THROTTLE_IDLE_PCT  = 0;     // Maximum is 10 to disable KERS when not braking above THROTTLE_MIN_KMH on stock firmware
const int   THROTTLE_MIN_PCT   = 30;    // Throttle minimum to set power at or below THROTTLE_MIN_KMH (71 for 350W motor, but we recommend adapting the firmware instead)
const int   THROTTLE_MAX_PCT   = 100;   // Throttle maximum to set power at or below THROTTLE_MIN_KMH (71 for 350W motor, but we recommend adapting the firmware instead)
const int   BRAKE_LIMIT        = 48;    // Limit for disabling throttle when pressing brake pedal (we recommend setting this as low as possible)
const int   THROTTLE_PIN       = 10;    // Pin of programming board (9=D9 or 10=D10)
const int   SERIAL_PIN         = 2;     // Pin of serial input (2=D2)
const int   DEBUG_MODE         = NONE;  // Debug mode (NONE for no logging, EVENT for event logging, ALL for serial data logging)
const int   THROTTLE_DIFF_KMH  = THROTTLE_MAX_KMH - THROTTLE_MIN_KMH;
const int   THROTTLE_DIFF_PCT  = THROTTLE_MAX_PCT - THROTTLE_MIN_PCT;

#endif /* CONFIG_H_ */
