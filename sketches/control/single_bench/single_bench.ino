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
 * the correct task ran; LED2 flashes once per minute to show the
 * sketch is working.
 */

// Uncomment for LEDs flashing for debug
#define DEBUG_FLASH 1
// Uncomment for debug traces on the serial line
#define DEBUG_TRACE 1 

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>

const byte maxm1Dev = 12;
const byte maxm2Dev = 13;

// Start and end hours of operation
const unsigned char start_hh = 19;
const unsigned char start_mm = 50;
const unsigned char stop_hh = 19;
const unsigned char stop_mm = 51;

// Length of first MaxM light cycle in seconds
// The light scripts on the MaxMs MUST be
// shorter than this value.
// const byte maxm1_cycle = 90;
const byte maxm1_cycle = 30;

// Seconds after starting first MaxM to start second
// const byte maxm2_offset = 30;
const byte maxm2_offset = 15;

// Length of time check cycle in seconds
// const byte check_cycle = 60;
const byte check_cycle = 15;

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

long toMin(const unsigned char hh, const unsigned char mm) {
    return (hh * 60) + mm;
}

void ledOn (const Port ledPort, byte mask) {
  if (mask & 1) {
    ledPort.digiWrite(1);
  }
  if (mask & 2) {
    ledPort.digiWrite2(1);
  }
}

void ledOff (const Port ledPort, byte mask) {
  if (mask & 1) {
    ledPort.digiWrite(0);
  }
  if (mask & 2) {
    ledPort.digiWrite2(0);
  }
}

void doFlash(const Port ledPort, const byte mask, const byte cnt) {
#ifdef DEBUG_FLASH
    byte cntr = cnt;
    while (cntr > 0) {
        cntr--;
        ledOn(ledPort, mask);
        Sleepy::loseSomeTime(50);
        ledOff(ledPort, mask);
        if (cntr < 1) break;
        Sleepy::loseSomeTime(250);
    }
#endif
}

// RTC on port 1
PortI2C RTCbus (1);
RTC_Plug RTC (RTCbus);

// MaxMs on port 2
PortI2C MaxMbus (2);
DeviceI2C maxm1 (MaxMbus, maxm1Dev);
DeviceI2C maxm2 (MaxMbus, maxm2Dev);

// LEDs are on port 4
Port ledPort (4);

// 1 = lights working, 0 = lights not
byte is_operational = 0;

// Set to 1 when MaxM 1 is active
byte maxm1_active = 0;

long start_min = 0;
long stop_min = 0;

// Add more tasks for second bench
enum { MAXM1, MAXM2, OPER_TIME, TASK_LIMIT };

static word schedBuf[TASK_LIMIT];
Scheduler scheduler (schedBuf, TASK_LIMIT);

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

    // Convert start hh:mm to a start minutes
    start_min = toMin(start_hh, start_mm);
    stop_min = toMin(stop_hh, stop_mm);

    // The MaxMs are supposed to be programmed to not start on power-on
    // but turn thenm off just to be sure
    //TODO check for errors; and then do what?
    maxm1.send();
    maxm1.write('o');
    maxm1.stop();
    maxm1.send();
    maxm1.write('n');
    maxm1.write(0);
    maxm1.write(0);
    maxm1.write(0);
    maxm1.stop();
    maxm2.send();
    maxm2.write('o');
    maxm2.stop();
    maxm2.send();
    maxm2.write('n');
    maxm2.write(0);
    maxm2.write(0);
    maxm2.write(0);
    maxm2.stop();

    // Check the RTC is working correctly
    DateTime t1 = RTC.now();
    // Wait 2 seconds and see if the clock changes
    Sleepy::loseSomeTime(2000);
    DateTime t2 = RTC.now();

    // If no time elapsed, RTC is stopped so blink both LEDs 8 times
    if (t1.second() == t2.second()) {
        doFlash(ledPort, 1 + 2, 8);
    }

    // Start the main task
    scheduler.timer(OPER_TIME, 0);
}

void loop () {
    switch (scheduler.pollWaiting()) {
        // Control first MaxM
        case MAXM1:
            if (is_operational == 1) {
                scheduler.timer(MAXM1, maxm1_cycle * 10);
                scheduler.timer(MAXM2, maxm2_offset * 10);
                doFlash(ledPort, led1, 1);
#ifdef DEBUG_TRACE
                Serial.println("MAXM1");
                Serial.flush();
#endif
                maxm1.send();
                maxm1.write('p');
                maxm1.write(0);
                maxm1.write(1);
                maxm1.write(0);
                maxm1.stop();
            } 
            break;

        // Control second MaxM
        case MAXM2:
            maxm2.send();
            maxm2.write('p');
            maxm2.write(0);
            maxm2.write(1);
            maxm2.write(0);
            maxm2.stop();

            doFlash(ledPort, led1, 2);
#ifdef DEBUG_TRACE
            Serial.println("MAXM2");
            Serial.flush();
#endif
            break;

        // Check if within operational time
        case OPER_TIME:
            // Get time
            DateTime now = RTC.now();

            // Check every check_cycle seconds
            int sec_til_check = check_cycle - (now.get() % check_cycle);
            scheduler.timer(OPER_TIME, sec_til_check * 10);

#ifdef DEBUG_TRACE
            Serial.print("RTC time: ");
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
            Serial.print(now.second());
            Serial.print(" dow: ");
            Serial.println(now.dayOfWeek());
            Serial.flush();
#endif
            // Check if should be operational and set flag 
            long cur_min = toMin(now.hour(), now.minute());
            if ((cur_min >= start_min) && (cur_min < stop_min))  {
                is_operational = 1;
                // Start the MAXM1 task if not already active
                if (maxm1_active == 0) {
                  scheduler.timer(MAXM1, 0);
                  maxm1_active = 1;
                }
#ifdef DEBUG_TRACE
                Serial.println("Lights are operational");
                Serial.flush();
#endif
            } else {
                is_operational = 0;
                maxm1_active = 0;
                //TODO consider turning off MaxMs like in setup()
#ifdef DEBUG_TRACE
                Serial.println("Lights are sleeping");
                Serial.flush();
#endif
            }

            // Flash LED2 one for every minute
            doFlash(ledPort, led2, 1);
            break;
    }
}
