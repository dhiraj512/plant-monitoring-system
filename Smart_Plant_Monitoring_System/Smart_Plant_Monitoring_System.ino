// Adding libraries
#include <LiquidCrystal_I2C.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// Initialize the LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

char auth[] = ""; // your Blynk Auth token
char ssid[] = ""; // your WIFI SSID
char pass[] = ""; // your WIFI Password

DHT dht(D4, DHT11); // (DHT_sensor_pin, sensor_model)

BlynkTimer timer; // This function creates the timer object.

// Define the sensor pins
#define soil A0      // A0 Soil Moisture Sensor
#define PIR D5       // D5 PIR Motion Sensor
#define BuzzerPin D6 // D6 Led pin
int PIR_ToggleValue; // Variable to store the toggle state of the button

void checkPhysicalButton(); // Function to check the physical button
int relayState = LOW;       // Variable to store the state of the relay
int pushButtonState = HIGH; // Variable to store the state of the button
#define RELAY_PIN_1 D3      // D3 Relay pin
#define PUSH_BUTTON_1 D7    // D7 Push button pin
#define VPIN_BUTTON_1 V12   // V12 Button Widget virtual pin on the Blynk

// Variables to store the sensor values
double T, P;
char status;

void setup()
{
  Serial.begin(9600);                               // Start the Serial communication
  lcd.begin();                                      // Start the LCD display
  lcd.backlight();                                  // Turn on the backlight
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80); // Start the Blynk connection
  dht.begin();                                      // Start the DHT11 sensor

  pinMode(PIR, INPUT);        // PIR as input
  pinMode(BuzzerPin, OUTPUT); // Buzzer as output

  pinMode(RELAY_PIN_1, OUTPUT);          // Relay as output
  digitalWrite(RELAY_PIN_1, LOW);        // Turn off the relay
  pinMode(PUSH_BUTTON_1, INPUT_PULLUP);  // Push button as input
  digitalWrite(RELAY_PIN_1, relayState); // Turn off the relay

  lcd.setCursor(0, 0);           // Set the cursor to the first column and first row
  lcd.print("  Initializing  "); // Print the text on the LCD
  for (int a = 5; a <= 10; a++)
  {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(500);
  }
  lcd.clear();
  lcd.setCursor(11, 1);
  lcd.print("W:OFF");

  timer.setInterval(100L, soilMoistureSensor);  // Call the soil moisture sensor function
  timer.setInterval(100L, DHT11sensor);         // Call the DHT11 sensor function
  timer.setInterval(500L, checkPhysicalButton); // Call the physical button function
}

void DHT11sensor()
// Get the DHT11 sensor values
{
  float h = dht.readHumidity();    // Read humidity
  float t = dht.readTemperature(); // Read temperature as Celsius (the default)

  if (isnan(h) || isnan(t)) // Check if any reads failed and exit early (to try again).
  {
    Serial.println("Failed to read data from DHT sensor!"); // Print error message to serial monitor
    return;
  }
  Blynk.virtualWrite(V0, t); // Send the temperature value to the Blynk app
  Blynk.virtualWrite(V1, h); // Send the humidity value to the Blynk app

  // Print the temperature value on the LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t);

  // Print the humidity value on the LCD
  lcd.setCursor(8, 0);
  lcd.print("H:");
  lcd.print(h);
}

void soilMoistureSensor()
// Get the soil moisture sensor values
{
  int value = analogRead(soil);        // Read the soil moisture sensor value
  value = map(value, 0, 1024, 0, 100); // Convert the value to percentage
  value = (value - 100) * -1;          // Convert the value to percentage

  Blynk.virtualWrite(V3, value); // Send the soil moisture value to the Blynk app
  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(value); // Print the soil moisture value on the LCD
  lcd.print(" ");
}

void PIRsensor()
// Get the PIR sensor values
{
  bool value = digitalRead(PIR); // Read the PIR sensor value
  if (value)                     // If motion detected
  {
    Blynk.logEvent("pirmotion", "WARNNG! Motion Detected!"); // Send the notification to the Blynk app
    WidgetLED LED(V5);
    LED.on();
    tone(BuzzerPin, 1000); // If motion detected Led will on
  }
  else
  {
    WidgetLED LED(V5);
    LED.off();
    noTone(BuzzerPin); // If motion not detected Led will off
    delay(1000);
  }
}

BLYNK_WRITE(V6) // Button Widget is writing to pin V6
{
  PIR_ToggleValue = param.asInt(); // Get the value from the Blynk app
}

BLYNK_CONNECTED() // This function will run every time Blynk connection is established
{
  // Request the latest state from the server
  Blynk.syncVirtual(VPIN_BUTTON_1); // Sync Button Widget
}

BLYNK_WRITE(VPIN_BUTTON_1) // Button Widget is writing to pin V12
{
  relayState = param.asInt();            // Get the value from the Blynk app
  digitalWrite(RELAY_PIN_1, relayState); // Update relay state
}

void checkPhysicalButton()
// Function to check the physical button
{
  if (digitalRead(PUSH_BUTTON_1) == LOW) // If button is pressed
  {
    // pushButtonState is used to avoid sequential toggles
    if (pushButtonState != LOW)
    {
      relayState = !relayState; // Toggle relay state
      digitalWrite(RELAY_PIN_1, relayState);
      Blynk.virtualWrite(VPIN_BUTTON_1, relayState); // Update Button Widget
    }
    pushButtonState = LOW;
  }
  else
  {
    pushButtonState = HIGH;
  }
}

void loop()
{
  if (PIR_ToggleValue == 1)
  {
    lcd.setCursor(5, 1);
    lcd.print("M:ON ");
    PIRsensor();
  }
  else
  {
    lcd.setCursor(5, 1);
    lcd.print("M:OFF");
    WidgetLED LED(V5);
    LED.off();
  }

  if (relayState == LOW)
  {
    lcd.setCursor(11, 1);
    lcd.print("W:ON ");
  }
  else if (relayState == HIGH)
  {
    lcd.setCursor(11, 1);
    lcd.print("W:OFF");
  }

  Blynk.run(); // Run the Blynk library
  timer.run(); // Run the Blynk timer
}