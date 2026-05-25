#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

const char *ssid = "Ggggg";
const char *password = "";
const int CAGE_ID = 1;

const char *mqtt_server = "10.50.57.207";

WiFiClient espClient;
PubSubClient client(espClient);

#define DHTPIN 4
#define DHTTYPE DHT22
#define READ_INTERVAL 15000UL

DHT dht(DHTPIN, DHTTYPE);

enum State
{
  STATE_IDLE,
  STATE_READING,
  STATE_SUCCESS,
  STATE_ERROR
};

State currentState = STATE_IDLE;

unsigned long previousMillis = 0;

float suhu = 0;
float kelembapan = 0;

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Connecting MQTT...");
    if (client.connect("ESP32Client"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

  setup_wifi();

  client.setServer(mqtt_server, 1883);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  switch (currentState)
  {

  case STATE_IDLE:
    if (currentMillis - previousMillis >= READ_INTERVAL)
    {
      previousMillis = currentMillis;
      currentState = STATE_READING;
    }
    break;

  case STATE_READING:
    suhu = dht.readTemperature();
    kelembapan = dht.readHumidity();

    if (isnan(suhu) || isnan(kelembapan))
    {
      currentState = STATE_ERROR;
    }
    else
    {
      currentState = STATE_SUCCESS;
    }
    break;

  case STATE_SUCCESS:
  {
    Serial.println("Kirim ke MQTT...");

    String payload = "{";
    payload += "\"suhu\":" + String(suhu) + ",";
    payload += "\"kelembapan\":" + String(kelembapan);
    payload += "}";
    String topic = "cages/" + String(CAGE_ID) + "/environment";

    client.publish(topic.c_str(), payload.c_str());

    Serial.println(payload);

    currentState = STATE_IDLE;
    break;
  }

  case STATE_ERROR:
    Serial.println("Gagal membaca sensor!");
    currentState = STATE_IDLE;
    break;
  }
}
