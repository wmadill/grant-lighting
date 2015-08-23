/**
 * Script to scan an I2C bus for devices. Blink an LED on Jeenode
 * port 3 to show the sketch is active.
 *
 * The concept for this sketch came from todbot's work on BlinkM's. See
 * http://todbot.com/blog/2009/11/29/i2cscanner-pde-arduino-as-i2c-bus-scanner/
 *
 * I2C pins
 *
 * JN port pins
 * (1) not used
 * (2) DIO as SDA
 * (3) GND
 * (4) VCC (+)
 * (5) AIO as SCL
 * (6) not used
 *
 * BlinkM pins
 * (1) GND
 * (2) VCC (+)
 * (3) SDA
 * (4) SCL
 *
 * Pin mappings from Jeenode to BlinkM
 * Jeenode   BlinkM
 * Pin 2     Pin 3
 * Pin 3     Pin 1 
 * Pin 4     Pin 2
 * Pin 5     Pin 4
 *
 * Be sure to set the Jeenode port number.
 * Set the BlinkM to be powered externally and power
 * it or it won't respond
 *
 * @author Bill <bill@jamimi.com>
 * @license http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>

// Jeenode port to scan
const byte portNum = 2;

// The I2C bus and device objects
PortI2C i2cBus (portNum);
// Device address is set during scan
DeviceI2C dev (i2cBus, 0);

// Blink an LED on Jeenode port 3 to show the sketch is active
Port led (3);

const byte loDevId = 1;
const byte hiDevId = 127;

void setup () {
  Serial.begin(57600);
  Serial.print("\nScan I2C bus on port ");
  Serial.println(portNum);

  led.mode(OUTPUT);

  // Scan for devices
  for (byte devid = loDevId; devid <= hiDevId; devid++) {
    dev.setAddress(devid);
    Serial.print(devid, DEC);
    Serial.print(": ");
    dev.isPresent() ? Serial.print("yes") : Serial.print("no");
    devid % 8 ? Serial.print('\t') : Serial.print('\n');
  }
}

void loop () {
  // Blink the LED
  led.digiWrite(1);
  delay(50);
  led.digiWrite(0);
  delay(950);
}
