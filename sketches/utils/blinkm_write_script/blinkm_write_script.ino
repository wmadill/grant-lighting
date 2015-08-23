/**
 * Write a light script to the BlinkM with the lowest I2C device id
 * on a Jeenode port.
 *
 * Motivated by some of todbot's code.
 *
 * @author Bill <bill@jamimi.com>
 */

#include <JeeLib.h>

// The light script array is named "script_lines"
// Change the following to get the correct light script
#include "light_script_ss.h"

// Jeenode port used as I2C bus
const byte portNum = 2;

// The I2C bus and device objects
PortI2C i2cBus (portNum);
// Device address is set during scan
DeviceI2C dev (i2cBus, 0);

// Blink an LED on Jeenode port 3 to show the sketch is active
Port led (3);

const byte loDevId = 1;
const byte hiDevId = 127;
byte foundDevId = 0;
char cmd = '\0';

void writeScript(byte len, byte reps, blinkm_script_line* lines) {
  for (byte i = 0; i < len; i++) {
    blinkm_script_line l = lines[i];
    writeLine(i, l.dur, l.cmd[0], l.cmd[1], l.cmd[2], l.cmd[3]);
    delay(20);
  }
  setLength(len, reps);
}

void writeLine(byte i, byte dur, byte cmd, byte arg1, byte arg2, byte arg3) {
  Serial.print("Writing script line ");
  Serial.print(i);
  Serial.print(": dur=");
  Serial.print(dur);
  Serial.print(", cmd=");
  Serial.print(char(cmd));
  Serial.print(", args=");
  Serial.print(arg1, HEX);
  Serial.print(", ");
  Serial.print(arg2, HEX);
  Serial.print(", ");
  Serial.print(arg3, HEX);
  Serial.println();
  dev.send();
  dev.write('W');
  dev.write(0);
  dev.write(i);
  dev.write(dur);
  dev.write(cmd);
  dev.write(arg1);
  dev.write(arg2);
  dev.write(arg3);
  dev.stop();
}

void setLength(byte len, byte reps) {
  Serial.print("Writing script length/reps: ");
  Serial.print(len);
  Serial.print(", ");
  Serial.print(reps);
  Serial.println();
  dev.send();
  dev.write('L');
  dev.write(0);
  dev.write(len);
  dev.write(reps);
  dev.stop();
}

void setup () {
  Serial.begin(57600);
  Serial.print("\nScan I2C bus on port ");
  Serial.println(portNum);

  led.mode(OUTPUT);

  Serial.print("Trying ID ");

  // Scan for devices and stop at first one
  for (byte devid = loDevId; devid <= hiDevId; devid++) {
    Serial.print(devid, DEC);
    Serial.print("...");
    if (!devid % 20) Serial.println();
    dev.setAddress(devid);
    if (!dev.isPresent()) continue;
    foundDevId = devid;
    break;
  }

  Serial.println();
  if (foundDevId == 0) {
    Serial.println("No device found");
  } else {
    Serial.print("Found device at ID ");
    Serial.println(foundDevId);
  }

  Serial.print("Enter \"W\" to write light script: ");
}

void loop () {
  // Build command string
  while (Serial.available() > 0) {
    cmd = Serial.read();
  }
  
  int n = 0;

  // Process command
  if (cmd != '\0') {
    // Echo the entered command
    Serial.println(cmd);

    switch (cmd) {

      // Write script
      case 'W':
        Serial.println("Writing light script");
        Serial.print("Lines in light script: ");
        n = sizeof(script_lines) / sizeof(script_lines[0]);
        Serial.println(n);
        writeScript(n, 0, script_lines);
        break;

      default:
        Serial.println("Invalid command");
        break;
    }   // end switch
    cmd = '\0';
  }   // end processing command
}   // end loop

