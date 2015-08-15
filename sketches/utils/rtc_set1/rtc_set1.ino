/**
 * Script to set the date on an RTC on the I2C bus at device 0x68.
 * This is cobbled together from  jcw's "rtc_demo" sketch for setting
 * the date and time and from his "parser_demo" sketch for reading
 * the time from the serial interface.
 *
 * The interface is still somewhat clunky because the time
 * setting requires dots after each number. For example,
 * setting the clock to "2015-08-14 15:04:10" means entering
 * "2015. 08. 14. 15. 04. 10. c". However the RTC is updated
 * immediately after the command is entered so you can set the RTC 
 * accurately.
 *
 * Original "rtc_demo" sketch:
 * 2009-09-17 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Original "parser_demo" sketch:
 * 2010-10-23 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Modifications
 * 2015-08-13 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>

PortI2C myport (1 /*, PortI2C::KHZ400 */);
DeviceI2C rtc (myport, 0x68);

extern InputParser::Commands cmdTab[] PROGMEM;
InputParser parser (50, cmdTab);
int clock_set = 0;

static byte bin2bcd (byte val) {
    return val + 6 * (val / 10);
}

static byte bcd2bin (byte val) {
    return val - 6 * (val >> 4);
}

static void setDate (byte yy, byte mm, byte dd, byte h, byte m, byte s) {
    rtc.send();
    rtc.write(0);
    rtc.write(bin2bcd(s));
    rtc.write(bin2bcd(m));
    rtc.write(bin2bcd(h));
    rtc.write(bin2bcd(0));
    rtc.write(bin2bcd(dd));
    rtc.write(bin2bcd(mm));
    rtc.write(bin2bcd(yy));
    rtc.write(0);
    rtc.stop();
}

static void getDate (byte* buf) {
	rtc.send();
	rtc.write(0);	
    rtc.stop();

	rtc.receive();
    buf[5] = bcd2bin(rtc.read(0));
    buf[4] = bcd2bin(rtc.read(0));
    buf[3] = bcd2bin(rtc.read(0));
    rtc.read(0);
    buf[2] = bcd2bin(rtc.read(0));
    buf[1] = bcd2bin(rtc.read(0));
    buf[0] = bcd2bin(rtc.read(1));
    rtc.stop();
}

static void setClockCmd () {
    int y, m, d, h, i, s;
    parser >> y >> m >> d >> h >> i >> s;
    Serial.print("Setting clock to ");
    Serial.print(y);
    Serial.print("-");
    Serial.print(m);
    Serial.print("-");
    Serial.print(d);
    Serial.print(" ");
    Serial.print(h);
    Serial.print(":");
    Serial.print(i);
    Serial.print(":");
    Serial.print(s);
    Serial.println();
    setDate(y, m, d, h, i, s);
    clock_set = 1;
}

InputParser::Commands cmdTab[] = {
    { 'c', 6, setClockCmd },
    { 0 }    
};

void setup() {
    Serial.begin(57600);
    Serial.println("\n[Set the RTC data/time]");
    // Be sure to follow each field with a dot!
    Serial.println("\n[Input y m d h i s c ]");
}

void loop() {    
    // Read date/time from serial line
    while (clock_set != 1) {
      parser.poll();
    }

    // test code:
    byte now[6];
    getDate(now);
    
    Serial.print("rtc");
    for (byte i = 0; i < 6; ++i) {
        Serial.print(' ');
        Serial.print((int) now[i]);
    }
    Serial.println();
        
    delay(1000);
 }
