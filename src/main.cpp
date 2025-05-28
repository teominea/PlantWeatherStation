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
#define UV_PIN 33
#define N_SAMPLES 10

#include <Arduino.h>
#include <DHT.h>
#include "BluetoothSerial.h"
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);



// Variables

const int buzzerPin = 25;
const int freq = 2000;
const int channel = 0;
const int resolution = 8;

float tempMinThreshold = 15.0;
float tempMaxThreshold = 30.0;
float humMinThreshold = 30.0;
float humMaxThreshold = 80.0;
float soilMinThreshold = 20.0;
float soilMaxThreshold = 80.0;
float lightMinThreshold = 10.0;
float lightMaxThreshold = 90.0;


bool alarmOn = false;
bool tempTooHigh = false;
bool tempTooLow = false;
bool humTooHigh = false;
bool humTooLow = false;
bool lightTooHigh = false;
bool lightTooLow = false;
bool soilMoistureTooLow = false;
bool soilMoistureTooHigh = false;

// avoid multiple error messages
bool alreadyOneError = false;

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
const int numPlants = 14;
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
  { "Dahlia",     10.0, 30.0,   50.0, 80.0,   50.0, 80.0,   80.0,100.0 },
  {"Use as default", 15.0, 28.0, 30.0, 80.0, 20.0, 70.0, 30.0, 80.0}
};

// DONT DELETE THESE LINES

char auth[] = "LbaA_MvyzdZJvx_28cdx5xovWM3cq2yy";
char ssid[] = "HUAWEI P30 Pro"; // Your WiFi SSID
char pass[] = "aaaaaaaa"; // Your WiFi Password

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
BluetoothSerial SerialBT; // Create Bluetooth Serial object

BlynkTimer timer; // Create a timer object for Blynk




// Threshold meniu from Bluetooth Serial
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

// Plant menu from Bluetooth Serial
void printPlantMenu() {
  SerialBT.println();
  SerialBT.println(F("Select plant type by number:\n"));
  for(int i=0; i<numPlants; i++){
    SerialBT.printf("  %2d. %s\n", i+1, plants[i].name);
  }
  SerialBT.println(F("\nOr type BACK to return.\n"));
  SerialBT.print(F("Your choice > "));
}

// Function to print the main menu in Bluetooth Serial
void printMainMenu() {
  SerialBT.println();
  SerialBT.println(F("Hi there! Welcome to Plant Weather Station!\n"));
  SerialBT.println(F("Choose an option:\n"));
  SerialBT.println(F("  1. Change Thresholds"));
  SerialBT.println(F("  2. Select Plant\n"));
  SerialBT.print  (F("Enter 1 or 2: "));
}

// Function to read light sensor value
int readLightRaw() {
  // Read multiple samples to get a more stable value
  long sum = 0;
  for (int i = 0; i < N_SAMPLES; i++) {
    sum += analogRead(LIGHT_PIN);
    delay(5); // mică pauză între eșantioane
  }
  // Return the average value
  return sum / N_SAMPLES;
}

// Blynk - send sensor data
void sendSensor() {
  // Read humidity and temperature from DHT sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Read soil moisture
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercent = map(soilMoisture, 3500, 1000, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);
  
  // Read light sensor
  int raw = readLightRaw();
  float pct = map(raw, 0, 4095, 0, 100);
  int percentLight = constrain(pct, 0, 100);
  
  // Send data to Blynk
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, soilMoisturePercent);
  Blynk.virtualWrite(V3, percentLight);

}


void setup() {

  // Initialize serial communication at 115200 baud rate
  Serial.begin(115200); 

  // Start the DHT sensor
  dht.begin();

  // Blynk initialization
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendSensor);

  // Initialize Bluetooth Serial
  SerialBT.begin("MyPlantStation");
  Serial.println("Bluetooth device is ready to pair");


  // Initialize soil moisture sensor
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  // Initialize light sensor pin as input
  pinMode(LIGHT_PIN, INPUT); 
 

  // Initialize led pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize the buzzer pin
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(buzzerPin, channel);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  // Clear the display buffer, set text size and color
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  // Print initial message on the display
  display.println("Plant Monitor");
  display.display();
  delay(2000);
  // Print the main menu on the Bluetooth Serial
  printMainMenu();
}

void loop() {

  // Show the main menu if a Bluetooth client is connected
  if (SerialBT.hasClient() && !menuShown) {
    menuShown = true; 
    printMainMenu();
  }
  // If no Bluetooth client is connected, reset the menu state
  if (!SerialBT.hasClient()) {
    menuShown = false;
    menuState = MENU_MAIN;
  } 
  

  Blynk.run();
  timer.run();

  if (SerialBT.available()) {
  String cmd = SerialBT.readStringUntil('\n');
  cmd.trim();

  switch (menuState) {
    // If in main menu, handle commands
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
      // If user selects back, return to main menu
      if (cmd.equalsIgnoreCase("BACK")) {
        menuState = MENU_MAIN;
        printMainMenu();
      }
      // If user enters a threshold command, parse and set the threshold
      else if (cmd.startsWith("TMN:")) {
        float tempMinThresholdAux = cmd.substring(4).toFloat();
        if (tempMinThresholdAux > 0 && tempMinThresholdAux < 50) {
          tempMinThreshold = tempMinThresholdAux;
        } else {
          SerialBT.println(F("Invalid temperature min threshold. Must be between -10 and 50."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("Temp min = %.1f°C\n", tempMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("TMX:")) {
        float tempMaxThresholdAux = cmd.substring(4).toFloat();
        if (tempMaxThresholdAux > 0 && tempMaxThresholdAux < 50) {
          tempMaxThreshold = tempMaxThresholdAux;
        } else {
          SerialBT.println(F("Invalid temperature max threshold. Must be between -10 and 50."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("Temp max = %.1f°C\n", tempMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("AHN:")) {
        float humMinThresholdAux = cmd.substring(4).toFloat();
        if (humMinThresholdAux > 0 && humMinThresholdAux < 100) {
          humMinThreshold = humMinThresholdAux;
        } else {
          SerialBT.println(F("Invalid humidity min threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("AirHum min = %.1f%%\n", humMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("AHX:")) {
        float humMaxThresholdAux = cmd.substring(4).toFloat();
        if (humMaxThresholdAux > 0 && humMaxThresholdAux < 100) {
          humMaxThreshold = humMaxThresholdAux;
        } else {
          SerialBT.println(F("Invalid humidity max threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("AirHum max = %.1f%%\n", humMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("SHN:")) {
        float soilMinThresholdAux = cmd.substring(4).toFloat();
        if (soilMinThresholdAux >= 0 && soilMinThresholdAux <= 100) {
          soilMinThreshold = soilMinThresholdAux;
        } else {
          SerialBT.println(F("Invalid soil moisture min threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("SoilH min = %.1f%%\n", soilMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("SHX:")) {
        float soilMaxThresholdAux = cmd.substring(4).toFloat();
        if (soilMaxThresholdAux >= 0 && soilMaxThresholdAux <= 100) {
          soilMaxThreshold = soilMaxThresholdAux;
        } else {
          SerialBT.println(F("Invalid soil moisture max threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("SoilH max = %.1f%%\n", soilMaxThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("LMN:")) {
        float lightMinThresholdAux = cmd.substring(4).toFloat();
        if (lightMinThresholdAux >= 0 && lightMinThresholdAux <= 100) {
          lightMinThreshold = lightMinThresholdAux;
        } else {
          SerialBT.println(F("Invalid light min threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
        SerialBT.printf("Light min = %.1f%%\n", lightMinThreshold);
        printThresholdMenu();
      }
      else if (cmd.startsWith("LMX:")) {
        float lightMaxThresholdAux = cmd.substring(4).toFloat();
        if (lightMaxThresholdAux >= 0 && lightMaxThresholdAux <= 100) {
          lightMaxThreshold = lightMaxThresholdAux;
        } else {
          SerialBT.println(F("Invalid light max threshold. Must be between 0 and 100."));
          printThresholdMenu();
          return;
        }
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

  // Reset error flags
  alarmOn = false;

  tempTooHigh = false;
  humTooHigh = false;
  lightTooHigh = false;
  lightTooLow = false;
  soilMoistureTooLow = false;
  soilMoistureTooHigh = false;
  tempTooLow = false;
  humTooLow = false;

  // Read sensor values from DHT sensor and soil moisture sensor
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercent = map(soilMoisture, 3500, 1000, 0, 100); // Adjust the mapping based on your sensor's range
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100); // Ensure the value is between 0 and 100

  // Read light sensor value
  int raw = readLightRaw();
  float pct = map(raw, 0, 4095, 0, 100);
  int percentLight = constrain(pct, 0, 100);

  // Check if the sensor readings are valid
  if (t > tempMaxThreshold) {
    alarmOn = true;
    tempTooHigh = true;
  }
  if (t < tempMinThreshold) {
    alarmOn = true;
    tempTooLow = true;
  }

  if (h > humMaxThreshold) {
    alarmOn = true;
    humTooHigh = true;
  }

  if (h < humMinThreshold) {
    alarmOn = true;
    humTooLow = true;
  }

  if (percentLight > lightMaxThreshold) {
    alarmOn = true;
    lightTooHigh = true;
  }

  if (percentLight < lightMinThreshold) {
    lightTooLow = true;
  }

  if (soilMoisturePercent < soilMinThreshold) {
    alarmOn = true;
    soilMoistureTooLow = true;
  }

  if (soilMoisturePercent > soilMaxThreshold) {
    alarmOn = true;
    soilMoistureTooHigh = true;
  }

  alreadyOneError = false; // Reset error flag

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
    display.setCursor(0, 50);
    if (tempTooHigh && !alreadyOneError) {
      display.println("Temp Too High!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    if (humTooHigh && !alreadyOneError) {
      display.println("Humidity Too High!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    if (lightTooHigh && !alreadyOneError) {
      display.println("Light Too High!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    if (tempTooLow && !alreadyOneError) {
      display.println("Temp Too Low!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    if (humTooLow && !alreadyOneError) {
      display.println("Humidity Too Low!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    if (soilMoistureTooHigh && !alreadyOneError) {
      display.println("Soil Moisture Too High!");
      alreadyOneError = true; // Set the flag to true after the first error
    }

    if (soilMoistureTooLow && !alreadyOneError) {
      display.println("Soil Moisture Too Low!");
      alreadyOneError = true; // Set the flag to true after the first error
    }
    display.setTextColor(SSD1306_WHITE); // Reset text color
    display.setCursor(0, 50);
  }

  if (lightTooLow) {
    if (!alreadyOneError) {
      alreadyOneError = true; // Set the flag to true after the first error
      display.println("\nLight Too Low! Artificial light on.");
    }
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