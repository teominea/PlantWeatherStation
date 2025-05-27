#define BLYNK_PRINT Serial
#define DHTPIN 4
#define DHTTYPE DHT11  

#define BLYNK_TEMPLATE_ID "TMPL4mUQwYNC0"
#define BLYNK_TEMPLATE_NAME "WeatherStation"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     21
#define OLED_CS     5
#define OLED_RESET  22
#define SOIL_MOISTURE_PIN 36
#define LED_PIN 26
#define LIGHT_PIN 32
#define MENU_MAIN 0
#define MENU_THRESHOLDS 1
#define MENU_PLANT 2

#include <Arduino.h>
#include <DHT.h>
#include "BluetoothSerial.h"
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);




// put function declarations here:
int myFunction(int, int);


// Variables

const int buzzerPin = 25;
const int freq    = 2000;
const int channel = 0;
const int resolution = 8;

float tempMinThreshold       = 15.0;
float tempMaxThreshold       = 30.0;
float humMinThreshold        = 30.0;
float humMaxThreshold        = 70.0;
float soilMinThreshold       = 20.0;
float soilMaxThreshold       = 80.0;
float lightMinThreshold      = 10.0;
float lightMaxThreshold      = 90.0;


bool alarmOn = false;
bool tempTooHigh = false;
bool tempTooLow = false;
bool humTooHigh = false;
bool humTooLow = false;
bool lightTooHigh = false;
bool lightTooLow = false;
bool soilMoistureTooLow = false;
bool soilMoistureTooHigh = false;

// Menu state
int menuState = MENU_MAIN;
bool menuShown = false;


// Plant data structure
struct Plant {
  const char* name;
  float tempMin;
  float tempMax;
  float humMin;
  float humMax;
  float soilMin;
  float soilMax;
  float lightMin;
  float lightMax;
};

// Array of plants
const int numPlants = 13;
Plant plants[numPlants] = {
  {"Cactus", 20.0, 30.0, 10.0, 30.0, 10.0, 50.0, 20.0, 80.0},
  {"Fern",   15.0, 25.0, 40.0, 70.0, 20.0, 60.0, 30.0, 90.0},
  {"Rose",   18.0, 28.0, 30.0, 60.0, 20.0, 70.0, 40.0, 100.0},
  { "Basil",        16.0, 27.0,  50.0, 80.0,  30.0, 60.0,  40.0, 80.0 },
  { "Orchid",       18.0, 30.0,  60.0, 80.0,  50.0, 70.0,  10.0, 60.0 },
  { "Succulent",    20.0, 35.0,  10.0, 40.0,  10.0, 30.0,  60.0,100.0 },
  { "Lavender",     15.0, 25.0,  40.0, 70.0,  20.0, 50.0,  50.0, 90.0 },
  { "Mint",         15.0, 25.0,  50.0, 90.0,  40.0, 70.0,  20.0, 60.0 },
  { "Spider Plant", 15.0, 25.0,  50.0, 70.0,  40.0, 70.0,  30.0, 60.0 },
  { "Pothos",       16.0, 26.0,  50.0, 90.0,  30.0, 60.0,  20.0, 80.0 },
  { "Dandelion",  10.0, 25.0,   40.0, 80.0,   40.0, 80.0,   70.0,100.0 },
  { "Tulip",       5.0, 18.0,   50.0, 80.0,   40.0, 60.0,   50.0, 70.0 },
  { "Dahlia",     10.0, 30.0,   50.0, 80.0,   50.0, 80.0,   80.0,100.0 }
};

// DONT DELETE THESE LINES

// char auth[] = "LbaA_MvyzdZJvx_28cdx5xovWM3cq2yy";
// char ssid[] = "HUAWEI P30 Pro"; // Your WiFi SSID
// char pass[] = "aaaaaaaa"; // Your WiFi Password

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
BluetoothSerial SerialBT; // Create Bluetooth Serial object

BlynkTimer timer; // Create a timer object for Blynk

void sendSensor() {
  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Read soil moisture
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercent = map(soilMoisture, 3500, 1000, 0, 100); // Adjust the mapping based on your sensor's range
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100); // Ensure the value is between 0 and 100
  // Read light sensor
  int lightValue = analogRead(LIGHT_PIN);
  float lightVoltage = (lightValue / 4095.0) * 3.3; // Convert ADC value to voltage
  float percentLight = map(lightValue, 0, 4095, 0, 100);
  
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, soilMoisturePercent);
  Blynk.virtualWrite(V3, percentLight);

}

void printThresholdMenu() {
  SerialBT.println();
  SerialBT.println(F("Current thresholds:\n"));
  SerialBT.printf("  Temp  : %.1f .. %.1f °C\n", tempMinThreshold, tempMaxThreshold);
  SerialBT.printf("  AirHum: %.1f .. %.1f %%\n", humMinThreshold, humMaxThreshold);
  SerialBT.printf("  SoilH : %.1f .. %.1f %%\n", soilMinThreshold, soilMaxThreshold);
  SerialBT.printf("  Light : %.1f .. %.1f %%\n", lightMinThreshold, lightMaxThreshold);
  SerialBT.println(F("\nTo change, type:\n"));
  SerialBT.println(F("  TMN:<val>  (Temp min)"));
  SerialBT.println(F("  TMX:<val>  (Temp max)"));
  SerialBT.println(F("  AHN:<val>  (AirHum min)"));
  SerialBT.println(F("  AHX:<val>  (AirHum max)"));
  SerialBT.println(F("  SHN:<val>  (SoilH min)"));
  SerialBT.println(F("  SHX:<val>  (SoilH max)"));
  SerialBT.println(F("  LMN:<val>  (Light min)"));
  SerialBT.println(F("  LMX:<val>  (Light max)"));
  SerialBT.println(F("\nOr type BACK to return.\n"));
  SerialBT.print(F("Your choice > "));
}


void printPlantMenu() {
  SerialBT.println();
  SerialBT.println(F("Select plant type by number:\n"));
  for(int i=0; i<numPlants; i++){
    SerialBT.printf("  %2d. %s\n", i+1, plants[i].name);
  }
  SerialBT.println(F("\nOr type BACK to return.\n"));
  SerialBT.print(F("Your choice > "));
}

void printMainMenu() {
  SerialBT.println();
  SerialBT.println(F("Hi there! Welcome to Plant Weather Station!\n"));
  SerialBT.println(F("Choose an option:\n"));
  SerialBT.println(F("  1. Change Thresholds"));
  SerialBT.println(F("  2. Select Plant\n"));
  SerialBT.print  (F("Enter 1 or 2: "));
}


void setup() {
  
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  dht.begin(); // Start the DHT sensor


  // Blynk.begin(auth, ssid, pass);
  // timer.setInterval(1000L, sendSensor);

  Serial.println("DHT11 test!");
  Serial.println("Reading temperature and humidity...");

  SerialBT.begin("ESP32"); // Start Bluetooth with the name "ESP32"
  Serial.println("Bluetooth device is ready to pair");

  // printMainMenu();

  // Initialize soil moisture sensor
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  pinMode(LIGHT_PIN, INPUT); // Initialize light sensor pin

  pinMode(LED_PIN, OUTPUT); // Initialize UV sensor pin
  digitalWrite(LED_PIN, LOW); // Set LED pin to LOW

  // Initialize the buzzer pin
  ledcSetup(channel, freq, resolution); // Set up the PWM properties
  ledcAttachPin(buzzerPin, channel); // Attach the buzzer pin to the PWM channel

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Plant Monitor");
  display.display();
  delay(2000); // Pause for 2 seconds

  printMainMenu();
}

void loop() {


  if (SerialBT.hasClient() && !menuShown) {
    menuShown = true; 
    printMainMenu();
  }
  
  // Blynk.run(); // Run Blynk
  // timer.run(); // Run the timer to check for events

  if (SerialBT.available()) {
  String cmd = SerialBT.readStringUntil('\n');
  cmd.trim();

  switch (menuState) {
    case MENU_MAIN:
      if (cmd == "1") {
        menuState = MENU_THRESHOLDS;
        printThresholdMenu();
      }
      else if (cmd == "2") {
        menuState = MENU_PLANT;
        printPlantMenu();
      }
      else {
        SerialBT.println(F("Invalid. Enter 1 or 2."));
        printMainMenu();
      }
      break;

    case MENU_THRESHOLDS:
      if (cmd.equalsIgnoreCase("BACK")) {
        menuState = MENU_MAIN;
        printMainMenu();
      }
      else if (cmd.startsWith("TMN:")) {
        tempMinThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("Temp min = %.1f°C\n", tempMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("TMX:")) {
        tempMaxThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("Temp max = %.1f°C\n", tempMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("AHN:")) {
        humMinThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("AirHum min = %.1f%%\n", humMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("AHX:")) {
        humMaxThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("AirHum max = %.1f%%\n", humMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("SHN:")) {
        soilMinThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("SoilH min = %.1f%%\n", soilMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("SHX:")) {
        soilMaxThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("SoilH max = %.1f%%\n", soilMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("LMN:")) {
        lightMinThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("Light min = %.1f%%\n", lightMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("LMX:")) {
        lightMaxThreshold = cmd.substring(4).toFloat();
        SerialBT.printf("Light max = %.1f%%\n", lightMaxThreshold);
        printThresholdMenu();
      }
      else {
        SerialBT.println(F("Invalid command."));
        printThresholdMenu();
      }
      break;

    case MENU_PLANT:
      if (cmd.equalsIgnoreCase("BACK")) {
        menuState = MENU_MAIN;
        printMainMenu();
      }
      else {
        int choice = cmd.toInt();
        if (choice >= 1 && choice <= numPlants) {
          Plant &p = plants[choice - 1];
          tempMinThreshold   = p.tempMin;
          tempMaxThreshold   = p.tempMax;
          humMinThreshold    = p.humMin;
          humMaxThreshold    = p.humMax;
          soilMinThreshold   = p.soilMin;
          soilMaxThreshold   = p.soilMax;
          lightMinThreshold  = p.lightMin;
          lightMaxThreshold  = p.lightMax;
          SerialBT.printf("Plant \"%s\" selected. Thresholds applied.\n", p.name);
            menuState = MENU_MAIN;
          printMainMenu();
        }
        else {
          SerialBT.println(F("Invalid number."));
          printPlantMenu();
        }
      }
      break;
    }

    // Exit loop
    return;
  }

  alarmOn = false; // Reset alarm status

  tempTooHigh = false;
  humTooHigh = false;
  lightTooHigh = false;
  lightTooLow = false;
  soilMoistureTooLow = false;
  soilMoistureTooHigh = false;
  tempTooLow = false;
  humTooLow = false;


  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercent = map(soilMoisture, 3500, 1000, 0, 100); // Adjust the mapping based on your sensor's range
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100); // Ensure the value is between 0 and 100*


  int lightValue = analogRead(LIGHT_PIN);
  float lightVoltage = (lightValue / 4095.0) * 3.3; // Convert ADC value to voltage
  float percentLight = map(lightValue, 0, 4095, 0, 100);
  

  // Serial.println("Light Value: " + String(lightValue));
  // Serial.println("Light Voltage: " + String(lightVoltage));
  // Serial.println("Soil Moisture Value: " + String(soilMoisture));

  if (t > tempMaxThreshold) {
    // Serial.println("Temperature is too high!");
    alarmOn = true;
    tempTooHigh = true;
  }
  if (t < tempMinThreshold) {
    // Serial.println("Temperature is too low!");
    alarmOn = true;
    tempTooLow = true;
  }

  if (h > humMaxThreshold) {
    // Serial.println("Humidity is too high!");
    alarmOn = true;
    humTooHigh = true;
  }

  if (h < humMinThreshold) {
    // Serial.println("Humidity is too low!");
    alarmOn = true;
    humTooLow = true;
  }

  if (percentLight > lightMaxThreshold) {
    // Serial.println("Light level is too high!");
    alarmOn = true;
    lightTooHigh = true;
  }

  if (percentLight < lightMinThreshold) {
    // Serial.println("Light level is too low!");
    alarmOn = true;
    lightTooLow = true;
  }

  if (soilMoisturePercent < soilMinThreshold) {
    // Serial.println("Soil moisture is too low!");
    alarmOn = true;
    soilMoistureTooLow = true;
  }

  if (soilMoisturePercent > soilMaxThreshold) {
    // Serial.println("Soil moisture is too high!");
    alarmOn = true;
    soilMoistureTooHigh = true;
  }



  if (alarmOn) {
    ledcWrite(channel, 128);
  } else {
    ledcWrite(channel, 0);
  }

    // Display temperature and humidity on OLED
    display.clearDisplay();
    display.setCursor(0,0);

  if (isnan(h) || isnan(t)) {
    display.println("Error");
    return;
  } else {
    display.print("Temp: ");
    display.print(t, 1);
    display.println(" C");

    display.print("Hum: ");
    display.print(h, 1);
    display.println(" %");
  }

  display.print("Soil Hum: ");
  display.print(soilMoisturePercent, 1);
  display.println(" %");

  display.print("Light: ");
  display.print(percentLight, 1);
  display.println(" %");


  if (alarmOn) {
    // Write the alarm message to the display
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Invert colors for alarm
    display.setCursor(0, 50);
    if (tempTooHigh) {
      display.println("Temp Too High!");
    }
    if (humTooHigh) {
      display.println("Humidity Too High!");
    }
    if (lightTooHigh) {
      display.println("Light Too High!");
    }
    if (tempTooLow) {
      display.println("Temp Too Low!");
    }
    if (humTooLow) {
      display.println("Humidity Too Low!");
    }
    if (soilMoistureTooHigh) {
      display.println("Soil Moisture Too High!");
    }
    
    if (soilMoistureTooLow) {
      display.println("Soil Moisture Too Low!");
    }
    display.setTextColor(SSD1306_WHITE); // Reset text color
    display.setCursor(0, 50);
  }

  if (lightTooLow) {
    display.println("Light Too Low! Turning artificial light on.");
    // turn on the LED
    digitalWrite(LED_PIN, HIGH); // Turn on the LED
  } else {
    digitalWrite(LED_PIN, LOW); // Turn off the LED
  }


  display.display(); // Update the display
  delay(2000); // Pause for 2 seconds





 
  

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}