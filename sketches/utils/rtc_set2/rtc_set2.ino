/**
 * Script to read or set the date on an RTC on the I2C bus at device
 * 0x68.  This is cobbled together from  jcw's "rtc_demo" sketch for
 * setting the date and time using the RTClib.
 *
 * Original "rtc_demo" sketch:
 * 2009-09-17 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Modifications
 * 2015-08-13 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>

PortI2C myport (1 /*, PortI2C::KHZ400 */);
DeviceI2C rtc (myport, 0x68);

int incomingByte = 0;
byte now[6];

uint8_t yOff, m, d, hh, mm, ss;

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

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

void setup() {
  Serial.begin(57600);
  Serial.println("Type 'Y' to change the time");
}

void loop() {    
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if (incomingByte == 'Y') {
      Serial.println("Set the clock");
      const char* date = __DATE__;
      const char* time = __TIME__;

      yOff = conv2d(date + 9);

      // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
      switch (date[0]) {
        case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
      }
      d = conv2d(date + 4);
      hh = conv2d(time);
      mm = conv2d(time + 3);
      ss = conv2d(time + 6);
      setDate(yOff, m, d, hh, mm, ss);
    }
  }

  getDate(now);
    
    Serial.print("rtc");
    for (byte i = 0; i < 6; ++i) {
      Serial.print(' ');
      Serial.print((int) now[i]);
    }
    Serial.println();
        
    delay(1000);
 }
