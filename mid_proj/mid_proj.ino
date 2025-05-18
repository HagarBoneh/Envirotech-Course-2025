#include <Wire.h>                            // This helps the Arduino talk to devices over I2C (like our sensors)
#include <Adafruit_SHTC3.h>                  // This library is for the SHTC3 temperature & humidity sensor
#include <SparkFun_SCD4x_Arduino_Library.h>  // And this one is for the SCD4x sensor (CO2 + temp & humidity)
#include <SD.h>                              // For reading and writing to an SD card

// --- Setting up the sensors ---
Adafruit_SHTC3 shtc3;
SCD4x scd4x;

const int ledPin = 13;                  // The onboard LED — we’ll use it as a basic status light
const int chipSelect = 8;              // This tells the SD card which pin to listen on
const char* logFile = "log1.txt";      // This is the file where all the data will be saved
unsigned long startTime;               // Used to track how long we’ve been logging

void setup() {
  Serial.begin(115200);                // Start talking to the computer — fast enough to keep up with data
  delay(1000);                         // Give it a sec to connect

  Serial.println("[BOOT] Starting environmental logger...");

  pinMode(ledPin, OUTPUT);             // We'll blink the LED to show when a reading is happening

  // --- Let’s check the sensors ---
  Serial.println("Initializing sensors...");
  if (!shtc3.begin()) {
    Serial.println("Uh-oh... couldn't find the SHTC3 sensor.");
    while (1) delay(10);              // Stop everything if we can’t find it
  }
  Serial.println("SHTC3 sensor is ready to go.");

  if (!scd4x.begin()) {
    Serial.println("Uh-oh... couldn't find the SCD4x sensor.");
    while (1) delay(10);              // Stop here too if CO2 sensor isn’t found
  }
  Serial.println("SCD4x sensor is good to go.");

  delay(1000);                        // Just a little pause before we try the SD card

  // --- SD card setup ---
  Serial.print("Checking SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Nope. SD card didn’t initialize.");
    while (1) delay(10);
  }
  Serial.println("SD card is ready!");

  delay(2000);                        // Give it a bit more time to get stable

  Serial.println("Skipping the file header to avoid any issues.");
  Serial.println("Setup complete. Let’s get logging!");

  scd4x.startPeriodicMeasurement();   // Start CO2 sensor readings in the background
  startTime = millis();               // Mark the time we started
}

void loop() {
  digitalWrite(ledPin, HIGH);         // Turn LED on so we know it’s doing something
  delay(500);                         // Leave it on for a moment so it's noticeable

  // --- Grab data from SHTC3 ---
  sensors_event_t shtc3_humidity_event, shtc3_temp_event;
  float shtc3_temperature = 0;
  float shtc3_humidity = 0;

  if (shtc3.getEvent(&shtc3_humidity_event, &shtc3_temp_event)) {
    shtc3_temperature = shtc3_temp_event.temperature;
    shtc3_humidity = shtc3_humidity_event.relative_humidity;

    Serial.print("SHTC3 - Temp: ");
    Serial.print(shtc3_temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(shtc3_humidity);
    Serial.println(" %");
  } else {
    Serial.println("Hmm, couldn’t read from the SHTC3 sensor.");
  }

  // --- Now grab data from the SCD4x ---
  uint16_t scd4x_co2 = 0;
  float scd4x_temperature = 0;
  float scd4x_humidity = 0;

  if (scd4x.readMeasurement()) {
    scd4x_co2 = scd4x.getCO2();
    scd4x_temperature = scd4x.getTemperature();
    scd4x_humidity = scd4x.getHumidity();

    Serial.print("SCD4x - CO2: ");
    Serial.print(scd4x_co2);
    Serial.print(" ppm, Temp: ");
    Serial.print(scd4x_temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(scd4x_humidity);
    Serial.println(" %");
  } else {
    Serial.println("Hmm, couldn’t read from the SCD4x sensor.");
  }

  // --- Save everything to the SD card ---
  File dataFile = SD.open(logFile, FILE_WRITE);
  if (dataFile) {
    unsigned long currentTime = millis();  // How long it’s been running in milliseconds

    dataFile.print(currentTime);           dataFile.print(",");
    dataFile.print(shtc3_temperature);     dataFile.print(",");
    dataFile.print(shtc3_humidity);        dataFile.print(",");
    dataFile.print(scd4x_temperature);     dataFile.print(",");
    dataFile.print(scd4x_humidity);        dataFile.print(",");
    dataFile.println(scd4x_co2);

    dataFile.flush();
    dataFile.close();
    Serial.println("Wrote to SD card successfully.");
  } else {
    Serial.println("Couldn’t open log1.txt to save data.");
  }

  // --- For real-time plotter or debugging ---
  Serial.print("<DATA>");
  Serial.print(shtc3_temperature); Serial.print(",");
  Serial.print(scd4x_temperature); Serial.print(",");
  Serial.print(shtc3_humidity);    Serial.print(",");
  Serial.print(scd4x_humidity);    Serial.print(",");
  Serial.print(scd4x_co2);
  Serial.println("</DATA>");

  digitalWrite(ledPin, LOW);        // LED off to mark the end of this cycle
  delay(14500);                     // Chill for the rest of the 15-second cycle
}
