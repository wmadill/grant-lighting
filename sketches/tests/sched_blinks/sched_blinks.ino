/**
 * Use the schedule class to have two different LEDs on different
 * JeeNode ports flash at different rates.
 *
 * Based largely on jcw's "schedule" and "blink_ports" sketches 
 *
 * Changes:
 * - remove BlinkPlug specifics and replace with
 *   two Port instances
 *
 * Original "blink_ports" sketch:
 * 2009-02-13 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Original "schedule" sketch:
 * 2010-10-18 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Modifications
 * 2015-08-14 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>

Port one (1);
Port two (2);

// Connect a series resistor (470 or 1k ohm) to two leds. Connect one
// to pins 2 (DIO) and 3 (GND) on Jeenode port 1 and the other to the
// same pins on port 2.

enum { TASK1, TASK2, TASK_LIMIT };

static word schedBuf[TASK_LIMIT];
Scheduler scheduler (schedBuf, TASK_LIMIT);

byte led1, led2;

// this has to be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    Serial.begin(57600);
    Serial.println("\n[schedule]");
    
    // turn the radio off completely
    rf12_initialize(17, RF12_868MHZ);
    rf12_sleep(RF12_SLEEP);

    one.mode(OUTPUT);
    two.mode(OUTPUT);

    led1 = 0;
    led2 = 0;
    
    // start both tasks 1.5 seconds from now
    scheduler.timer(TASK1, 15);
    scheduler.timer(TASK2, 15);
}

void loop () {
    switch (scheduler.pollWaiting()) {
        // LED 1 blinks .1 second every second
        case TASK1:
            led1 = !led1;
            if (led1) {
              one.digiWrite(1);
              scheduler.timer(TASK1, 1);
            } else {
              one.digiWrite(0);
              scheduler.timer(TASK1, 9);
            }
            break;
        // LED 2 blinks .5 second every second
        case TASK2:
            led2 = !led2;
            if (led2) {
              two.digiWrite(1);
              scheduler.timer(TASK2, 5);
            } else {
              two.digiWrite(0);
              scheduler.timer(TASK2, 5);
            }
            break;
    }
}
