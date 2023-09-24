#include <WiFi.h>
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

char ssid[] = "Kamar Belakang"; // your network SSID (name)
char pass[] = "12345Rindu";     // your network password
WiFiClient client;

unsigned long myChannelNumber = 2276425;
const char *myWriteAPIKey = "7QEY2GF4AUY6UFID";

// Initialize our values
int number1 = 0;
int number2 = random(0, 100);
int number3 = random(0, 100);
int number4 = random(0, 100);
String myStatus = "";

void setup()
{
    Serial.begin(115200); // Initialize serial
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for Leonardo native USB port only
    }

    WiFi.mode(WIFI_STA);
    ThingSpeak.begin(client); // Initialize ThingSpeak
}

void loop()
{

    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println("Kamar Belakang");
        while (WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
            Serial.print(".");
            delay(5000);
        }
        Serial.println("\nConnected.");
    }

    // set the fields with the values
    ThingSpeak.setField(1, number1);
    ThingSpeak.setField(2, number2);
    ThingSpeak.setField(3, number3);
    ThingSpeak.setField(4, number4);

    // figure out the status message
    if (number1 > number2)
    {
        myStatus = String("field1 is greater than field2");
    }
    else if (number1 < number2)
    {
        myStatus = String("field1 is less than field2");
    }
    else
    {
        myStatus = String("field1 equals field2");
    }

    // set the status
    ThingSpeak.setStatus(myStatus);

    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200)
    {
        Serial.println("Channel update successful.");
    }
    else
    {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
    }

    // change the values
    number1++;
    if (number1 > 99)
    {
        number1 = 0;
    }
    number2 = random(0, 100);
    number3 = random(0, 100);
    number4 = random(0, 100);

    delay(20000); // Wait 20 seconds to update the channel again
}
