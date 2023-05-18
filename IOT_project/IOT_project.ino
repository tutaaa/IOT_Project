#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          21         // Configurable, see typical pin layout above
#define green_LED       25
#define yellow_LED      26
#define red_LED         27
#define capbutton       4
#define capSignal       33
#define buzzer          32

#define ssid            "Tinn"
#define password        "tinntinn"

const char* host = "script.google.com";
const int httpsPort = 443;



WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID = "AKfycbxA177WxaEn_se8ibKsfRJyzwBMDwg8dILf69sRwcV6jniLtrUnyfrKAAqZa6eziOA4uA"; //--> spreadsheet script ID


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
unsigned long previousMillis = 0;
int millisState = LOW;
int buzzerflag = 0;
int state = 0;
int flag = 0;

uint64_t RFID = 0;
uint64_t LOGIN_RFID = 0;
int number = 1;
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);    // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println("Scan PICC to see UID, SAK, type, and data blocks...");

  WiFi.begin(ssid, password);
  //----------------------------------------Wait for connection
  Serial.print("Connecting ...........");
  while (WiFi.status() != WL_CONNECTED) {
    //----------------------------------------Make the On Board Flashing LED on the process of connecting to the wifi router.
    //----------------------------------------
  }
  //----------------------------------------
  //----------------------------------------If successfully connected to the wifi router, the IP Address that will be visited is displayed in the serial monitor
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------

  client.setInsecure();


  pinMode(2, OUTPUT);
  pinMode(green_LED, OUTPUT);
  pinMode(yellow_LED, OUTPUT);
  pinMode(red_LED, OUTPUT);
  pinMode(capbutton, INPUT);
  pinMode(capSignal, OUTPUT);
  pinMode(buzzer, OUTPUT);



  /*for (int i = 0; i < 3; i++) {
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);
    }*/

  digitalWrite(red_LED, HIGH);
}

void loop() {

  if (state == 0) { // state that wait for reading login student ID card
    SerialprintOnce("State0 : INIT , wait login student ID card");
    readRFID();
    buzzerAlert();
  }
  if (state == 1) { //state that wait for transmiting to https
    SerialprintOnce("State1 : wait for transmiting to google sheet");
    sendValue(RFID);
    state = 2;
  }
  if (state == 2) { // state that wait for capturing items from ESPcamera
    SerialprintOnce("State2 : ready for capuring items");
    blinkLED(yellow_LED);
    if ( digitalRead(capbutton) == 1) {
      digitalWrite(capSignal, HIGH);
      digitalWrite(yellow_LED, LOW);
      delay(500);
      state = 3;

    }

  }
  if (state == 3) { // state that wait for reading logout student ID card
    SerialprintOnce("State3 : capturing sucessful , wait for logout student ID card");
    digitalWrite(capSignal, LOW);
    readRFID();
    timeOut();
    buzzerAlert();
  }
  if (state == 4) { // state that reset state machine and then going to state 0
    Serial.println("State4 : logout sucessful , prepare for next student");
    digitalWrite(red_LED, HIGH);
    digitalWrite(green_LED, LOW);
    number ++;
    sendValue(0);
    state = 0;
    flag = 0;
  }
  if (state == 5) { //error handler
    Serial.println("ERROR Handler......press capture button to restart");
    blinkLED(yellow_LED);
    blinkLED(red_LED);
    blinkLED(green_LED);

    if ( digitalRead(capbutton) == 1) {
      ESP.restart();
    }
  }

}
void readRFID() {
  RFID = 0;
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
    Serial.print(mfrc522.uid.uidByte[i], DEC);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
    RFID += mfrc522.uid.uidByte[i], DEC;

  }
  Serial.println();
  Serial.println(RFID);

  Serial.print("Message : ");
  content.toUpperCase();
  if (state == 0) {
    correctRFID();
    LOGIN_RFID = RFID;
    state += 1 ;
  }
  else if (state == 3) {

    if (RFID == LOGIN_RFID) {

      correctRFID();
      state += 1 ;
    } else {
      Serial.println("incorrect RFID");
      digitalWrite(green_LED, LOW);
      delay(250);
      digitalWrite(green_LED, HIGH);
      delay(250);
    }
  }


}

void blinkLED(int LED_COLOR) {

  const long interval = 300;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (millisState == LOW) {
      millisState = HIGH;
    } else {
      millisState = LOW;
    }
    digitalWrite(LED_COLOR, millisState);
  }
}

void sendValue(uint64_t value) {

  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);

  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    ESP.restart();

  }
  uint64_t ID = value;
  String url = "/macros/s/" + GAS_ID + "/exec?temp=" + ID;
  // String url = "/macros/s/" + GAS_ID + "/exec?temp=" + ID + "&humi=" + number;


  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //---------------------------------------
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();


  //----------------------------------------
}
void correctRFID() {
  Serial.println("Authorized access");
  Serial.println();
  digitalWrite(2, HIGH);
  digitalWrite(green_LED, HIGH);
  digitalWrite(red_LED, LOW);
  delay(1500);
  digitalWrite(2, LOW);
}
void SerialprintOnce(String text) {
  if (flag == state) {
    Serial.println(text);
    flag = state + 1;
  }
}
void timeOut() {
  const long timeout = 60000;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= timeout) {
    previousMillis = currentMillis;

    state = 0;
    digitalWrite(buzzer, HIGH);
    delay(1500);
    digitalWrite(buzzer, LOW);
    digitalWrite(green_LED, LOW);
    digitalWrite(red_LED, HIGH);
  }
}

void buzzerAlert() { //

  if (buzzerflag == 0) {
    if ( digitalRead(capbutton) == 1) {

      for (int i = 0; i < 2; i++) {
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
        delay(100);
      }
      buzzerflag = 1;
    }
  } else if (buzzerflag == 1 &&  digitalRead(capbutton) == 0) {
    buzzerflag = 0;
  }
}
