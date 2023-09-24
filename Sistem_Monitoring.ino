#include <WiFiManager.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <MQUnifiedsensor.h>
#include <EEPROM.h>

// Pin Definitions
#define LED_PIN 2
#define TRIGGER_PIN 0
#define DHT_PIN 33
#define DSM_PIN 35
#define MQ_PIN 32

// Constants
#define DHT_TYPE DHT22
#define RATIO_CLEANAIR_MQ7 (27.5)
#define DSM_LOW_RATIO_MULTIPLIER (30.0 / 1000.0)
#define DSM_INTERVAL (1000)          // DSM501a interval pembacaan
#define TS_INTERVAL (20000)          // ThingSpeak interval kirim data
#define CHANNEL_ID_EEPROM_ADDR 0     // Alamat EEPROM untuk menyimpan Channel ID
#define WRITE_API_KEY_EEPROM_ADDR 10 // Alamat EEPROM untuk menyimpan Write API Key

// ThingSpeak Settings
char channelID[21];
char writeAPIKey[41];

WiFiManager wm;

WiFiManagerParameter channel_id_param;
WiFiManagerParameter api_key_param;
WiFiClient client;

DHT dht(DHT_PIN, DHT_TYPE);
// MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ_PIN, "MQ-7");
MQUnifiedsensor MQ7("ESP-32", 3.3, 12, MQ_PIN, "MQ-7");

float mq7Value = 0;
float humi = 0;
float tempC = 0;
float tempF = 0;
float dsm_consentrate = 0;
float dsm_particle = 0;
unsigned long dsm_lowPulse = 0;
unsigned long dsm_previousTime = 0;
unsigned long ts_previousTime = 0;

void setup()
{
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  loadFromEEPROM();
  delay(3000);
  Serial.println("Starting");

  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT);

  configureWiFiManager();
  configureDHT();
  configureMQ7();
  configureDSM501a();
  configureThingSpeak();
}

void configureWiFiManager()
{
  // Setup WiFi Manager di sini
  new (&channel_id_param) WiFiManagerParameter("channelid", "ThingSpeak Channel ID", channelID, 20);
  new (&api_key_param) WiFiManagerParameter("apikey", "ThingSpeak API Key", writeAPIKey, 40);

  wm.addParameter(&channel_id_param);
  wm.addParameter(&api_key_param);

  wm.setConfigPortalBlocking(false);
  wm.setSaveParamsCallback(saveParamCallback);
  std::vector<const char *> menu = {"wifi", "info", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);
  wm.setClass("invert");

  if (wm.autoConnect())
  {
    Serial.println("Connected to WiFi");
  }
  else
  {
    Serial.println("Config portal running");
  }
}

void configureDHT()
{
  // Setup DHT22 di sini
  dht.begin();
}

void configureMQ7()
{
  // Setup MQ-7 di sini
  // Set math model to calculate the PPM concentration and the value of constants
  MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ7.setA(99.042);           // Configure the equation to calculate CO concentration value
  MQ7.setB(-1.518);           // Configure the equation to calculate CO concentration value
  MQ7.init();

  Serial.print("Calibrating sensor, please wait");
  float calcR0 = 0;
  for (int i = 0; i < 10; i++)
  {
    MQ7.update();
    calcR0 += MQ7.calibrate(RATIO_CLEANAIR_MQ7);
    Serial.print(".");
    delay(1000);
  }
  MQ7.setR0(calcR0 / 10);
  Serial.println(" done!");

  if (isinf(calcR0) || calcR0 == 0)
  {
    Serial.println("Warning: Connection issue with MQ7 sensor. Please check wiring and supply.");
    while (1)
      ;
  }
}

void configureDSM501a()
{
  // Setup DSM501a di sini
  pinMode(DSM_PIN, INPUT);
  dsm_previousTime = millis();
}

void configureThingSpeak()
{
  // Setup ThingSpeak di sini
  ThingSpeak.begin(client);
}

void saveToEEPROM()
{
  EEPROM.put(CHANNEL_ID_EEPROM_ADDR, channelID);
  EEPROM.put(WRITE_API_KEY_EEPROM_ADDR, writeAPIKey);
  EEPROM.commit();
}

void loadFromEEPROM()
{
  EEPROM.get(CHANNEL_ID_EEPROM_ADDR, channelID);
  EEPROM.get(WRITE_API_KEY_EEPROM_ADDR, writeAPIKey);
}

void saveParamCallback()
{
  strcpy(channelID, channel_id_param.getValue());
  strcpy(writeAPIKey, api_key_param.getValue());

  // Save to EEPROM
  saveToEEPROM();
}

float getParticlemgm3(float r)
{
  /*
   * with data sheet...regression function is
   *    y=0.1776*x^3-2.24*x^2+ 94.003*x
   */
  // https://github.com/R2D2-2019/R2D2-2019/wiki/Is-the-given-formula-for-calculating-the-mg-m3-for-the-dust-sensor-dsm501a-correct%3F

  float mgm3 = 0.001915 * pow(r, 2) + 0.09522 * r - 0.04884;
  return mgm3 < 0.0 ? 0.0 : mgm3;
}

void loop()
{
  digitalWrite(LED_PIN, HIGH);
  delay(100);

  int currentWiFiStatus = WiFi.status();
  static int lastWiFiStatus = WL_IDLE_STATUS;
  if (currentWiFiStatus != lastWiFiStatus)
  {
    lastWiFiStatus = currentWiFiStatus;
    if (currentWiFiStatus != WL_CONNECTED)
    {
      Serial.println("WiFi disconnected. Reconnecting...");
      wm.startConfigPortal("OnDemandAP");
    }
  }

  // WiFi Manager
  wm.process();

  // DSM501a
  dsm_lowPulse += pulseIn(DSM_PIN, LOW);

  if ((millis() - dsm_previousTime) > DSM_INTERVAL)
  {
    Serial.println("============================");
    float lowRatio = (dsm_lowPulse * DSM_LOW_RATIO_MULTIPLIER) / DSM_INTERVAL;
    dsm_consentrate = 1.1 * pow(lowRatio, 3) - 3.8 * pow(lowRatio, 2) + 520.0 * lowRatio + 0.62; // using spec sheet curve
    dsm_particle = getParticlemgm3(lowRatio);

    Serial.print("Air Quality         : ");
    if (dsm_consentrate < 1000)
    {
      Serial.println("Air is Clean");
    }
    else if (dsm_consentrate < 10000)
    {
      Serial.println("Air is Good");
    }
    else if (dsm_consentrate < 20000)
    {
      Serial.println("Air is Moderate");
    }
    else if (dsm_consentrate < 50000)
    {
      Serial.println("Air is Heavy");
    }
    else
    {
      Serial.println("Air is Hazardous");
    }

    Serial.print("Concentration PM2.5 : ");
    Serial.print(dsm_consentrate);
    Serial.println(" ppm");

    Serial.print("Particles PM2.5     : ");
    Serial.print(dsm_particle);
    Serial.println(" μg/m³");

    dsm_lowPulse = 0;

    // MQ-7
    MQ7.update();
    mq7Value = MQ7.readSensor();
    Serial.print("Carbon monoxide     : ");
    Serial.print(mq7Value);
    Serial.println(" ppm");

    // DHT-22
    humi = dht.readHumidity();
    tempC = dht.readTemperature();
    tempF = dht.readTemperature(true);

    if (isnan(tempC) || isnan(tempF) || isnan(humi))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      Serial.print("Humidity            : ");
      Serial.print(humi);
      Serial.println("%");
      Serial.print("Temperature         : ");
      Serial.print(tempC);
      Serial.print("°C  ~  ");
      Serial.print(tempF);
      Serial.println("°F");
    }

    dsm_previousTime = millis();
    Serial.println("============================");
  }

  // ThingSpeak
  if ((millis() - ts_previousTime) > TS_INTERVAL)
  {
    Serial.println("============================");
    ts_previousTime = millis();
    ThingSpeak.setField(1, tempC);
    ThingSpeak.setField(2, tempF);
    ThingSpeak.setField(3, humi);
    ThingSpeak.setField(4, mq7Value);
    ThingSpeak.setField(5, dsm_consentrate);
    ThingSpeak.setField(6, dsm_particle);

    bool sentSuccessfully = false;
    int attempts = 0;
    while (!sentSuccessfully && attempts < 3)
    {
      int responseCode = ThingSpeak.writeFields(String(channelID).toInt(), writeAPIKey);
      if (responseCode == 200)
      {
        sentSuccessfully = true;
        Serial.println("Channel update successful.");
      }
      else
      {
        Serial.println("Problem updating channel. HTTP error code " + String(responseCode));
        attempts++;
        delay(1000);
      }
    }

    if (!sentSuccessfully)
    {
      Serial.println("Failed to send data to ThingSpeak after several attempts.");
    }

    Serial.println("============================");
  }

  digitalWrite(LED_PIN, LOW);
  delay(100);
}
