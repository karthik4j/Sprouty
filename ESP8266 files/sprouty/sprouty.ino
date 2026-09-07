#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>


// Wi-Fi CONFIGURATION
const char* WIFI_SSID = "x";
const char* WIFI_PASSWORD = "x";

// HiveMQ CLOUD CONFIGURATION
// You can find these credentials by looking at the HiveMQ dashboard.
const char* MQTT_HOST ="x";
const int MQTT_PORT = 8883;

const char* MQTT_USERNAME = "x";
const char* MQTT_PASSWORD = "x";

// MQTT TOPICS
// ------------------------------------------------------------------------ MQTT topics for sending sensor data -----------------------------------------------------------------------
const char* TOPIC_TEMPERATURE = "college-iot/device01/sensors/temperature";

const char* TOPIC_HUMIDITY = "college-iot/device01/sensors/humidity";

const char* TOPIC_SOIL_MOISTURE = "college-iot/device01/sensors/soil-moisture";

const char* TOPIC_SOIL_CONDITION = "college-iot/device01/sensors/soil-condition";

// ------------------------------------------------------------------------ Automatic irrigation permission topics-----------------------------------------------------------------------
const char* TOPIC_RELAY = "college-iot/device01/actuators/relay";

// Manual irrigation command
const char* TOPIC_IRRIGATION_CONTROL = "college-iot/device01/irrigation/control";

// ------------------------------------------------------------------------ // State feedback topics-----------------------------------------------------------------------

const char* TOPIC_IRRIGATION_STATUS = "college-iot/device01/irrigation/status";

const char* TOPIC_PERMISSION_STATUS = "college-iot/device01/irrigation/permission";

// PIN DEFINITIONS for ESP8266 
#define RELAY_PIN 14      // D5
#define SENSOR_PIN A0
#define DHT_PIN 2         // D4
#define LED_PIN 12        // D6

#define DHT_TYPE DHT11

// GLOBAL OBJECTS

DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClientSecure espClient;

PubSubClient mqttClient(espClient);

// GLOBAL VARIABLES


// Moisture threshold.
// If sensor value falls BELOW this value,
// irrigation will be required.

int MOISTURE_THRESHOLD = 255;

// Remote irrigation control
// IRRIGATION CONTROL STATES

// true  = automatic irrigation is allowed
// false = automatic irrigation is disabled
bool remoteState = true;

// Manual pump control
bool manualMode = false;
bool manualPumpState = false;

// Actual pump state
bool pumpState = false;

// FUNCTION: CONTROL WATER PUMP
void controlPump(bool irrigationRequired)
{
    bool newPumpState = false;
    // MANUAL MODE
    if (manualMode)
    {
        newPumpState = manualPumpState;
    }
    // AUTOMATIC MODE    
    else
    {
        if (irrigationRequired && remoteState)
        {
            newPumpState = true;
        }
        else
        {
            newPumpState = false;
        }
    }
    // UPDATE PUMP ONLY IF STATE CHANGED
    if (newPumpState != pumpState)
    {
        pumpState = newPumpState;

        digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
        digitalWrite(LED_PIN, pumpState ? HIGH : LOW);

        if (pumpState)
        {
            Serial.println("IRRIGATION: ON");
            lcd.setCursor(0, 1);
            lcd.print("IRRIGATION: ON ");
        }
        else
        {
            Serial.println("IRRIGATION: OFF");
            lcd.setCursor(0, 1);
            lcd.print("IRRIGATION: OFF");
        }

        // Publish actual pump state
        if (pumpState)
        {
            mqttClient.publish(TOPIC_IRRIGATION_STATUS,"ON",true);
        }
        else
        {
            mqttClient.publish(TOPIC_IRRIGATION_STATUS,"OFF",true);
        }
    }
}

// FUNCTION: GET TEMPERATURE AND HUMIDITY
String getTemperatureHumidity()
{
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity))
    {
        return "DHT22 sensor error";
    }

    String result = "Temperature: ";
    result += String(temperature, 2);
    result += " C, Humidity: ";
    result += String(humidity, 2);
    result += " %";
    return result;
}
// FUNCTION: CONNECT TO WI-FI
void connectWiFi()
{
    Serial.print("Connecting to Wi-Fi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected!");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
}

// FUNCTION: MQTT CALLBACK
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    String message = "";

    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.println("--------------------------------");
    Serial.print("MQTT message received");
    Serial.println();

    Serial.print("Topic: ");
    Serial.println(topic);

    Serial.print("Message: ");
    Serial.println(message);

    // AUTOMATIC IRRIGATION PERMISSION
    if (String(topic) == TOPIC_RELAY)
    {
        if (message == "ENABLE")
        {
            remoteState = true;

            Serial.println("Automatic irrigation ENABLED");

            mqttClient.publish(TOPIC_PERMISSION_STATUS,"ENABLED",true);
        }
        else if (message == "DISABLE")
        {
            remoteState = false;
            Serial.println("Automatic irrigation DISABLED");

            // If manual mode is not active, disabling
            // automatic irrigation should turn the pump off.
            if (!manualMode)
            {
                pumpState = false;

                digitalWrite(RELAY_PIN, LOW);
                digitalWrite(LED_PIN, LOW);

                mqttClient.publish(TOPIC_IRRIGATION_STATUS,"OFF",true);
            }
            mqttClient.publish(TOPIC_PERMISSION_STATUS,"DISABLED",true);
        }
    }

    
    // MANUAL IRRIGATION CONTROL
    else if (String(topic) == TOPIC_IRRIGATION_CONTROL)
    {
        if (message == "ON")
        {
            manualMode = true;
            manualPumpState = true;

            Serial.println("MANUAL IRRIGATION: ON");

            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(LED_PIN, HIGH);

            pumpState = true;

            mqttClient.publish(TOPIC_IRRIGATION_STATUS,"ON",true);
        }

        else if (message == "OFF")
        {
            manualMode = false;
            manualPumpState = false;

            Serial.println("MANUAL IRRIGATION: OFF");

            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(LED_PIN, LOW);

            pumpState = false;

            mqttClient.publish(TOPIC_IRRIGATION_STATUS,"OFF",true);
        }
    }
}


// FUNCTION: CONNECT TO HIVE MQ
void connectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to HiveMQ...");

        String clientID = "WokwiESP8266-";

        clientID += String(random(0xffff), HEX);

        if (mqttClient.connect(clientID.c_str(),MQTT_USERNAME,MQTT_PASSWORD))
        {
            Serial.println("connected!");

            // Subscribe to dashboard control
            mqttClient.subscribe(TOPIC_RELAY);
            mqttClient.subscribe(TOPIC_IRRIGATION_CONTROL);

            Serial.println("Subscribed to irrigation control topic.");

            // Tell Node-RED that ESP32 is online
          mqttClient.publish(TOPIC_PERMISSION_STATUS,remoteState ? "ENABLED" : "DISABLED",true);              
          mqttClient.publish(TOPIC_IRRIGATION_STATUS,remoteState ? "ON" : "OFF");
        }

        else
        {
            Serial.print("Failed, MQTT state = ");
            Serial.println(mqttClient.state());
            Serial.println("Retrying in 5 seconds...");
            delay(5000);
        }
    }
}


// FUNCTION: PUBLISH SENSOR DATA
void publishSensorData(int sensorValue)
{

    //Publish Soil Mositure and State value 
    // Soil moisture
    String moistureMessage = String(sensorValue);
    mqttClient.publish(TOPIC_SOIL_MOISTURE,moistureMessage.c_str());

    // Temperature and humidity
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity))
    {
        String temperatureMessage =
            String(temperature, 2);

        String humidityMessage =
            String(humidity, 2);


        mqttClient.publish(TOPIC_TEMPERATURE,temperatureMessage.c_str());
        Serial.print("Temperature MQTT publish: ");
        mqttClient.publish(TOPIC_HUMIDITY,humidityMessage.c_str());
    }
    else
    {
        Serial.println("DHT22 reading failed.");
    }
}


void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" SMART AGRICULTURE SYSTEM");
    Serial.println(" PRECISION IRRIGATION");
    Serial.println("================================");

    
    pinMode(SENSOR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    // Make sure pump and LED start OFF during BOOT

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    dht.begin();

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Smart Agriculture");
    lcd.setCursor(0, 1);
    lcd.print("System Starting");
    delay(2000);

    lcd.clear();

    connectWiFi();

    
    // TLS CONFIGURATION
    // TLS encryption is still used, but server certificate verification is disabled.
    espClient.setInsecure();

    // CONFIGURE MQTT
    mqttClient.setServer(MQTT_HOST,MQTT_PORT);

    mqttClient.setCallback(mqttCallback);

    // CONNECT TO HIVE MQ
    connectMQTT();

    Serial.println();
    Serial.println("MQTT system ready.");
    Serial.println("================================");
}

void loop()
{
    // ================================================= CHECK WI-FI
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
    }

    // ================================================= // CHECK MQTT
    if (!mqttClient.connected())
    {
        connectMQTT();
    }

    // Process incoming MQTT messages
    mqttClient.loop();

    int sensorValue = analogRead(SENSOR_PIN);

    //initially set to false so that the system does not start by irrigating itself 
    bool irrigationRequired = false;

    if (sensorValue < MOISTURE_THRESHOLD)
    {
        irrigationRequired = true;
    }

    // CONTROL PUMP
    controlPump(irrigationRequired);

    // LCD DISPLAY
    lcd.setCursor(0, 0);
    lcd.print("Moisture:");
    lcd.print(sensorValue);
    lcd.print("    ");


    Serial.println("--------------------------------");
    Serial.print("Raw Soil Sensor Value: ");
    Serial.println(sensorValue);
    Serial.print("Moisture Threshold: ");
    Serial.println(MOISTURE_THRESHOLD);
    Serial.print("Irrigation Required: ");

    if (irrigationRequired)
    {
        Serial.println("YES");
    }
    else
    {
        Serial.println("NO");
    }

    Serial.print("Remote Irrigation: ");

    if (remoteState)
    {
        Serial.println("ENABLED");
    }
    else
    {
        Serial.println("DISABLED");
    }

    Serial.print("Pump State: ");

    if (pumpState)
    {
        Serial.println("ON");
    }
    else
    {
        Serial.println("OFF");
    }

    Serial.println(getTemperatureHumidity());
    // MQTT SENSOR DATA (we call this function to periodically publish the data)
    publishSensorData(sensorValue);
    delay(2000);
}