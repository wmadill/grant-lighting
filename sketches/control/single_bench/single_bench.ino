/**
 * Control the BlinkMs for a single bench with one painting. Fire off
 * the appropriate MaxMs at the correct intervals during the daily
 * operational time.
 *
 * The MaxMs that actually control the light pattern for one group
 * of LED clusters (Blasters) are preloaded with their light
 * scripts. The MaxMs are connected over an I2C and have device ids
 * preset also. Note that the RTC has device id 0x68 whcih cannot
 * be changed.
 *
 * TODO finish description of operation
 */

#define DEBUG_FLASH
/* #define DEBUG_TRACE */

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>

const byte maxm1Dev = 12;
const byte maxm2Dev = 13;

// Start and end hours of operation
const unsigned char start_hh = 9;
const unsigned char start_mm = 0;
const unsigned char stop_hh = 22;
const unsigned char stop_mm = 0;

// Seconds after starting first MaxM to start second
const byte maxm2_offset = 30;

// boilerplate for low-power waiting
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

#ifdef DEBUG_FLASH
void doFlash(const Port ldnum, const byte cnt) {
    byte cntr = cnt;
    while (cntr > 0) {
        cntr--;
        ldnum.digiWrite(1);
        Sleepy::loseSomeTime(50);
        ldnum.digiWrite(0);
        if (cntr < 1) break;
        Sleepy::loseSomeTime(250);
    }
}
#endif

// RTC on port 1
PortI2C RTCbus (1);
RTC_Plug RTC (RTCbus);

// MaxMs on port 2
PortI2C MaxMbus (2);
DeviceI2C maxm1 (MaxMbus, maxm1Dev);
DeviceI2C maxm2 (MaxMbus, maxm2Dev);

#ifdef DEBUG_FLASH
// LEDs are on ports 3 and 4
// LED1 flashes once for the MAXM1 task, twice for the MAXM2 task
Port led1 (3);
// LED2 flashes once per 10 seconds in a minute (1..6)
Port led2 (4);

byte led1_on = 0;
byte led2_on = 0; 
#endif

int is_operational = 0;

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

    // Turn the radio off completely
    rf12_initialize(17, RF12_868MHZ);
    rf12_sleep(RF12_SLEEP);

    // Convert start hh:mm to a start minutes
    start_min = toMin(start_hh, start_mm);
    stop_min = toMin(stop_hh, stop_mm);

#ifdef DEBUG_FLASH
    led1.mode(OUTPUT);
    led2.mode(OUTPUT);
#endif

    // The MaxMs should be programmed to not start on poweroff
    // but turn thenm off just to be sure

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

    // Start tasks after a pause for the power supply to settle
    scheduler.timer(OPER_TIME, 15);
    scheduler.timer(MAXM1, 20);
}

void loop () {
    switch (scheduler.pollWaiting()) {
        // Control first MaxM
        case MAXM1:
            maxm1.send();
            maxm1.write('p');
            maxm1.write(0);
            maxm1.write(1);
            maxm1.write(0);
            maxm1.stop();
#ifdef DEBUG_TRACE
            Serial.print("MAXM1 is operational ");
            Serial.println(is_operational);
            Serial.flush();
#endif
            if (is_operational == 1) {
#ifdef DEBUG_FLASH
                doFlash(led1, 1);
#endif
                // TODO need correct amount to reschedule
#ifdef DEBUG_TRACE
                Serial.println("MAXM1");
                Serial.flush();
#endif
            }

            scheduler.timer(MAXM2, maxm2_offset * 10);
            scheduler.timer(MAXM1, 900);
            break;

        // Control second MaxM
        case MAXM2:
            if  (is_operational != 1) break;
            maxm2.send();
            maxm2.write('p');
            maxm2.write(0);
            maxm2.write(1);
            maxm2.write(0);
            maxm2.stop();

#ifdef DEBUG_FLASH
            doFlash(led1, 2);
#endif
            break;

        // Check if within operational time
        case OPER_TIME:
            // Get time
            DateTime now = RTC.now();

            // Loop every 60 seconds
            const int loop_interval = 60;
            int sec_til_loop = loop_interval - (now.get() % loop_interval);

            // Check if should be operational and set flag
            long cur_min = toMin(now.hour(), now.minute());
            if ((cur_min >= start_min) && (cur_min < stop_min))  {
#ifdef DEBUG_TRACE
                Serial.println("Operational");
                Serial.flush();
#endif
                is_operational = 1;
#ifdef DEBUG_TRACE
                Serial.print("is operational ");
                Serial.println(is_operational);
                Serial.flush();
#endif
            } else {
#ifdef DEBUG_TRACE
                Serial.println("Sleep");
                Serial.flush();
#endif
                is_operational = 0;
            }

#ifdef DEBUG_FLASH
            // Flash LED2 one for every minute in ten if "operational"
            // otherwise only 1 flash
            int num_flash = 1;
            if (is_operational == 1) {
                num_flash = (now.get() % 600);
                num_flash = num_flash / 60;
            }
            doFlash(led2, num_flash);
#endif

            // Check reschedule amount aganst RTC
            scheduler.timer(OPER_TIME, sec_til_loop * 10);
            break;
    }
}
