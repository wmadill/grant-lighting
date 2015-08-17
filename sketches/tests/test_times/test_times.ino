/**
 * Test various time manipulations using the RTClib and blink
 * the LED on Jeenode port 2 accordingly.
 *
 * 2015-08-16 Bill
 */

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>

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

PortI2C i2cBus (1);
RTC_Plug RTC (i2cBus);
/* Port led_port (2); */

unsigned char start_hh = 14;
unsigned char start_mm = 49;
unsigned char stop_hh = 14;
unsigned char stop_mm = 50;
long start_min = 0;
long stop_min = 0;


long new_time = 0;

long toMin(const unsigned char hh, const unsigned char mm) {
  return (hh * 60) + mm;
}

void showDate(const char* txt, const DateTime& dt) {
    Serial.print(txt);
    Serial.print(' ');
    Serial.print(dt.year(), DEC);
    Serial.print('/');
    Serial.print(dt.month(), DEC);
    Serial.print('/');
    Serial.print(dt.day(), DEC);
    Serial.print(' ');
    Serial.print(dt.hour(), DEC);
    Serial.print(':');
    Serial.print(dt.minute(), DEC);
    Serial.print(':');
    Serial.print(dt.second(), DEC);
    Serial.println();
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[test times]");
    Serial.flush();

    /* led_port.mode(OUTPUT); */
    
    // turn the radio off completely
    rf12_initialize(17, RF12_868MHZ);
    rf12_sleep(RF12_SLEEP);
    // wait another 2s for the power supply to settle
    Sleepy::loseSomeTime(2000);

    /* Serial.print("start min "); */
    /* Serial.print(start_min); */
    /* Serial.print("stop min "); */
    /* Serial.print(stop_min); */
    /* Serial.println(); */
    /* Serial.flush(); */

    // Convert start hh:mm to a start minutes
    start_min = toMin(start_hh, start_mm);
    stop_min = toMin(stop_hh, stop_mm);

    int led_state = 0;
    
    // start both tasks 1.5 seconds from now
    /* scheduler.timer(TASK1, 15); */
    /* scheduler.timer(TASK2, 15); */
}

void loop () {
    // Get time
    DateTime now = RTC.now();
    showDate("dt", now);
    /* Serial.print("secs "); */
    /* Serial.println(now.get()); */
    /* Serial.println(); */

    int sec_til_ten_min = 0;
    sec_til_ten_min = 600L - (now.get() % 600L);

    /* Serial.print("secs until 10 min: "); */
    /* Serial.println(sec_til_ten_min); */

    /* long cur_min = now.hour() * 60 + now.minute(); */
    long cur_min = toMin(now.hour(), now.minute());
    Serial.print("cur min");
    Serial.println(cur_min);

    if ((cur_min >= start_min) && (cur_min < stop_min))  {
      Serial.println("Operational");
    } else {
      Serial.println("Sleep");
    }

    Serial.flush();
    
    // Wait 3 secs and continue
    Sleepy::loseSomeTime(3000);
    /* delay(3000); */

          // Check if should be operational
          // Set flag used
          // Check reschedule amount aganst RTC
}
