/***************************************************
  Motion Detection PIR 

  The sensor will send a MQTT Message
 ****************************************************/
#include <Arduino.h>
#include <WiFi.h>

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "WiFiCredentials.h"

/************************* Adafruit.io Setup *********************************/
#define LEAK_SENSOR_PIN 2
#define LED_STATUS_PIN LED_BUILTIN

#define SERIALENDCHAR '\n' // Needed for the simulation on Tinker

/************ Global Variables ******************/
boolean debug = true;
int i = 0;
bool status_leak = false;
bool previous_status_leak = false;

unsigned long currentTimer0 = 0;
unsigned long previousTimer0 = 0;
unsigned long interval0 = 10000;

unsigned long currentTimer1 = 0;
unsigned long previousTimer1 = 0;
unsigned long interval1 = 1000;
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, BROKER_IP, SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish leak_status = Adafruit_MQTT_Publish(&mqtt, LEAK_STATUS_FEED);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void publishMsg(Adafruit_MQTT_Publish topic, const char *Msg)
{
  if (debug)
  {
    Serial.print(F("\nSending Topic Value "));
    Serial.print(Msg);
    Serial.print("...");
  }
  if (!topic.publish(Msg))
  {
    if (debug)
    {
      Serial.println(F("Failed"));
    }
  }
  else
  {
    if (debug)
    {
      Serial.println(F("OK!"));
    }
  }
}

void check_status_leak()
{
  status_leak = digitalRead(LEAK_SENSOR_PIN);

  if (status_leak != previous_status_leak)
  {
    previous_status_leak = status_leak;
    if (status_leak)
    {
      digitalWrite(LED_STATUS_PIN, HIGH);
    }
    else
    {
      digitalWrite(LED_STATUS_PIN, LOW);
    }

    if (status_leak)
    {
      publishMsg(leak_status, "No Leak");
    }
    else
    {
      publishMsg(leak_status, "Leak");
    }
  }
}

void MQTT_connect()
{
  // Function to connect and reconnect as necessary to the MQTT server.
  // Should be called in the loop function and it will take care if connecting.
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }

  if (debug)
  {
    Serial.print("Connecting to MQTT... ");
  }

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    if (debug)
    {
      Serial.println(mqtt.connectErrorString(ret));
    }
    if (debug)
    {
      Serial.println("Retrying MQTT connection in 5 seconds...");
    }
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  if (debug)
  {
    Serial.println("MQTT Connected!");
  }
}

void setup()
{

  pinMode(LEAK_SENSOR_PIN, INPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);
  digitalWrite(LEAK_SENSOR_PIN, HIGH);

  Serial.begin(9600);
  delay(10);

  if (debug)
  {
    Serial.println(F("Leak-Sensor Debug"));
    // Connect to WiFi access point.
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
  }

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (debug)
    {
      Serial.print(".");
    }
  }
  if (debug)
  {
    Serial.println();
  }

  if (debug)
  {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop()
{
  /*************************** Connect to MQTT SERVER ************************************/
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  /*************************** Detect Change in leak Sensor  ************************************/
  currentTimer0 = millis();
  currentTimer1 = millis();
  digitalWrite(LEAK_SENSOR_PIN, HIGH);

  if (currentTimer0 - previousTimer0 > interval0)
  {
    previousTimer0 = currentTimer0;
    if (debug)
    {
      Serial.println(F("Timer 0"));
    }
    //Timer 0 sends an alive MQTT Message with the Status of the Sensor
    if (status_leak)
    {
      publishMsg(leak_status, "No Leak");
    }
    else
    {
      publishMsg(leak_status, "Leak");
    }
  }

  if (currentTimer1 - previousTimer1 > interval1)
  {
    previousTimer1 = currentTimer1;
    if (debug)
    {
      Serial.println(F("Timer 1"));
    }
    //Timer 1 checks in a short interval the status of the sensor if it detects a CHANGE
    // it sends an MQTT Message
    check_status_leak();
  }
}