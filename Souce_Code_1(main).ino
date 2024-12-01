#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

const String EMERGENCY_PHONE = "+919360445274";
#define rxPin 16
#define txPin 17
#define BUZZER 13
#define BUTTON 12
#define xPin 36
#define yPin 39
#define zPin 34

HardwareSerial sim800(1);
HardwareSerial neogps(2);
TinyGPSPlus gps;

String sms_status, sender_number, received_date, msg;
String latitude, longitude;

byte updateflag;

int xaxis = 0, yaxis = 0, zaxis = 0;
int deltx = 0, delty = 0, deltz = 0;
int vibration = 2, devibrate = 75;
int magnitude = 0;
int sensitivity = 20;
double angle;
boolean impact_detected = false;
unsigned long time1;
unsigned long impact_time;
unsigned long alert_delay = 30000;

void setup() {
  Serial.begin(9600);

  sim800.begin(9600, SERIAL_8N1, rxPin, txPin);
  neogps.begin(9600, SERIAL_8N1, 12, 13);

  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  sms_status = "";
  sender_number = "";
  received_date = "";
  msg = "";

  sim800.println("AT");
  delay(1000);
  sim800.println("ATE1");
  delay(1000);
  sim800.println("AT+CPIN?");
  delay(10000);
  sim800.println("AT+CMGF=1");
  delay(10000);
  sim800.println("AT+CNMI=1,1,0,0,0");
  delay(10000);
  time1 = micros();
  xaxis = analogRead(xPin);
  yaxis = analogRead(yPin);
  zaxis = analogRead(zPin);
}

void loop() {
  if (micros() - time1 > 1999) Impact();

  if (updateflag > 0) {
    updateflag = 0;
    Serial.println("Impact detected!!");
    Serial.print("Magnitude:");
    Serial.println(magnitude);

    getGps();
    digitalWrite(BUZZER, HIGH);
    impact_detected = true;
    impact_time = millis();
  }

  if (impact_detected == true) {
    if (millis() - impact_time >= alert_delay) {
      digitalWrite(BUZZER, LOW);
      makeCall();
      delay(10000);
      sendAlert();
      impact_detected = false;
      impact_time = 0;
    }
  }

  if (digitalRead(BUTTON) == LOW) {
    delay(10000);
    digitalWrite(BUZZER, LOW);
    impact_detected = false;
    impact_time = 0;
  }
  while (sim800.available()) {
    parseData(sim800.readString());
  }
  while (Serial.available()) {
    sim800.println(Serial.readString()); 
  }
}

void Impact() {
  time1 = micros();
  int oldx = xaxis;
  int oldy = yaxis;
  int oldz = zaxis;

  xaxis = analogRead(xPin);
  yaxis = analogRead(yPin);
  zaxis = analogRead(zPin);

  vibration--;
  if (vibration < 0) vibration = 0;

  if (vibration > 0) return;
  deltx = xaxis - oldx;
  delty = yaxis - oldy;
  deltz = zaxis - oldz;

  magnitude = sqrt(sq(deltx) + sq(delty) + sq(deltz));
  if (magnitude >= sensitivity) {
    updateflag = 1;
    vibration = devibrate;
  } else {
    if (magnitude > 15)
      Serial.println(magnitude);
    magnitude = 0;
  }
}

void parseData(String buff) {
  Serial.println(buff);

  unsigned int len, index;
  index = buff.indexOf("\r");
  buff.remove(0, index + 2);
  buff.trim();

  if (buff != "OK") {

    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();

    buff.remove(0, index + 2);

    if (cmd == "+CMTI") {

      index = buff.indexOf(",");
      String temp = buff.substring(index + 1, buff.length());
      temp = "AT+CMGR=" + temp + "\r";
      sim800.println(temp);
    }

    else if (cmd == "+CMGR") {

      if (buff.indexOf(EMERGENCY_PHONE) > 1) {
        buff.toLowerCase();
        if (buff.indexOf("get gps") > 1) {
          getGps();
          String sms_data;
          sms_data = "GPS Location Data\r";
          sms_data += "http://maps.google.com/maps?q=loc:";
          sms_data += latitude + "," + longitude;

          sendSms(sms_data);
        }
      }
    }

  } else {
  }
}

void getGps() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  float flat, flon;
  unsigned long age;

  gps.f_get_position(&flat, &flon, &age);
  latitude = String(flat, 6);
  longitude = String(flon, 6);

  if ((age == TinyGPSPlus::GPS_INVALID_AGE) || (age > 5000)) {
    Serial.println("No GPS fix available");
    latitude = "";
    longitude = "";
  } else {
    Serial.print("Latitude= ");
    Serial.println(latitude);
    Serial.print("Longitude= ");
    Serial.println(longitude);
  }
}
void sendAlert() {
  String sms_data;
  sms_data = "Accident Alert!!\r";
  sms_data += "http://maps.google.com/maps?q=loc:";
  sms_data += latitude + "," + longitude;

  sendSms(sms_data);
}

void makeCall() {
  Serial.println("calling....");
  sim800.println("ATD" + EMERGENCY_PHONE + ";");
  delay(10000);
  sim800.println("ATH");
  delay(10000);
}

void sendSms(String text) {
  sim800.print("AT+CMGF=1\r");
  delay(10000);
  sim800.print("AT+CMGS=\"" + EMERGENCY_PHONE + "\"\r");
  delay(10000);
  sim800.print(text);
  delay(10000);
  sim800.write(0x1A);
  delay(10000);
  Serial.println("SMS Sent Successfully.");
}

boolean SendAT(String at_command, String expected_answer, unsigned int timeout) {
  uint8_t x = 0;
  boolean answer = 0;
  String response;
  unsigned long previous;

  while (sim800.available() > 0) sim800.read();

  sim800.println(at_command);

  x = 0;
  previous = millis();

  do {
    if (sim800.available() != 0) {
      response += sim800.read();
      x++;
      if (response.indexOf(expected_answer) > 0) {
        answer = 1;
        break;
      }
    }
  } while ((answer == 0) && ((millis() - previous) < timeout));

  Serial.println(response);
  return answer;
}