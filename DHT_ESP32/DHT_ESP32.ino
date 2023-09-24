#include "DHT.h"

#define DHT_PIN 33
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
float humi = 0;
float tempC = 0;

unsigned long starttime;
unsigned long sampletime_ms = 5 * 1000; // Smaple time 30s

void setup()
{
  Serial.begin(115200);

  // Setup DHT22 di sini
  dht.begin();
}

void loop()
{
  if ((millis() - starttime) > sampletime_ms)
  {
    Serial.println("============================");
    // DHT-22
    humi = dht.readHumidity();
    tempC = dht.readTemperature();

    if (isnan(tempC) || isnan(humi))
    {
      Serial.println("Gagal membaca data Sensor DHT22");
    }
    else
    {
      Serial.print("Relative Humidity   : ");
      Serial.print(humi);
      Serial.println("%");
      Serial.print("Temperature         : ");
      Serial.print(tempC);
      Serial.println("°C");
    }
    Serial.println("============================");

    starttime = millis();
  }
}
