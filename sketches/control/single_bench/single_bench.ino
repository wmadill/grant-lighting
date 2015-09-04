/**
 * Control the BlinkMs for a single bench with one painting. Fire off
 * the appropriate MaxMs at the correct intervals during the daily
 * operational time.
 *
 * The MaxMs that actually control the light pattern for one group
 * of LED clusters (Blasters) are preloaded with their light scripts.
 * The MaxMs are connected over an I2C and have device ids preset
 * also. Note that the RTC has device id 0x68 whcih cannot be changed.
 *
 * There is an RTC plug on Jeenode port 1, the MaxMs are on the I2C 
 * bus on port 2, and optionally there are 2 status LEDs on port 4
 * being independently controlled with the DIO and AIO pins. LED1
 * flashes once for the MAXM1 task, twice for the MAXM2 task to show
 * the correct task ran; LED2 flashes twice per minute when the
 * lights are awake and once per minute when they are asleep.
 */

// Uncomment for LEDs flashing for debug
#define DEBUG_FLASH
// Uncomment for debug traces on the serial line
// #define DEBUG_TRACE 

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>
#include "benches.h"

const byte maxm1Dev = 13;
const byte maxm2Dev = 12;

// Length of first MaxM light cycle in seconds
// The light scripts on the MaxMs MUST be
// shorter than this value.
const byte maxm1_cycle = 75;
// const byte maxm1_cycle = 30;

// Seconds after starting first MaxM to start second
const byte maxm2_offset = 20;
// const byte maxm2_offset = 15;

// Length of time check cycle in seconds
const word check_cycle = 60;
// const word check_cycle = 15;

// Number of time check cycles before refreshing the millis()-based
// RTC from real RTC. For example with the time check cycle is 60
// (once per minute), set the RTC_refresh_cycle to 720 to refresh
// every 12 hours.
// const int mRTC_refresh_cycle = 720;
const int mRTC_refresh_cycle = 5;

// LEDs
const byte led1 = 1;
const byte led2 = 2;

// Boilerplate for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// RTC based on the DS1307 chip connected via the Ports library
class RTC_Plug : public DeviceI2C {
    // shorthand
    static uint8_t bcd2bin (uint8_t val) { return RTC_DS1307::bcd2bin(val); }
    static uint8_t bin2bcd (uint8_t val) { return RTC_DS1307::bin2bcd(val); }
public:
    RTC_Plug (const PortI2C& port) : DeviceI2C (port, 0x68) {}

    void begin() {}
    
    void adjust(const DateTime& dt) {
        send();
        write(0);
        write(bin2bcd(dt.second()));
        write(bin2bcd(dt.minute()));
        write(bin2bcd(dt.hour()));
        write(bin2bcd(0));
        write(bin2bcd(dt.day()));
        write(bin2bcd(dt.month()));
        write(bin2bcd(dt.year() - 2000));
        write(0);
        stop();
    }

    DateTime now() {
      	send();
      	write(0);	
        stop();

        receive();
        uint8_t ss = bcd2bin(read(0));
        uint8_t mm = bcd2bin(read(0));
        uint8_t hh = bcd2bin(read(0));
        read(0);
        uint8_t d = bcd2bin(read(0));
        uint8_t m = bcd2bin(read(0));
        uint16_t y = bcd2bin(read(1)) + 2000;
    
        return DateTime (y, m, d, hh, mm, ss);
    }
};

// RTC on port 1
PortI2C RTCbus (1);
RTC_Plug RTC (RTCbus);

// MaxMs on port 2
PortI2C MaxMbus (2);
DeviceI2C maxm1 (MaxMbus, maxm1Dev);
DeviceI2C maxm2 (MaxMbus, maxm2Dev);

// LEDs on port 4
Port ledPort (4);

// millis()-based RTC 
RTC_Millis millisRTC;

// Currest millisRTC cycle
int cur_mRTC_cycle = 0;

// Set to 1 when the MaxMs are awake (active)
byte maxms_awake = 0;

// Wake periods. There are some serious constraints on this list
// which are not checked but violations will cause unintended
// results!
// 1. The entries must be in ascending order by day and 
//    start time.
// 2. The sleep time of one period must be earlier than the
//    wake time of the next period.
// 3. This implies that the periods cannot overlap.

// Set to Orcas Center schedule 9/2015
Wakeperiod wps[] = {
    {4,  hm2min(10, 0),  hm2min(22, 0)},
    {5,  hm2min(17, 30), hm2min(22, 0)},
    {6,  hm2min(17, 30), hm2min(22, 0)},
    {8,  hm2min(10, 0),  hm2min(17, 0)},
    {9,  hm2min(10, 0),  hm2min(17, 0)},
    {10, hm2min(10, 0),  hm2min(17, 0)},
    {11, hm2min(10, 0),  hm2min(17, 0)},
    {15, hm2min(10, 0),  hm2min(17, 0)},
    {16, hm2min(10, 0),  hm2min(17, 0)},
    {17, hm2min(10, 0),  hm2min(17, 0)},
    {18, hm2min(10, 0),  hm2min(17, 0)},
    {18, hm2min(18, 30), hm2min(22, 0)},
    {19, hm2min(17, 0),  hm2min(22, 0)},
    {22, hm2min(10, 0),  hm2min(17, 0)},
    {23, hm2min(10, 0),  hm2min(17, 0)},
    {24, hm2min(10, 0),  hm2min(17, 0)},
    {25, hm2min(10, 0),  hm2min(17, 0)},
    {26, hm2min(17, 30), hm2min(22, 0)},
    {29, hm2min(10, 0),  hm2min(17, 0)},
};
byte num_wps = sizeof(wps) / sizeof(wps[0]);

// Add more tasks for second bench
enum { MAXM1, MAXM2, CHK_TIME, TASK_LIMIT };

static word schedBuf[TASK_LIMIT];
Scheduler scheduler (schedBuf, TASK_LIMIT);

// Initialization
void setup () {
#ifdef DEBUG_TRACE
    Serial.begin(57600);
    Serial.println("Single bench start");
    Serial.flush();
#endif

    // Start the clock
    // Not sure this is needed since it is a null function
    RTC.begin();

    ledPort.mode(OUTPUT);
    ledPort.mode2(OUTPUT);

    // Flash both LEDs to show setup starting
    doFlash(ledPort, 1 + 2, 1);

    // Turn the radio off completely
    rf12_initialize(17, RF12_868MHZ);
    rf12_sleep(RF12_SLEEP);

    // The MaxMs are supposed to be programmed to not start on power-on
    // but turn thenm off just to be sure
    //TODO check for errors; and then do what?
    maxmOff(maxm1);
    maxmOff(maxm2);

    // Check the RTC is working correctly
    DateTime t1 = RTC.now();
    // Wait 2 seconds and see if the clock changes
    Sleepy::loseSomeTime(2000);
    DateTime t2 = RTC.now();

    // If no time elapsed, RTC is stopped so blink both LEDs 8 times
    if (t1.second() == t2.second()) {
        doFlash(ledPort, 1 + 2, 8);
    }

    // Initialize millisRTC for time check task
    millisRTC.begin(t2);
    cur_mRTC_cycle = mRTC_refresh_cycle; 
    
#ifdef DEBUG_TRACE
    // Dump wps array
    for (int i = 0; i < num_wps; i++) {
      Serial.print("wp[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.print(wps[i].day);
      Serial.print(", ");
      Serial.print(wps[i].wake);
      Serial.print(",");
      Serial.println(wps[i].sleep);
      Serial.flush();
    }
#endif

    // Start the main task
    scheduler.timer(CHK_TIME, 0);
}

void loop () {
    switch (scheduler.pollWaiting()) {
        // Control first MaxM
        case MAXM1:
            if (!maxms_awake) break;

            scheduler.timer(MAXM1, (word) (maxm1_cycle * 10));
            scheduler.timer(MAXM2, (word) (maxm2_offset * 10));
            doFlash(ledPort, led1, 1);
            maxmRun(maxm1);
#ifdef DEBUG_TRACE
            Serial.println("MAXM1");
            Serial.flush();
#endif
            break;

        // Control second MaxM
        case MAXM2:
            if (!maxms_awake) break;

            doFlash(ledPort, led1, 2);
            maxmRun(maxm2);

#ifdef DEBUG_TRACE
            Serial.println("MAXM2");
            Serial.flush();
#endif
            break;

        // Check if awake or asleep
        case CHK_TIME:
            // Reset millis()-based RTC from real one if the time
            // cycles have elapsed
            if (--cur_mRTC_cycle < 0) {
#ifdef DEBUG_TRACE
                DateTime t_mrtc = millisRTC.now();
                Serial.print("mRTC time: ");
                Serial.print(t_mrtc.year());
                Serial.print('-');
                Serial.print(t_mrtc.month());
                Serial.print('-');
                Serial.print(t_mrtc.day());
                Serial.print(' ');
                Serial.print(t_mrtc.hour());
                Serial.print(':');
                Serial.print(t_mrtc.minute());
                Serial.print(':');
                Serial.println(t_mrtc.second());
                Serial.flush();
                DateTime t_rtc = RTC.now();
                Serial.print("RTC time: ");
                Serial.print(t_rtc.year());
                Serial.print('-');
                Serial.print(t_rtc.month());
                Serial.print('-');
                Serial.print(t_rtc.day());
                Serial.print(' ');
                Serial.print(t_rtc.hour());
                Serial.print(':');
                Serial.print(t_rtc.minute());
                Serial.print(':');
                Serial.println(t_rtc.second());
                Serial.flush();
#endif
                millisRTC.adjust(RTC.now());
                cur_mRTC_cycle = mRTC_refresh_cycle; 
#ifdef DEBUG_TRACE
                Serial.println("mRTC refreshed");
                Serial.flush();
#endif
            } 

            // Get time
            DateTime now = millisRTC.now();

            // Check every check_cycle seconds
            word sec_til_check = check_cycle - (now.get() % check_cycle);
            // Never check sooner than 2 seconds. A perhaps futile attempt
            // to prevent RTC clock from being interrogated too often
            if (sec_til_check < 2) sec_til_check = 2;
            // TODO if sec_til_check is shorter than the check cycle by
            // more than a couple seconds, then reschedule and break so
            // the lights start their cycle with the CHK_TIME cycle.
            scheduler.timer(CHK_TIME, sec_til_check * 10);

#ifdef DEBUG_TRACE
            Serial.print("mRTC time: ");
            Serial.print(now.year());
            Serial.print('-');
            Serial.print(now.month());
            Serial.print('-');
            Serial.print(now.day());
            Serial.print(' ');
            Serial.print(now.hour());
            Serial.print(':');
            Serial.print(now.minute());
            Serial.print(':');
            Serial.println(now.second());
            Serial.flush();
#endif
            // Check if should be awake or asleep
            if (checkAwake(now, wps, num_wps)) {
                // Start the MAXM1 task if not already active
                if (!maxms_awake) {
                  maxms_awake = 1;
                  scheduler.timer(MAXM1, 0);
                }
                // Flash LED2 twice every minute
                doFlash(ledPort, led2, 2);
#ifdef DEBUG_TRACE
                Serial.println("Awake");
                Serial.flush();
#endif
            } else {
                maxms_awake = 0;
                // Flash LED2 once every minute
                doFlash(ledPort, led2, 1);
                // Turn off MaxMs 
                maxmOff(maxm1);
                maxmOff(maxm2);
#ifdef DEBUG_TRACE
                Serial.println("Asleep");
                Serial.flush();
#endif
            }
            break;
    }
}
