/*
 * Control a BlinkM on a specific Jeenode port using BlinkM commands.
 * See the comments at the top of the i2c_scan sketch for pin
 * assignments. This only works for the first device on the bus.
 *
 * Notes:
 * 1. Set the Jeenode port number running the I2C bus that has the
 *    BlinkM on it.
 * 2. Set the BlinkM to be powered externally.
 *
 * This sketch motivated by, and parts taken from, todbot's BlinkMTester
 * sketch. The current version is at
 * github.com/todbot/BlinkM-Arduino/BlinkM/examples/BlinkMTester
 * although an earlier version was used in this sketch.
 *
 * A major refactoring would be useful but postponed until there
 * is more time.
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
byte foundDevId = 0;
char cmd = '\0';

// Variables for reading input
// The input string
char inputData[20];
// The just-read input character
char inputChar = -1;
// Index of where to store the next character
byte index = 0;

byte addr = 0;

// Various named parameters used below
//TODO there must be a better way...
int scriptNum = 0;
int numRepeats = 0;
int startLine = 0;
int scriptMode = 0;
int fadeSpeed = 0;
int timeAdj = 0;
byte cRed = 0;
byte cGreen = 0;
byte cBlue = 0;

/*
const char helpstr[] PROGMEM = 
  "\nBlinkMTester!\n"
  "'c<rrggbb>'  fade to an rgb color\n"
  "'h<hhssbb>'  fade to an hsb color\n"
  "'C<rrggbb>'  fade to a random rgb color\n"
  "'H<hhssbb>'  fade to a random hsb color\n"
  "'p<n>'  play a script\n"
  "'o'  stop a script\n"
  "'f<nn>'  change fade speed\n"
  "'t<nn>'  set time adj\n"
  "'g'  get current color\n"
  "'a'  get I2C address\n"
  "'A<n>'  set I2C address\n"
  "'B'  set startup params to default\n"
  "'Z'  get BlinkM version\n"
  "'i'  get input values\n"
  "'s'/'S'  scan i2c bus for 1st BlinkM / search for devices\n"
  "'R'  return BlinkM to factory settings\n"
  "'?'  for this help msg\n\n"
  ;
void printProgStr(const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    Serial.write(c);
}

void help()
{
  printProgStr( helpstr );
}
*/

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

  Serial.print("Enter command: ");
}

void loop () {
  // Build command string
  while (Serial.available() > 0) {
    if (index < 19) {
      inputChar = Serial.read();
      inputData[index] = inputChar;
      index++;
      inputData[index] = '\0';
    } else {
      inputData[index] = '\0';
      char ignore = Serial.read();
    }
  }

  // Process command
  if (inputData[0] != '\0') {
    // Echo the entered command
    Serial.println(inputData);

    // The first character is the command
    cmd = inputData[0];
    int val = 0;
    int parms[5];
    byte parmIdx = 0;

    // Set parms to -1 for "not set"
    for (byte i = 0; i <= 4; i++) {
      parms[i] = -1;
    }

    // 0 if val is null, 1 if has value
    byte valSet = 0;

    // Parse the rest of the string which is decimal numbers 
    // separated by commas
    // TODO need to process leading "-"
    for (byte i = 1; i <= 19; i++) {
      // Done if no more characters
      if (inputData[i] == '\0') break;
      // Process parm
      // Commas delimit parms
      if (inputData[i] == ',') {
        if (parmIdx > 4) break;
        parms[parmIdx] = val;
        parmIdx++;
        val = 0;
        valSet = 0;
        continue;
      }
      // Skip blanks
      if (inputData[i] == ' ') continue;

      // Stop processing if non-digit
      if ((inputData[i] < '0') || (inputData[i] > '9')) break;
      valSet = 1;
      val = (val * 10) + (inputData[i] - 48);
    }   // end for
    
    // Get any last parm
    if ((parmIdx <= 4) && (valSet ==1 )) {
      parms[parmIdx] = val;
      parmIdx++;
    }

    /*
    Serial.print("Parms: ");
    for (byte j = 0; j <= parmIdx - 1; j++) {
      Serial.print(j);
      Serial.print('=');
      if (parms[j] == -1) {
        Serial.println("Not set");
        break;
      }
      Serial.print(parms[j], DEC);
      Serial.print(", ");
    }
    Serial.println("done");
    */

    switch (cmd) {

      // Print help
      case '?':
        Serial.println("Help is not implemented");
        break;

      // Set new device id
      case 'A':
        // new device id in parm[0]
        if ((parms[0] < 1) || (parms[0] > 127)) {
          Serial.println("Address is out of range; ignoring");
        } else {
          addr = byte(parms[0]);
          Serial.print("Setting new address to ");
          Serial.print(addr, DEC);
          Serial.print(" (");
          Serial.print(addr, HEX);
          Serial.println(") hex");

          dev.send();
          dev.write('A');
          dev.write(addr);
          dev.write(0xd0);
          dev.write(0x0d);
          dev.write(addr);
          dev.stop();
          dev.setAddress(addr);
          delay(15);        // Datasheet says controller needs 15ms
        }
        break;

      // Get the current device address
      case 'a':
        dev.send();
        dev.write('a');
        dev.stop();
        dev.receive();
        addr = dev.read(1);
        dev.stop();
        Serial.println("Get the current device address");
        Serial.println(addr);
        break;
  
      // Set the startup action
      // 5 parameters: play script or not, scriptnum, repeats, fade, time
      case 'B':
        scriptMode = parms[0];
        scriptNum = parms[1];
        numRepeats = parms[2];
        fadeSpeed = parms[3];
        // TODO need to handle negative numbers
        timeAdj = parms[4];

        if ((scriptMode != 0) && (scriptMode !=1)) {
          Serial.println("Script start mode is out of range; setting to 1 (play a script)");
          scriptMode = 1;
        }
        if ((scriptNum < 0) || (scriptNum > 18)) {
          Serial.println("Light script id is out of range; setting to script 0");
          scriptNum = 0;
        }
        if (numRepeats < 0) {
          Serial.println("Number of repeats not set; setting to 0 (run forever)");
          numRepeats = 0;
        }
        if ((fadeSpeed < 1) || (fadeSpeed > 255)) {
          Serial.println("Fade speed out of range; setting to 128");
          fadeSpeed = 128;
        }
        if ((timeAdj < -128) || (timeAdj > 127)) {
          Serial.println("Time adjustment is out of range; setting to 0 (default)");
          timeAdj = 0;
        }

        Serial.print("Set startup parameters to ");
        if (scriptMode == 1) {
          Serial.print("play a script, ");
        } else {
          Serial.print("do nothing, ");
        }
        Serial.print("script id ");
        Serial.print(scriptNum);
        Serial.print(" with ");
        Serial.print(numRepeats);
        Serial.print(" repeats, fade speed ");
        Serial.print(fadeSpeed);
        Serial.print(", time adjustment ");
        Serial.println(timeAdj);

        dev.send();
        dev.write('B');   // Set startup parameters
        dev.write(scriptMode);
        dev.write(scriptNum);  
        dev.write(numRepeats); 
        dev.write(fadeSpeed); 
        dev.write(timeAdj);   
        dev.stop();
        delay(50);        // Data sheet says controller needs 20ms
        break;

      // Fade to HSB color
      // 3 parameters: R, G, B values
      //TODO implement
      case 'h':
        Serial.println("Fade to HSB color is not implemented");
        break;

      // Fade to random HSB color
      // 3 parameters: R, G, B values
      //TODO implement
      case 'H':
        Serial.println("Fade to random HSB color is not implemented");
        break;


      // Fade to RGB color
      // 3 parameters: R, G, B values
      //TODO implement
      case 'c':
        Serial.println("Fade to RGB color is not implemented");
        break;

      // Fade to random RGB color
      // 3 parameters: R, G, B values
      //TODO implement
      case 'C':
        Serial.println("Fade to random RGB color is not implemented");
        break;

      // Go to RGB color now
      // 3 parameters: R, G, B values
      //TODO implement
      case 'n':
        Serial.println("Go to RGB color now is not implemented");
        break;

      // Write script line
      // 7 parameters: script num, line num (0-49), duration (ticks),
      //  blinkm command followed by its 3 parameters
      // Allow 20ms after writing
      // Follow writing of script lines with an "L" command
      case 'W':
        Serial.println("Write script line is not implemented");
        break;

      // Read script line
      // 2 parameters: script id, line num
      // returns 5 values: duration (ticks), command followed by its 3
      //                   parameters
      case 'R':
        Serial.println("Read script line is not implemented");
        break;

      // Set script length and repeats
      // 3 parameters: script id, length, number of repeats
      // 15ms complete time
      case 'L':
        Serial.println("Set script length is not implemented");
        break;

      // Get Current RGB Color
      //TODO implement
      case 'g':
        Serial.print("Current RGB Color is ");
        dev.send();
        dev.write('g');
        dev.stop();
        dev.receive();
        cRed = dev.read(1);
        cGreen = dev.read(1);
        cBlue = dev.read(1);
        dev.stop();
        Serial.print(cRed, HEX);
        Serial.print(cGreen, HEX);
        Serial.print(cBlue, HEX);
        Serial.println();
        break;

      // Set fade speed
      // 1 parameter: fade speed
      case 'f':
        fadeSpeed = parms[0];
        if ((fadeSpeed < 1) || (fadeSpeed > 255)) {
          Serial.println("Fade speed out of range; setting to 128");
          fadeSpeed = 128;
        }
        Serial.print("Set fade speed to ");
        Serial.println(fadeSpeed);
        dev.send();
        dev.write('f');
        dev.write(fadeSpeed);
        dev.stop();
        break;

      // Set time adjustment; smaller numbers are faster; added to
      // all durations of scripts
      // 1 parameter: time adjustment
      // TODO need to handle negative numbers
      case 't':
        timeAdj = parms[0];
        if ((timeAdj < -128) || (timeAdj > 127)) {
          Serial.println("Time adjustment is out of range; setting to 0 (default)");
          timeAdj = 0;
        }
        Serial.print("Set time adjustment to ");
        Serial.println(timeAdj);
        dev.send();
        dev.write('t');
        dev.write(timeAdj);
        dev.stop();
        break;

      // Stop light script
      case 'o':
        dev.send();
        dev.write('o');
        dev.stop();
        break;

      // Play light script
      // 3 parameters 
      // script num (1=18)
      // number of repeats (0 = forever)
      // line number to start with (0 = first)
      case 'p':
        scriptNum = parms[0];
        numRepeats = parms[1];
        startLine = parms[2];

        if ((scriptNum < 0) || (scriptNum > 18)) {
          Serial.println("Light script id is out of range; setting to script 0");
          scriptNum = 0;
        }
        if (numRepeats < 0) {
          Serial.println("Number of repeats not set; setting to 0 (run forever)");
          numRepeats = 0;
        }
        if (startLine < 0) {
          Serial.println("Script starting line not set; setting to 0 (run from beginning)");
          startLine = 0;
        }
        Serial.print("Play light script ");
        Serial.print(scriptNum);
        Serial.print(" , repeat ");
        Serial.print(numRepeats);
        Serial.print(" times, starting at script line ");
        Serial.println(startLine);

        // Unclear if this is useful
        // Turn off 
        dev.send();
        dev.write('n');
        dev.write(0);
        dev.write(0);
        dev.write(0);
        dev.stop();

        dev.send();
        dev.write('p');
        dev.write(byte(scriptNum));
        dev.write(byte(numRepeats));
        dev.write(byte(startLine));
        dev.stop();
        break;

      // Get BlinkM firmware version
      // returns major firmware version and minor firmware version
      case 'Z':
        Serial.println("Get BlinkM firmware version is not implemented");
        break;

      // Talk on a new I2C address
      // 1 parameter I2C device id
      // Convenience routine swiped from todbot
      case '@':
        Serial.println("Talk on BlinkM address not set");
        break;

      // Scan for first I2C device on bus
      // Conventience routine that just runs the starup scan
      // Idea swiped from todbot
      case 's':
        Serial.println("Scan for first I2C device not implemented");
        break;

      // Scan I2C bus for all devices
      //TODO get code from i2c_dev_scan
      // Idea swiped from todbot
      case 'S':
        Serial.println("Scan I2C bus not implemented");
        break;

      // Factory reset the BlinkM
      // Idea swiped from todbot but command collision
      // since "R" used for reading a script line
      // A Factory reset involves:
      // 1. set address to 0x09
      // 2. delay(30)
      // 3. set startup parms (1,0,0,8,0)
      // 4. delay(30)
      // 5. write this script in todbot notation:
      //   blinkm_script_line script1_lines[] = {
      //     {  1,  {'f',   10,  00,  00}},  // set fade speed to 10
      //     { 100, {'c', 0xff,0xff,0xff}},  // white
      //     {  50, {'c', 0xff,0x00,0x00}},  // red
      //     {  50, {'c', 0x00,0xff,0x00}},  // green
      //     {  50, {'c', 0x00,0x00,0xff}},  // blue 
      //     {  50, {'c', 0x00,0x00,0x00}},  // off
      //   };
      //   int script1_len = 6;  // number of script lines above
      //   BlinkM_writeScript( 0x09, 0, script1_len, 0, script1_lines);

      case 'X':
        Serial.println("Factory reset not implemented");
        break;
          
      default:
        Serial.println("Unknown command.");
    }   // end switch

    inputData[0] = '\0';
    index = 0;
    Serial.print("Enter command: ");
  }   // end processing command

  // Blink the LED every 5 seconds
  led.digiWrite(1);
  delay(50);
  led.digiWrite(0);
  delay(4950);
}
