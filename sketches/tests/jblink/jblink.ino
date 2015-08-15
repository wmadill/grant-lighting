/**
 * Blink an LED on the DIO pin of JeeNode header 4. 
 * This uses standard Arduino pin assignments with
 * no power management.
 *
 * 2015-08-14 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

#define LED_PIN 7

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop () {
  digitalWrite(LED_PIN, 1);
  delay(50);
  digitalWrite(LED_PIN, 0);
  delay(10000);
}
