/**
 * Write a light script to the BlinkM at a specific I2C device id.
 *
 * TODO ask for device address via serial monitor
 *
 * Motivated by some of todbot's code.
 *
 * @author Bill <bill@jamimi.com>
 */

#include <JeeLib.h>

// The light script array is named "script_lines"
// Change the following to get the correct light script
// #include "light_script_ss.h"
#include "light_script_sr.h"

// I2C device id for MaxM being programmed
const byte maxmDev = 12;

// MaxMs on port 2
PortI2C MaxMbus (2);
DeviceI2C maxm (MaxMbus, maxmDev);

// TODO actually turn on LED someday :)
// LEDs on port 4
Port ledPort (4);

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
  maxm.send();
  maxm.write('W');
  maxm.write(0);
  maxm.write(i);
  maxm.write(dur);
  maxm.write(cmd);
  maxm.write(arg1);
  maxm.write(arg2);
  maxm.write(arg3);
  maxm.stop();
}

void setLength(byte len, byte reps) {
  Serial.print("Writing script length/reps: ");
  Serial.print(len);
  Serial.print(", ");
  Serial.print(reps);
  Serial.println();
  maxm.send();
  maxm.write('L');
  maxm.write(0);
  maxm.write(len);
  maxm.write(reps);
  maxm.stop();
}

void setup () {
  Serial.begin(57600);
  Serial.print("\nProgram MaxM on port ");
  Serial.print(2);
  Serial.print(" at I2C device id ");
  Serial.println(maxmDev);

  ledPort.mode(OUTPUT);

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


