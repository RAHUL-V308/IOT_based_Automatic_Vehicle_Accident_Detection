#include <TinyGPS++.h>
#include <SoftwareSerial.h>

const int x_out = 34; /* connect x_out of module to A1 of UNO board */
const int y_out = 35; /* connect y_out of module to A2 of UNO board */
const int z_out = 32; /* connect z_out of module to A3 of UNO board */

SoftwareSerial sim800(16, 17); 

static const int RXPin = 18, TXPin = 5;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

char presentLink[100];

void setup(){
  Serial.begin(9600);
  ss.begin(9600);

  sim800.begin(38400); 
  Serial.println("Initializing...GSM");
  delay(1000);
  sim800.println("AT");                 // Sends an ATTENTION command, reply should be OK
  sim800.println("AT+CMGF=1");          // Configuration for sending SMS
  sim800.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  Serial.println("Initializing...Completed");
}

void loop() {
  int x_adc_value, y_adc_value, z_adc_value;  

  x_adc_value = analogRead(x_out); /* Digital value of voltage on x_out pin */ 
  y_adc_value = analogRead(y_out); /* Digital value of voltage on y_out pin */ 
  z_adc_value = analogRead(z_out); /* Digital value of voltage on z_out pin */ 
  Serial.print("x = ");
  Serial.print(x_adc_value);
  Serial.print("\t\t");
  Serial.print("y = ");
  Serial.print(y_adc_value);
  Serial.print("\t\t");
  Serial.print("z = ");
  Serial.print(z_adc_value);
  Serial.println("");

  // Call the one() function if any value is greater than 200
  if (x_adc_value > 200 || y_adc_value > 200 || z_adc_value > 200) {
    accident();
  }
 
  /* Wait a second */
  delay(1000);
}

void accident() {
  getLocation(); 
  delay(3000);
  sendSMS(); 
  delay(10000);  
}
void sendSMS(){
  Serial.println("Sening sms");
  sim800.println("AT+CMGF=1");
  delay(500);
  sim800.println("AT+CMGS=\"+916379393126\"\r");
  delay(500);
  sim800.println("Accident detected at .... !     ");
  sim800.print(presentLink);
  delay(500);
  sim800.write(26);
}



void getLocation() {
  while (ss.available() > 0) {
    gps.encode(ss.read());
    /*if (gps.location.isUpdated()) {
      double latitude = gps.location.lat();
      double longitude = gps.location.lng();*/ //11.362558549663612, 77.8279939072903 
      double latitude = 11.362558549663612;
      double longitude = 77.8279939072903;

      // Create the link using a character array
      char link[100];
      snprintf(link, sizeof(link), "http://maps.google.com/maps?q=%.6f,%.6f", latitude, longitude);
      strncpy(presentLink, link, sizeof(presentLink));

      Serial.print("Latitude= ");
      Serial.print(latitude, 6);
      Serial.print(" Longitude= ");
      Serial.println(longitude, 6);

      // Print the link to the Serial Monitor
      Serial.print("Link: ");
      Serial.println(link);
      break;
    //}
  }
}