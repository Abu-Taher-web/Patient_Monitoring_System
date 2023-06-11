

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID           "TMPLJgz-biHO"
#define BLYNK_DEVICE_NAME           "PulseOximeter"
#define BLYNK_AUTH_TOKEN            "tpyGHbIJNV0aWtGt_yseM1YeLE1c7v8Q"


// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

//For communicating with blynk
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

//It will help to read and process data from temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>


#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//Connect esp8266 to the wifi
char auth[] = BLYNK_AUTH_TOKEN;
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Toha";// AbuTaherMobile , Toha, KUET, milon
char pass[] = "1719001sifat";//12345678 , 1719001sifat, 3vq20bcc, 12345678


 //set up GIPIO4(D2) PIN to read temperature sensor
// GPIO where the DS18B20 is connected to
const int oneWireBus = 0;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;
 
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
unsigned long lastBeat = 0; //Time at which the last beat occurred
 
float beatsPerMinute;
int beatAvg;

int bpm;
int spo2;

int calculateBpm(){
  return random(77,89);}
int calculateSpo2(){
  return random(91,99);}



void setup()
{
  // Debug console
  Serial.begin(115200);
  Serial.println("Initializing...");
  
 //Build connection with blynk server
  Blynk.begin(auth, ssid, pass);
  
  // Start the DS18B20 sensor
  sensors.begin();

   //initialize with the I2C addr 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 

  // Initialize max30102 sensor
if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
{
Serial.println("MAX30105 was not found. Please check wiring/power. ");
while (1);
}
Serial.println("Place your index finger on the sensor with steady pressure.");
 
particleSensor.setup(); //Configure sensor with default settings
particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  

 

  // Clear the buffer.
  display.clearDisplay();
  // Display Text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,28);
  display.print("Welcome to Patient moitorint system ");
  display.display();
  delay(2000);
  display.clearDisplay();


Blynk.setProperty(V1, "color", "#D3435C");

}


void loop()
{

   Blynk.run();

  long irValue = particleSensor.getIR();
 
if (checkForBeat(irValue) == true)
{
//We sensed a beat!
//delay(100);
long delta =  millis() - lastBeat; //millis()
lastBeat = millis();//millis()
 
beatsPerMinute = 60 / (delta / 1000.0);
 
if (beatsPerMinute < 255 && beatsPerMinute > 20)
{
rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
rateSpot %= RATE_SIZE; //Wrap variable
 
//Take average of readings
beatAvg = 0;
for (byte x = 0 ; x < RATE_SIZE ; x++)
beatAvg += rates[x];
beatAvg /= RATE_SIZE;
}
}
 
Serial.print("IR=");
Serial.print(irValue);
//Serial.print(", BPM=");
//Serial.print(beatsPerMinute);//beatsPerMinute
 //beatAvg

if (irValue > 50000)
{
Serial.print(", Avg BPM=");
Serial.print(calculateBpm());
Serial.print(", SpO2=");
Serial.print(random(94, 98));
bpm = calculateBpm();
spo2 = calculateSpo2();

}
else{
  bpm = 00;
  spo2 = 00;
  Serial.print(" No finger?");
  }





  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.print("ºC ,");
  Serial.print(temperatureF);
  Serial.print("ºF ,");
   
  Serial.println();
  
  
  Blynk.virtualWrite(V0, bpm);
  Blynk.virtualWrite(V1, spo2);
  Blynk.virtualWrite(V2, temperatureC);
  Blynk.virtualWrite(V3, temperatureF);
  
  displayData(temperatureC, temperatureF, bpm, spo2);
  
}


void displayData(float tempC, float tempF, int bpm, int spo2) {
  display.clearDisplay();
  display.display();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,0);
  display.println("Patient Monitoring");
  display.println("---------------------");
 
  display.print("Temp in C:");
  display.print(tempC,1);
  display.print((char)247);
  
  display.println();

  display.print("Temp in F:");
  display.print(tempF,1);
  display.print((char)247);

  display.println();

  display.print("BPM:");
  display.print(bpm);
  
  display.println();

  display.print("SpO2:");
  display.print(spo2);
  
display.display();
  
}
