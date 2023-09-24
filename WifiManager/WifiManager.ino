#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <SPIFFS.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

// define your default values here, if there are different values in config.json, they are overwritten.

// length should be max size + 1
char ts_channel[10];
char ts_writeapi[20];
char tg_token[50];
char tg_userid[12];

WiFiManagerParameter custom_ts_channel, custom_ts_writeapi, custom_tg_token, custom_tg_userid;

WiFiManager wifiManager;

// callback notifying us of the need to save config
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
    Serial.println(tg_userid);
}

void setup()
{
    // letakkan kode penyiapan Anda di sini, untuk dijalankan satu kali:
    Serial.begin(115200);
    Serial.println();

    // clean FS, for testing
    // SPIFFS.format();

    // baca konfigurasi dari FS json
    Serial.println("mounting file system...");
    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        loadFromSPIFFS();
    }
    else
    {
        Serial.println("failed to mount file system");
    }

    // Parameter tambahan yang akan dikonfigurasi (dapat berupa global atau hanya dalam penyiapan)
    // Setelah tersambung, parameter.getValue() akan memberi Anda nilai yang dikonfigurasi
    // id/nama penampung tempat/ketentuan panjang default
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

    // set minimu quality of signal so it ignores AP's under that quality
    // defaults to 8%
    wifiManager.setMinimumSignalQuality();

    // sets timeout until configuration portal gets turned off
    // useful to make it all retry or go to sleep
    // in seconds
    // wifiManager.setTimeout(120);

    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("Kualitas Udara"))
    {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        // reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("coneected... yeey :)");
}

void loop()
{
    // put your main code here, to run repeatedly:
}
