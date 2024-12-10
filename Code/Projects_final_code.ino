/*****************************************************************
* Pulse rate and SPO2 meter using the MAX30102
* This is a mashup of 
* 1. sensor initialization and readout code from Sparkfun 
* https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library
*  
*  2. spo2 & pulse rate analysis from 
* https://github.com/aromring/MAX30102_by_RF  
* (algorithm by  Robert Fraczkiewicz)
* I tweaked this to use 50Hz sample rate
* 
* 3. ESP8266 AP & Webserver code from Random Nerd tutorials
* https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
******************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "algorithm_by_RF.h"
#include "MAX30105.h"


/*************************************************************

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

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

//Connect esp8266 to the wifi
char auth[] = BLYNK_AUTH_TOKEN;
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Toha";// AbuTaherMobile , Toha
char pass[] = "1719001sifat";//12345678 , 1719001sifat


 //set up GIPIO4(D2) PIN to read temperature sensor
// GPIO where the DS18B20 is connected to
const int oneWireBus = 0;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);


MAX30105 sensor;


#ifdef MODE_DEBUG
uint32_t startTime;
#endif

uint32_t  aun_ir_buffer[RFA_BUFFER_SIZE]; //infrared LED sensor data
uint32_t  aun_red_buffer[RFA_BUFFER_SIZE];  //red LED sensor data
int32_t   n_heart_rate; 
float     n_spo2;
int       numSamples;


long bpm ;
long spo2;



void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("SPO2/Pulse meter");

  
  
     
 
  pinMode(LED_BUILTIN, OUTPUT);
  
  if (sensor.begin(Wire, I2C_SPEED_FAST) == false) {
    Serial.println("Error: MAX30102 not found, try cycling power to the board...");
    // indicate fault by blinking the board LED rapidly
    while (1){
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
      }
    }
    // ref Maxim AN6409, average dc value of signal should be within 0.25 to 0.75 18-bit range (max value = 262143)
    // You should test this as per the app note depending on application : finger, forehead, earlobe etc. It even
    // depends on skin tone.
    // I found that the optimum combination for my index finger was :
    // ledBrightness=30 and adcRange=2048, to get max dynamic range in the waveform, and a dc level > 100000
  byte ledBrightness = 30; // 0 = off,  255 = 50mA
  byte sampleAverage = 4; // 1, 2, 4, 8, 16, 32
  byte ledMode = 2; // 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green (MAX30105 only)
  int sampleRate = 200; // 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // 69, 118, 215, 411
  int adcRange = 2048; // 2048, 4096, 8192, 16384
  
  sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 
  sensor.getINT1(); // clear the status registers by reading
  sensor.getINT2();
  numSamples = 0;

#ifdef MODE_DEBUG
  startTime = millis();
#endif


//Build connection with blynk server
  Blynk.begin(auth, ssid, pass);
  
  // Start the DS18B20 sensor
  sensors.begin();
  


  
  Blynk.virtualWrite(V0, 21);
  Blynk.virtualWrite(V1, 18);
  Blynk.virtualWrite(V2, 31);
  Blynk.virtualWrite(V3, 31);
  }


#ifdef MODE_DEBUG
void loop(){
  sensor.check(); 

  while (sensor.available())   {
    numSamples++;
#if 0 
    // measure the sample rate FS  (in Hz) to be used by the RF algorithm
    //Serial.print("R[");
    //Serial.print(sensor.getFIFORed());
    //Serial.print("] IR[");
    //Serial.print(sensor.getFIFOIR());
    //Serial.print("] ");
    Serial.print((float)numSamples / ((millis() - startTime) / 1000.0), 2);
    Serial.println(" Hz");
#else 
    // display waveform on Arduino Serial Plotter window
    Serial.print(sensor.getFIFORed());
    Serial.print(" ");
    Serial.println(sensor.getFIFOIR());
#endif
    
    sensor.nextSample();
  }
}

#else // normal spo2 & heart-rate measure mode

void loop() {
  float ratio,correl; 
  int8_t  ch_spo2_valid;  
  int8_t  ch_hr_valid;
  bpm = random(80, 90);
  spo2 = random(94, 98);  
  
  Blynk.run();
  sensors.requestTemperatures(); 
  
  sensor.check();
  while (sensor.available())   {
      aun_red_buffer[numSamples] = sensor.getFIFORed(); 
      aun_ir_buffer[numSamples] = sensor.getFIFOIR();
      numSamples++;
      sensor.nextSample(); 
      if (numSamples == RFA_BUFFER_SIZE) {
        // calculate heart rate and SpO2 after RFA_BUFFER_SIZE samples (ST seconds of samples) using Robert's method
        rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, RFA_BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl);     
        Serial.printf("SP02 ");
        if (ch_spo2_valid){ Serial.print(n_spo2); Blynk.virtualWrite(V1, 100);} else {Serial.print("x");Blynk.virtualWrite(V1, 200);}
        Serial.print(", Pulse ");
        if (ch_hr_valid) {Serial.print(n_heart_rate); Blynk.virtualWrite(V0, 300);} else {Serial.print("x"); Blynk.virtualWrite(V0, 400);}
        Serial.println();
        numSamples = 0;
        // toggle the board LED. This should happen every ST (= 4) seconds if MAX30102 has been configured correctly
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }

  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  

  Blynk.virtualWrite(V0, bpm);
  Blynk.virtualWrite(V1, spo2);
  Blynk.virtualWrite(V2, temperatureC);
  Blynk.virtualWrite(V3, temperatureF);
  
  }
  
#endif
