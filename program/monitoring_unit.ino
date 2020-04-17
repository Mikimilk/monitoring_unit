#include <ssl_client.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <Adafruit_TSL2561_U.h>

#define BH1750_ADDR 0x23
#define DHTPIN 23
#define DHTTYPE DHT22
#define SOILV1PIN 34

char ssid[] = "milk-note"; //Your wifi's ssid
const char* password =  "milkyway"; //Your wifi's password

const char* mqtt_server = "192.168.137.161";

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);


void configureSensor(void)
{
  tsl.enableAutoRange(true);          
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      
}

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

void callback(char* topic, byte* message, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print(". Message: ");
  String messageTemp;
 
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void setup_wifi() {
  //connect WiFi
  Serial.println("Connecting to wifi ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void reconnect() {
  while (!client.connected()) { 
    Serial.println("Connecting to MQTT...");
    WiFi.mode(WIFI_STA); //Attempt to connect
    if (client.connect("ESP32D_unit1" )) { 
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("failed with state connecting to MQTT");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(10);
  setup_wifi();
  
  dht.begin();

  tsl.begin();
  configureSensor();

  
  //connect to mqtt broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

 
void loop() {

  if ((WiFi.status() != WL_CONNECTED)){
      Serial.println("Connection lost");
  }
  
  if (!client.connected()) {
    reconnect();
  }
 
  if(!client.loop())
    client.connect("ESP32D_unit1"); //Declare ESP32D_unitx ,x is number of each node

  now = millis();

  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    
    // Read humidity
    float h = dht.readHumidity();
    
    // Read temperature as Celsius 
    float t = dht.readTemperature();
    
    // Read light intensity as Lux
    sensors_event_t event;
    tsl.getEvent(&event);
    if (!event.light){
      Serial.println("tsl Sensor overload");
    }
    float l = event.light;

    //Read Soi Moisture Sensor v.1
    float m = map(analogRead(SOILV1PIN),4095,0,0,100);


    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      h = 0;
      t = 0;
    }

    if (isnan(l) || l == 65536) {
      Serial.println("Failed to read from TSL2561 sensor!");
      l = 0;
    }

    if (isnan(m)) {
      Serial.println("Failed to read from Soil Moisture Module V1 Sensor !");
    }
    
    // Computes temperature values in Celsius
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    static char temperatureTemp[7];
    float hic = dht.computeHeatIndex(t, h, false);
    dtostrf(hic, 6, 2, temperatureTemp);
        
    static char lightTemp[7];
    dtostrf(l, 6, 2, lightTemp);

    static char soilTemp[7];
    dtostrf(m, 6, 2, soilTemp);
    

    // Publishes Temperature and Humidity values
    client.publish("room/humidity", humidityTemp);
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/light_intensity", lightTemp);
    client.publish("room/soil", soilTemp);
//    client.publish("room/soil2", soilTemp2);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(" Temperature (Heat index): ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.println();
    Serial.print("Light is ");
    Serial.print(l);
    Serial.println(" lx.");
    Serial.print("Soil Moisture (V1) is ");
    Serial.println(m);
    Serial.println("---------------------------------------------------------");
  }
} 
