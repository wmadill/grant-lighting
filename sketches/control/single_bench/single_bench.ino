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
const unsigned char start_hh = 9;
const unsigned char start_mm = 0;
const unsigned char stop_hh = 17;
const unsigned char stop_mm = 0;

// Seconds after starting first MaxM to start second
const byte maxm2_offset = 30;

// LEDs
const byte led1 = 1;
const byte led2 = 2;

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

void doFlash(const Port ledPort, const byte ledNum, const byte cnt) {
#ifdef DEBUG_FLASH
    /* Serial.print("Flash LED "); */
    /* Serial.print(ledNum); */
    /* Serial.print(" "); */
    /* Serial.print(cnt); */
    /* Serial.println(" times"); */
    /* Serial.flush(); */

    byte cntr = cnt;
    while (cntr > 0) {
        cntr--;
        if (ledNum == 1) {
          ledPort.digiWrite(1);
          Sleepy::loseSomeTime(50);
          ledPort.digiWrite(0);
        } else {
          ledPort.digiWrite2(1);
          Sleepy::loseSomeTime(50);
          ledPort.digiWrite2(0);
        }

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

    ledPort.mode(OUTPUT);
    ledPort.mode2(OUTPUT);

    // The MaxMs are supposed to be programmed to not start on poweron
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

    // Start tasks after a pause for the power supply to settle
    scheduler.timer(OPER_TIME, 15);
    scheduler.timer(MAXM1, 20);
}

void loop () {
    switch (scheduler.pollWaiting()) {
        // Control first MaxM
        case MAXM1:
#ifdef DEBUG_TRACE
            Serial.print("MAXM1 operational: ");
            Serial.println(is_operational);
            Serial.flush();
#endif
            if (is_operational == 1) {
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

            doFlash(ledPort, led1, 2);
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
                is_operational = 1;
#ifdef DEBUG_TRACE
                Serial.println("Lights are operational");
                Serial.flush();
#endif
            } else {
                is_operational = 0;
#ifdef DEBUG_TRACE
                Serial.println("Lights are sleeping");
                Serial.flush();
#endif
            }

            // Flash LED2 one for every minute in ten if "operational"
            // otherwise only 1 flash
            int num_flash = 1;
            // Actually just flash one every minute
            // if (is_operational == 1) {
                // num_flash = (now.get() % 600);
                // num_flash = num_flash / 60;
            // }
            doFlash(ledPort, led2, num_flash);

            // Check reschedule amount aganst RTC
            scheduler.timer(OPER_TIME, sec_til_loop * 10);
            break;
    }
}
