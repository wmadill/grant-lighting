/**
 * Script to read and optionally set the date on an RTC at device id
 * 0x68 on the I2C bus on Jeenode port 1. The is mostly jcw's
 * "plugrtc" sketch from his RTClib example directory.
 *
 * Bill added code to optionally set the clock from 30 seconds after
 * the compiled date/time.
 *
 * Plug the RTC into Jeenode port 1, get an accurate time perhaps
 * by opening http://time.gov on your browser, compile and upload
 * this sketch. Open the serial monitor and type "Y" when the 
 * actual time matches the time the sketch says it will set the clock
 * to.
 *
 * Original sketch:
 * 2010-02-04 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Modifications
 * 2015-08-15 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>
#include <Wire.h> // needed to avoid a linker error :(
#include <RTClib.h>

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
byte incoming_byte = 0;
int time_set = 0;

long new_time = 0;

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
    
    /* Serial.print(" = "); */
    /* Serial.print(dt.get()); */
    /* Serial.print("s / "); */
    /* Serial.print(dt.get() / 86400L); */
    /* Serial.print("d since 2000"); */
    
    Serial.println();
}

void setup () {
    Serial.begin(57600);
    RTC.begin();
    DateTime dt1 (__DATE__, __TIME__);
    new_time = dt1.get() + 30;
    DateTime dt2 (new_time);
    Serial.print("Type 'Y' to change the time to ");
    showDate(" ", dt2);
}

void loop () {
    if (Serial.available() > 0) {
        incoming_byte = Serial.read();
        if ((incoming_byte == 'Y') && (time_set == 0)) {
            RTC.adjust(DateTime(new_time));
            time_set = 1;
        }
    }
    DateTime now = RTC.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since 2000 = ");
    Serial.print(now.get());
    Serial.print("s = ");
    Serial.print(now.get() / 86400L);
    Serial.println("d");
    
    Serial.println();
    delay(1000);
}
