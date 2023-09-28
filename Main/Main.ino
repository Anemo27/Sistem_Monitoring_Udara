/*===== SPIFFS =====*/
#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
/*===== SPIFFS =====*/

/*===== WiFi Manager =====*/
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
/*===== WiFi Manager =====*/

/*===== Timezone definition =====*/
#include <time.h>
#define timezone "WIB-7" // WIB-7, WITA-8, WIT-9
/*===== Timezone definition =====*/

/*===== ThingSpeak =====*/
#include "ThingSpeak.h"
/*===== ThingSpeak =====*/

/*===== Telegram =====*/
#include <AsyncTelegram2.h>
/*===== Telegram =====*/

/*===== Mq-7 =====*/
#include <MQUnifiedsensor.h>
#define BOARD ("ESP-32")          // Wemos ESP-32 or other board, whatever have ESP32 core.
#define PIN (32)                  // check the esp32-wroom-32d.jpg image on ESP32 folder
#define TYPE ("MQ-7")             // MQ7 or other MQ Sensor, if change this verify your a and b values.
#define VOLTAGE_RESOLUTION (3.3)  // 3V3 <- IMPORTANT. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define ADC_BIT_RESOLUTION (12)   // ESP-32 bit resolution. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define RATIO_MQ7_CLEANAIR (27.5) // RS / R0 = 27.5 ppm
MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, PIN, TYPE);
float mq7Value = 0;
/*===== Mq-7 =====*/

/*===== DHT22 =====*/
#include "DHT.h"

#define DHT_PIN 33
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
float humi = 0;
float tempC = 0;
/*===== DHT22 =====*/

/*===== PIN DEFINE =====*/
#define LED_PIN 2
#define DSM_PM25_PIN 35 // DSM501 Pin 4 of DSM501a
/*===== PIN DEFINE =====*/

/*===== SPIFFS =====*/
char ts_channel[10];
char ts_writeapi[20];
char tg_token[50];
char tg_userid[12];
/*===== SPIFFS =====*/

/*===== WiFi Manager =====*/
WiFiManager wifiManager;
WiFiManagerParameter custom_ts_channel, custom_ts_writeapi, custom_tg_token, custom_tg_userid;
WiFiClient ts_client;
WiFiClientSecure tg_client;
/*===== WiFi Manager =====*/

/*===== Telegram =====*/
AsyncTelegram2 myBot(tg_client);
/*===== Telegram =====*/

/*===== DSM501a =====*/
String coQuality;
String pm25Quality;
unsigned long dsm_lowPulse = 0.0;

float ratioPM25 = 0.0;
float concentrationPM25 = 0.0;
float particlePM25 = 0.0;
/*===== DSM501a =====*/

/*===== Additional global variabel =====*/
unsigned long starttime;
unsigned long sampletime = 30 * 1000; // Smaple time 30s
String message = "";
String concenPM25Quality = "";
String particlePM25Quality = "";
String concenCOQuality = "";
bool alertOn = false;
/*===== Additional global variabel =====*/

/*===== Configuration Function =====*/
void loading(int max = 10)
{
    for (int i = 0; i < max; i++)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
}
void configureSPIFFS()
{
    Serial.println("Mounting file system");
    // loading(5);
    // clean FS, for testing
    // SPIFFS.format();

    // baca konfigurasi dari FS json
    if (SPIFFS.begin())
    {
        Serial.println("Mounted file system");
        loadFromSPIFFS();
    }
    else
    {
        Serial.println("Failed to mount file system");
    }
}
void configureWifiManager()
{
    Serial.println("Configure Wifi Manager");

    new (&custom_ts_channel) WiFiManagerParameter("ts_channel", "TS Channel", ts_channel, 10);
    new (&custom_ts_writeapi) WiFiManagerParameter("ts_writeapi", "TS WriteAPIKey", ts_writeapi, 20);
    new (&custom_tg_token) WiFiManagerParameter("tg_token", "Telegram token", tg_token, 50);
    new (&custom_tg_userid) WiFiManagerParameter("tg_userid", "Telegram User ID", tg_userid, 12);

    // add all your parameters here
    wifiManager.addParameter(&custom_ts_channel);
    wifiManager.addParameter(&custom_ts_writeapi);
    wifiManager.addParameter(&custom_tg_token);
    wifiManager.addParameter(&custom_tg_userid);

    // set config save notify callback
    wifiManager.setSaveParamsCallback(saveParamCallback);

    std::vector<const char *> menu = {"param", "wifi", "sep", "info", "restart", "exit"};
    wifiManager.setMenu(menu);

    // reset settings - for testing
    // wifiManager.resetSettings();

    wifiManager.setMinimumSignalQuality(20);

    if (!wifiManager.autoConnect("KualitasUdara"))
    {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        // reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }
    // if you get here you have connected to the WiFi
    Serial.println("Wifi Conected... yeey :)");
}
void configureTime()
{
    // Sync time with NTP
    Serial.print("Configure NTP");
    configTzTime("WIB-7", "pool.ntp.org", "time.google.com", "time.windows.com");
    time_t now = time(nullptr);
    while (now < 1000)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.printf("Current Time: %s \n", asctime(&timeinfo));
}
void configureThingSpeak()
{
    Serial.println("Configure ThingSpeak");
    ThingSpeak.begin(ts_client); // Initialize ThingSpeak
    // loading();
}
void configureTelegram()
{
    Serial.println("Configuring Telegram...");
    tg_client.setCACert(telegram_cert);
    myBot.setUpdateTime(1000);
    myBot.setTelegramToken(tg_token);

    Serial.print("Test Telegram connection... ");
    myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
    Serial.print("Bot name: @");
    Serial.println(myBot.getBotName());

    time_t now = time(nullptr);
    struct tm t = *localtime(&now);
    char welcome_msg[64];
    strftime(welcome_msg, sizeof(welcome_msg), "Bot started at %X", &t);
    myBot.sendTo(atoll(tg_userid), welcome_msg);
}
void configureDSM501()
{
    Serial.println("Configure DSM501a");
    pinMode(DSM_PM25_PIN, INPUT); // Set DSM_PM25_PIN as input with pull-up resistor

    Serial.print("Warming up please wait.");
    loading(10);
}
void configureDHT22()
{
    Serial.println("Configure DHT22");
    dht.begin();
    // loading();
}
void configureMQ7()
{
    Serial.println("Configure MQ7 please wait.");
    // Set math model to calculate the PPM concentration and the value of constants
    MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
    MQ7.setA(99.042);
    MQ7.setB(-1.518); // Configure the equation to to calculate H2 concentration

    /*
      Exponential regression:
      GAS     | a      | b
      H2      | 69.014  | -1.374
      LPG     | 700000000 | -7.703
      CH4     | 60000000000000 | -10.54
      CO      | 99.042 | -1.518
      Alcohol | 40000000000000000 | -12.35
    */

    MQ7.init();
    float calcR0 = 0;
    for (int i = 1; i <= 10; i++)
    {
        MQ7.update(); // Update data, the arduino will read the voltage from the analog pin
        calcR0 += MQ7.calibrate(RATIO_MQ7_CLEANAIR);
        Serial.print(".");
    }
    MQ7.setR0(calcR0 / 10);
    Serial.println("  done!.");

    if (isinf(calcR0))
    {
        Serial.println("Warning: Masalah koneksi, R0 tidak terbatas (Sirkuit terbuka terdeteksi), periksa kabel dan suplai Anda");
        while (1)
            ;
    }
    if (calcR0 == 0)
    {
        Serial.println("Warning: Masalah koneksi, R0 adalah nol (pin analog pendek ke ground), periksa kabel dan suplai Anda");
        while (1)
            ;
    }
    /*****************************  MQ CAlibration ********************************************/
    // MQ7.serialDebug(true); // uncomment if you want to print the table on the serial port
}
/*===== Configuration Function =====*/

/*===== Additional Function =====*/
void cekWifiConnection()
{
    int currentWiFiStatus = WiFi.status();
    static int lastWiFiStatus = WL_IDLE_STATUS;
    if (currentWiFiStatus != lastWiFiStatus)
    {
        lastWiFiStatus = currentWiFiStatus;
        if (currentWiFiStatus != WL_CONNECTED)
        {
            Serial.println("[WiFi] WiFi disconnected. Reconnecting...");
            wifiManager.startConfigPortal("OnDemandAP");
        }
    }
}
void saveParamCallback()
{
    Serial.println("saving parameters");

    // read updated parameters
    strcpy(ts_channel, custom_ts_channel.getValue());
    strcpy(ts_writeapi, custom_ts_writeapi.getValue());
    strcpy(tg_token, custom_tg_token.getValue());
    strcpy(tg_userid, custom_tg_userid.getValue());

    DynamicJsonDocument json(1024);
    json["ts_channel"] = ts_channel;
    json["ts_writeapi"] = ts_writeapi;
    json["tg_token"] = tg_token;
    json["tg_userid"] = tg_userid;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("failed to open config file for writing");
    }
    else
    {
        Serial.println("parameters saved!");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    // end save
}
void loadFromSPIFFS()
{
    if (SPIFFS.exists("/config.json"))
    {
        // file tersedia, membaca dan memuat
        Serial.println("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile)
        {
            Serial.println("opened config file");
            size_t size = configFile.size();
            // Mengalokasikan buffer untuk menyimpan isi file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonDocument json(1024);
            auto deserializeError = deserializeJson(json, buf.get());
            serializeJson(json, Serial);
            if (!deserializeError)
            {

                Serial.println("\nparsed json");

                strcpy(ts_channel, json["ts_channel"]);
                strcpy(ts_writeapi, json["ts_writeapi"]);
                strcpy(tg_token, json["tg_token"]);
                strcpy(tg_userid, json["tg_userid"]);
            }
            else
            {
                Serial.println("failed to load json config");
            }
        }
        else
        {
            Serial.println("failed to read /config.json");
        }
    }
    else
    {
        Serial.println("tidak ada file /config.json");
    }

    // end read
    Serial.print("ThingSpeak Channel : ");
    Serial.println(ts_channel);
    Serial.print("ThingSpeak WriteAPI: ");
    Serial.println(ts_writeapi);
    Serial.print("Telegram Token     : ");
    Serial.println(tg_token);
    Serial.print("Telegram User ID   : ");
    Serial.println(atoll(tg_userid));
}
float getParticlemgm3(float ratio)
{
    /*
     * with data sheet...regression function is
     *    y=0.1776*x^3-2.24*x^2+ 94.003*x
     */
    // https://github.com/R2D2-2019/R2D2-2019/wiki/Is-the-given-formula-for-calculating-the-mg-m3-for-the-dust-sensor-dsm501a-correct%3F

    float mgm3 = 0.001915 * pow(ratio, 2) + 0.09522 * ratio - 0.04884;
    return mgm3 < 0.0 ? 0.0 : mgm3;
}
void telegramBot()
{
    // Check incoming messages and keep Telegram server connection alive
    TBMessage msg;
    if (myBot.getNewMessage(msg))
    {
        Serial.print("User ");
        Serial.print(msg.sender.username);
        Serial.print(" send this message: ");
        Serial.println(msg.text);

        if (msg.text.indexOf("/status") > -1)
        {
            setStatus();
            Serial.println(message);
            myBot.sendMessage(msg, message);
        }
        else
        {
            myBot.sendMessage(msg, msg.text);
        }
    }
}
void sendThingSpeak()
{
    ThingSpeak.setField(1, tempC);
    ThingSpeak.setField(2, humi);
    ThingSpeak.setField(3, mq7Value);
    ThingSpeak.setField(4, concentrationPM25);
    ThingSpeak.setField(5, (particlePM25 * 1000.0));
    ThingSpeak.setStatus(concenCOQuality);

    bool sentSuccessfully = false;
    int attempts = 0;
    while (!sentSuccessfully && attempts < 3)
    {
        int responseCode = ThingSpeak.writeFields(atoll(ts_channel), ts_writeapi);
        if (responseCode == 200)
        {
            sentSuccessfully = true;
            Serial.println("Channel update successful.");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code " + String(responseCode));
            attempts++;
            delay(3000);
        }
    }

    if (!sentSuccessfully)
    {
        Serial.println("Failed to send data to ThingSpeak after several attempts.");
    }
}
void setStatus()
{
    // Concentrate PM2.5
    if (concentrationPM25 < 1000)
    {
        concenPM25Quality = "Clean";
    }
    else if (concentrationPM25 < 10000)
    {
        concenPM25Quality = "Good";
    }
    else if (concentrationPM25 < 20000)
    {
        concenPM25Quality = "Acceptable";
    }
    else if (concentrationPM25 < 50000)
    {
        concenPM25Quality = "Bad";
    }
    else
    {
        concenPM25Quality = "Hazzard!";
    }

    // Particle PM2.5
    if ((particlePM25 * 1000.0) < 12)
    {
        particlePM25Quality = "Clean";
    }
    else if ((particlePM25 * 1000.0) < 35)
    {
        particlePM25Quality = "Healthy";
    }
    else if ((particlePM25 * 1000.0) < 55)
    {
        particlePM25Quality = "Unhealthy";
    }
    else if ((particlePM25 * 1000.0) < 150)
    {
        particlePM25Quality = "Very Unhealthy";
    }
    else
    {
        particlePM25Quality = "Dangerous!";
    }

    // Concentrate Gas CO
    if (mq7Value < 4)
    {
        concenCOQuality = "Clean";
    }
    else if (mq7Value < 9)
    {
        concenCOQuality = "Healthy";
    }
    else if (mq7Value < 15)
    {
        concenCOQuality = "Unhealthy";
    }
    else if (mq7Value < 30)
    {
        concenCOQuality = "Very Unhealthy";
    }
    else
    {
        concenCOQuality = "Dangerous!";
    }

    message = "";
    message += "\nStatus Kualitas Udara";
    message += "\n_____________________________";
    message += "\nPM2.5 Particle Quality: " + particlePM25Quality;
    message += "\nCO Quality            : " + concenCOQuality;
    message += "\nParticle PM2.5        : " + String(particlePM25) + " mg/m³";
    message += "\nParticle PM2.5        : " + String(particlePM25 * 1000.0) + " μg/m³";
    message += "\nCarbon monoxide       : " + String(mq7Value) + " ppm";
    message += "\nTemperature           : " + String(tempC) + "°C";
    message += "\nHumidity              : " + String(humi) + "%";

    Serial.println(message);
}
void alertSystem()
{
    if ((particlePM25 * 1000.0) >= 25 || mq7Value >= 9 && !alertOn)
    {
        alertOn = true;
        myBot.sendTo(atoll(tg_userid), "Udara dalam keadaan tidak sehat");
        myBot.sendTo(atoll(tg_userid), message);
    }
    if ((particlePM25 * 1000.0) < 25 && mq7Value < 9 && alertOn)
    {
        alertOn = false;
        myBot.sendTo(atoll(tg_userid), "Udara sudah kembali normal");
        myBot.sendTo(atoll(tg_userid), message);
    }
}
/*===== Additional Function =====*/

/*===== Read Sensor Function =====*/
void readDSM501(unsigned long lowPulse)
{
    ratioPM25 = lowPulse / (sampletime * 10.0); // Integer percentage 0=>100

    concentrationPM25 = 1.1 * pow(ratioPM25, 3) - 3.8 * pow(ratioPM25, 2) + 520 * ratioPM25; // using spec sheet curve
    particlePM25 = getParticlemgm3(ratioPM25);
    dsm_lowPulse = 0.0;

    Serial.println("Ratio    : " + String(ratioPM25));
    Serial.println("LowPulse : " + String(lowPulse));
    Serial.println("Concent  : " + String(concentrationPM25));
}
void readMq7()
{
    MQ7.update();
    mq7Value = MQ7.readSensor();
}
void readDHT22()
{
    humi = dht.readHumidity();
    tempC = dht.readTemperature();

    if (isnan(tempC) || isnan(humi))
    {
        Serial.println("Gagal membaca data Sensor DHT22");
    }
}
/*===== Read Sensor Function*/

void setup()
{
    delay(2000);
    pinMode(LED_PIN, OUTPUT);
    // letakkan kode persiapan Anda di sini, untuk dijalankan satu kali:
    Serial.begin(115200);
    Serial.println("Starting Monitoring Kualitas Udara");

    // start konfigure
    configureSPIFFS();
    configureWifiManager();
    configureTime();
    configureThingSpeak();
    configureTelegram();
    configureDSM501();
    configureDHT22();
    configureMQ7();
    starttime = millis();
}
void loop()
{
    // put your main code here, to run repeatedly:
    // cekWifiConnection();
    wifiManager.process();
    telegramBot();
    unsigned long lowPulsee = pulseIn(DSM_PM25_PIN, LOW);
    dsm_lowPulse = dsm_lowPulse + lowPulsee;
    Serial.println("===");
    Serial.println(lowPulsee);
    Serial.println(dsm_lowPulse);
    Serial.println("===");
    if ((millis() - starttime) > sampletime)
    {
        Serial.println("============================");
        readDSM501(dsm_lowPulse);
        starttime = millis();
        readMq7();
        readDHT22();
        setStatus();
        alertSystem();
        sendThingSpeak();
        Serial.println("============================");
    }

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}
