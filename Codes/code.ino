#include <WiFi.h>                                                           // Including WIFI Library file.
#include <ThingSpeak.h>                                                     // Including ThingSpeak Library file.
#include <Wire.h>                                                           // Including Wire Lirary file(I2C Communication).
#include <LiquidCrystal_I2C.h>                                              // Including LCD Library fie.
#include <DHT.h>                                                            // Including DHT Library file(DHT11 Sensor).

#define DHTPIN 4                                                            // DHT Sensor to PIN 4 of ESP32.
#define DHTTYPE DHT11                                                       // DHT Sensor type is DHT11. 
#define BUZZER 18                                                           // Buzzer to PIN 18 of ESP32.
#define PHPIN 34                                                            // PH Sensor to PIN 34 of ESP32.
#define TURBIDITYPIN 35                                                     // Turbidity Sensor to PIN 35 of ESP32.

const char* ssid = "WIFI NAME";                                             // Your WIFI Name.
const char* password = "WIFI PASSWORD";                                     // Your WIFI Password.

unsigned long channelID = CHANNEL ID;                                       // ThingSpeak Channel ID.
const char *writeAPIKey = "WRITE API KEY"                                   // ThingSpeak Channel Write API Key.

WiFiClient client;                                                          // Create a TCP client for connecting to servers. 

DHT dht(DHTPIN, DHTTYPE);                                                   // Reads temperature and humidity from a DHT11 Sensor.

LiquidCrystal_I2C lcd(0x27, 16, 2);                                         // Controls an LCD display via I²C. 
                                                                            // 0x27 is the I²C address and 16,2 means a 16×2 LCD.

void setup()
{
  Serial.begin(115200);                                                    // Starts the serial monitor at 115200 baudrate.
 
  pinMode(BUZZER, OUTPUT);                                                 // Sets the buzzer pin as an output.
  digitalWrite(BUZZER, LOW);                                               // Immediately writes it LOW so it’s off at startup.

  Wire.begin(21, 22);                                                      // Initializes the I²C bus on ESP32 with:SDA = GPIO 21,SCL = GPIO 22.

  lcd.init();                                                              // Initializes the I²C LCD.
  lcd.backlight();                                                         // Turns on the LCD backlight.
  lcd.clear();                                                             // Clears any leftover text from previous runs.

  lcd.setCursor(0, 0);                                                     // Set Cursor position to first column,first row.
  lcd.print("Water Quality");                                              // Prints "Water Quality" on LCD.
  lcd.setCursor(0, 1);                                                     // Set Cursor position to first column,Second row.
  lcd.print("Monitoring");                                                 // Prints "Monitoring" on LCD.
  delay(2000);                                                             // Delay of 20 seconds.
  dht.begin();                                                             // Initializes the DHT sensor to read temperature/humidity later.

  lcd.clear();                                                             // Clears any leftover text from previous runs.                            
  lcd.setCursor(0, 0);                                                     // Set Cursor position to first column,first row.
  lcd.print("Connecting WiFi");                                            // Prints "Connecting WiFi" on LCD.
  WiFi.mode(WIFI_STA);                                                     // Connect to an existing network.
  WiFi.begin(ssid, password);                                              // Starts connecting to the Wi‑Fi network with credentials.
  int retry = 0;                                                           // Initializes a counter to limit connection attempts.

  while (WiFi.status() != WL_CONNECTED && retry < 30)                      // Wi‑Fi connection retry mechanism.
  {
    delay(500);                                                            // Delay of 5 seconds.
    Serial.print(".");                                                     // Prints a dot to show progress in Serial Monitor
    retry++;                                                               // Count this attempt.
  }
  lcd.clear();                                                            // Clears any leftover text from previous runs. 

  if (WiFi.status() == WL_CONNECTED)                                      // If the ESP32 is connected to the network.
  {
    Serial.println("\nWiFi Connected");                                   // Prints "WiFi Connected" to the Serial Monitor.
    lcd.setCursor(0, 0);                                                  // Set Cursor position to first column,first row.
    lcd.print("WiFi Connected");                                          // Prints "WiFi Connected" on LCD.
    lcd.setCursor(0, 1);                                                  // Set Cursor position to first column,second row.
    lcd.print(WiFi.localIP());                                            // Shows the local IP address on the LCD.
    ThingSpeak.begin(client);                                             // Calls ThingSpeak client to initialize the ThingSpeak connection using WiFiClient object.
    delay(3000);                                                          // Delay of 30 seconds.
  }
  
  else                                                                    // If not connected.
  {
    Serial.println("\nWiFi Failed");                                      // Prints "WiFi Failed" to the Serial Monitor.
    lcd.setCursor(0, 0);                                                  // Set Cursor position to first column,first row.
    lcd.print("WiFi Failed");                                             // Prints "WiFi Failed" on LCD.
    delay(3000);                                                          // Delay of 30 seconds.
  }
}

void loop()
{
  float temp = dht.readTemperature();                                    // Reads temperature (°C) from DHT Sensor.
  float hum = dht.readHumidity();                                        // Reads humidity (%) from DHT Sensor. 
  long phSum = 0;                                                        // Initializing phSum as zero.
  for (int i = 0; i < 20; i++)                                           // Reads the pH sensor analog value 20 times.
  {
  phSum += analogRead(PHPIN);                                            // Adds each reading to phSum for averaging later.
  delay(10);                                                             // Delay of 20 samples * 10 ms = 200 ms.
  }
  
  float phADC = phSum / 20.0;                                            // Calculates the average raw ADC value.
  float phVoltage = phADC * 3.3 / 4095.0;                                // Converts the ADC value to voltage, 3.3 is the ESP32 ADC Vref and from 0 to 4095 (12‑bit ADC).
  float calibrationOffset = -8.5;                                        // This is unique to sensor and must be determined experimentally.
  float pH = 7 + ((2.5 - phVoltage) / 0.18) + calibrationOffset;         // pH conversion formula: 2.5 to voltage at pH 7, 0.18 to voltage change per pH unit and +calibrationOffset to adjusts specific probe.


  int turbADC = analogRead(TURBIDITYPIN);                                // Reads the raw ADC value from the Turbidity Sensor.
  float turbVoltage = turbADC * (3.3 / 4095.0);                          // Converts the ADC value to voltage, 3.3 is the ESP32 ADC Vref and from 0 to 4095 (12‑bit ADC). 
  
  float ntu = 3000 - (turbVoltage * 1000);                               // Turbidity conversion formula: 0 V is 3000 NTU and 3.0 V is 0 NTU.
  if (ntu < 0)
    ntu = 0;                                                             // Prevent negative turbidity readings
  if (pH<0)
    pH=0;                                                                // Prevent negative pH readings

  Serial.println("================================");                    // Prints a separator line to indicate Serial Monitor output.
  Serial.print("Temperature : ");                                        // Prints "Temperature : " to the Serial Monitor.
  Serial.print(temp);                                                    // Prints Temperature value from DHT Sensor on Serial Monitor.
  Serial.println(" C");                                                  // Prints Temperature value with "C" on Serial Monitor.

  Serial.print("Humidity    : ");                                        // Prints "Humidity : " to the Serial Monitor.
  Serial.print(hum);                                                     // Prints Humidity value from DHT Sensor on Serial Monitor.
  Serial.println(" %");                                                  // Prints Humidity value with "%" on Serial Monitor.

  Serial.print("pH Value    : ");                                        // Prints "pH Value : " to the Serial Monitor.
  Serial.println(pH);                                                    // Prints PH value from PH Sensor on Serial Monitor.

  Serial.print("Turbidity   : ");                                        // Prints "Turbidity : " to the Serial Monitor.
  Serial.print(ntu);                                                     // Prints Turbidity value from Turbidity Sensor on Serial Monitor.
  Serial.println(" NTU");                                                // Prints Turbidity value with "NTU" on Serial Monitor

  lcd.clear();                                                           // Clears any leftover text from previous runs. 

  lcd.setCursor(0, 0);                                                   // Set Cursor position to first column,first row.
  lcd.print("T:");                                                       // Prints "T: " on LCD.
  lcd.print(temp, 1);                                                    // Prints 1 decimal place of Temperature on LCD.
  lcd.print("C");                                                        // Prints "C" on LCD.

  lcd.setCursor(9, 0);                                                   // Set Cursor position to tenth column,first row.
  lcd.print("H:");                                                       // Print "H: " on LCD.
  lcd.print(hum, 0);                                                     // Print 0 decimal place of Humidity on LCD.

  lcd.setCursor(0, 1);                                                   // Set Cursor position to first column,second row.
  lcd.print("pH:");                                                      // Prints "pH: " on LCD.
  lcd.print(pH, 2);                                                      // Prints 2 decimal place of PH on LCD.

  delay(3000);                                                           // Delay of 30 seconds.
  lcd.clear();                                                           // Clears any leftover text from previous runs. 

  lcd.setCursor(0, 0);                                                   // Set Cursor position to first column,first row.
  lcd.print("Turb:");                                                    // Prints "Turb: " on LCD.
  lcd.print(ntu, 0);                                                     // Print 0 decimal place of Turbidity on LCD.

  lcd.setCursor(0, 1);                                                   // Set Cursor position to first column,second row.
  if (ntu < 500)                                                         // Check the condition if NTU<500.
  {
    lcd.print("Water Clear");                                            // Prints "Water Clear" on LCD if true.
  }
  else
  {
    lcd.print("Water Dirty");                                            // Prints "Water Dirty" on LCD if false.
  }
  delay(2000);                                                           // Delay of 20 seconds.
  lcd.clear();                                                           // Clears any leftover text from previous runs.

  if ((pH >= 5.5 && pH <= 8.5 ) && ntu<500)                              // Checks if: PH is in between 5.5 to 8.5 and Turbidity is below 500.
  { 
    lcd.setCursor(0, 0);                                                 // Set Cursor position to first column,first row.                                              
    lcd.print("Water safe");                                             // Prints "Water safe" on LCD. 
    Serial.print("Water safe");                                          // Prints "Water safe" to the Serial Monitor.
    digitalWrite(BUZZER, LOW);                                           // Turns buzzer OFF.
  }
  else                                                                   // If false.
  {
    lcd.setCursor(0, 0);                                                 // Set Cursor position to first column,first row. 
    lcd.print("Water unsafe");                                           // Prints "Water unsafe" on LCD. 
    Serial.print("Water unsafe");                                        // Prints "Water unsafe" to the Serial Monitor.
    digitalWrite(BUZZER, HIGH );                                         // Turns buzzer ON.
  }
  delay(500);                                                            // Delay of 5 seconds.

  if (WiFi.status() == WL_CONNECTED)                                     // Checks if the ESP32 is still connected to Wi‑Fi before attempting to send data.
  {
    ThingSpeak.setField(1, temp);                                        // Assigns DHT Sensor readings to ThingSpeak fields 1.
    ThingSpeak.setField(2, hum);                                         // Assigns DHT Sensor readings to ThingSpeak fields 2.
    ThingSpeak.setField(3, pH);                                          // Assigns PH Sensor readings to ThingSpeak fields 3.
    ThingSpeak.setField(4, ntu);                                         // Assigns Turbidity Sensor readings to ThingSpeak fields 4.

    int response = ThingSpeak.writeFields(channelID, writeAPIKey);       // Sends all fields to ThingSpeak channel in one HTTP requests: 200 for Success otherwise failure.

    Serial.print("\nThingSpeak Response : ");                            // Prints "ThinkSpeak Response : " to the Serial Monitor.
    Serial.println(response);                                            // Prints the HTTP response code for debugging.
  }
  else                                                                   // If Wi‑Fi is disconnected.
  {
    Serial.println("\nWiFi Disconnected");                               // Prints "WiFi Disconnected" to the Serial Monitor.
  }
  delay(20000);                                                          // Delay of 20 seconds.
}
