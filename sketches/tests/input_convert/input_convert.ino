/**
 * Test sketch to see how input characters are converted to integer
 */

// Variables for reading input
// The input string
char inputData[20];
// The just-read input character
char inputChar = -1;
// Index of where to store the next character
byte index = 0;

void setup() {
  Serial.begin(57600);
  Serial.println("\nTest output converstion");


  char c1 = 'A';
  int i1 = int(c1);
  Serial.print("Input: ");
  Serial.print(c1);
  Serial.print(" Int: ");
  Serial.println(i1);

  char c2 = '1';
  int i2 = int(c2);
  Serial.print("Input: ");
  Serial.print(c2);
  Serial.print(" Int: ");
  Serial.println(i2);

  char c3 = '9';
  int i3 = int(c3);
  Serial.print("Input: ");
  Serial.print(c3);
  Serial.print(" Int: ");
  Serial.println(i3);

  char c4 = '5';
  int i4 = int(c4);
  Serial.print("Input: ");
  Serial.print(c4);
  Serial.print(" Int: ");
  Serial.println(i4);

  int val = 0;
  val = (val * 10) + (i2 - 48);
  val = (val * 10) + (i3 - 48);
  val = (val * 10) + (i4 - 48);
  Serial.print("Converted: ");
  Serial.println(val);
  Serial.println(val, HEX);
}

void loop() {
}
