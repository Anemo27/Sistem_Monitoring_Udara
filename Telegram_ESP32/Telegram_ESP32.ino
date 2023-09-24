#include <AsyncTelegram2.h>

// Timezone definition
#include <time.h>
#define timezone 7 // WIB = 7, WITA = 8, WIT = 9

#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure client;

AsyncTelegram2 myBot(client);
const char *ssid = "Kamar Belakang";  // SSID WiFi network
const char *pass = "12345Rindu";      // Password  WiFi network
const char *token = "TOKEN-TELEGRAM"; // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 3241658769;

#define BUTTON 0

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  // initialize the Serial
  Serial.begin(115200);

  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  // connects to the access point
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\nConnected.");

  // Sync time with NTP
  configTime(timzone * 3600, "pool.ntp.org", "time.google.com", "time.windows.com");
  client.setCACert(telegram_cert);

  // Set the Telegram bot properties
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  Serial.print("Bot name: @");
  Serial.println(myBot.getBotName());

  time_t now = time(nullptr);
  struct tm t = *localtime(&now);
  char welcome_msg[64];
  strftime(welcome_msg, sizeof(welcome_msg), "Bot started at %X", &t);
  myBot.sendTo(userid, welcome_msg);
}

void loop()
{

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200)
  {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // Check incoming messages and keep Telegram server connection alive
  TBMessage msg;
  if (myBot.getNewMessage(msg))
  {
    Serial.print("User ");
    Serial.print(msg.sender.username);
    Serial.print(" send this message: ");
    Serial.println(msg.text);

    // echo the received message
    myBot.sendMessage(msg, msg.text);
  }

  if (digitalRead(BUTTON) == LOW)
  {
    time_t now = time(nullptr);
    struct tm t = *localtime(&now);
    char msg_buf[64];
    strftime(msg_buf, sizeof(msg_buf), "%X - Button pressed", &t);
    myBot.sendTo(userid, msg_buf);
  }
}
