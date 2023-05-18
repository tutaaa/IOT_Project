#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above
#define green_LED       25
#define yellow_LED      26
#define red_LED         27
#define capbutton       4

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
unsigned long previousMillis = 0;
int ledState = LOW;
int state = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);    // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  pinMode(2, OUTPUT);
  pinMode(green_LED, OUTPUT);
  pinMode(yellow_LED, OUTPUT);
  pinMode(red_LED, OUTPUT);
  pinMode(capbutton, INPUT);
  digitalWrite(red_LED, HIGH);
}

void loop() {

  if (state == 0) { // state that wait for reading login student ID card
    readRFID();
  }
  if (state == 1) { // state that wait for capturing items from ESPcamera
    blinkLED();
    if ( digitalRead(capbutton) == 1) {
      state = 2;

    }

  }
  if (state == 2) { // state that wait for reading logout student ID card
    readRFID();
  }
  if (state == 3) { // state that reset state machine and then going to state 0
    digitalWrite(red_LED, HIGH);
    digitalWrite(green_LED, LOW);
    state = 0;
  }

}
void readRFID() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  //  if (content.substring(1) == "E1 74 4C 32")
  //  {
  Serial.println("Authorized access");
  Serial.println();
  delay(500);
  digitalWrite(2, HIGH);


  delay(2500);

  digitalWrite(green_LED, HIGH);
  digitalWrite(2, LOW);
  digitalWrite(red_LED, LOW);
  state += 1 ;
  //}
  //  else   {
  //    Serial.println(" Access denied");
  //    delay(500);
  //  }

}

void blinkLED() {
  const long interval = 300;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(yellow_LED, ledState);
  }
}
