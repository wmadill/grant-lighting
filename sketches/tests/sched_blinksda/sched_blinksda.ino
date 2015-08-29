/**
 * Use the schedule class to have two different LEDs on one JeeNode
 * port flash at different rates. One LED is on DIO and the other
 * AIO. 
 *
 * This is a straight-forward modification of the sched_blinks.ino
 * sketch.
 *
 * Based largely on jcw's "schedule" and "blink_ports" sketches 
 *
 * Changes:
 * - remove BlinkPlug specifics but use the same techniques. The
 *   LEDs are wired between DIO/AIO and GND rather than VCC and
 *   DIO/AIO as in the BlinkPlug code.
 *
 * Original "blink_ports" sketch:
 * 2009-02-13 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Original "schedule" sketch:
 * 2010-10-18 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
 * Modifications
 * 2015-08-14 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#include <JeeLib.h>

Port one (4);

// Connect a series resistor (470 or 1k ohm) to two leds. Connect one
// to pins 2 (DIO) and 3 (GND) on Jeenode port 4 and the other to pins
// 5 (AIO) and 3 (GND).

enum { TASK1, TASK2, TASK_LIMIT };

static word schedBuf[TASK_LIMIT];
Scheduler scheduler (schedBuf, TASK_LIMIT);

byte led1, led2;

// this has to be added since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    Serial.begin(57600);
    Serial.println("\n[schedule]");
    Serial.flush();
    
    // turn the radio off completely
    rf12_initialize(17, RF12_868MHZ);
    rf12_sleep(RF12_SLEEP);

    one.mode(OUTPUT);
    one.mode2(OUTPUT);

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
              scheduler.timer(TASK1, 8);
            }
            break;
        // LED 2 blinks .5 second every second
        case TASK2:
            led2 = !led2;
            if (led2) {
              one.digiWrite2(1);
              scheduler.timer(TASK2, 1);
            } else {
              one.digiWrite2(0);
              scheduler.timer(TASK2, 3);
            }
            break;
    }
}

