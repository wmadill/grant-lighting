/**
 * The usual "Hello World" to show the JeeNode
 * can at least talk out the serial interface.
 *
 * 2015-08-14 <bill@jamimi.com> http://opensource.org/licenses/mit-license.php
 */

void setup () {
  Serial.begin(57600);
}

void loop () {
  Serial.println("Hello, world");
  delay(1000);
}
