#include <MQUnifiedsensor.h>
/************************Hardware Related Macros************************************/
#define BOARD ("ESP-32") // Wemos ESP-32 or other board, whatever have ESP32 core.
#define PIN (32)         // check the esp32-wroom-32d.jpg image on ESP32 folder

/***********************Software Related Macros************************************/
#define TYPE ("MQ-7")             // MQ7 or other MQ Sensor, if change this verify your a and b values.
#define VOLTAGE_RESOLUTION (3.3)  // 3V3 <- IMPORTANT. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define ADC_BIT_RESOLUTION (12)   // ESP-32 bit resolution. Source: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
#define RATIO_MQ7_CLEANAIR (27.5) // RS / R0 = 27.5 ppm
/*****************************Globals***********************************************/
MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, PIN, TYPE);
/*****************************Globals***********************************************/
float mq7Value = 0;
unsigned long starttime;
unsigned long sampletime_ms = 30 * 1000; // Smaple time 30s

void setup()
{

  // Init the serial port communication - to debug the library
  Serial.begin(115200); // Init serial port
  delay(10);

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

  /*****************************  MQ Init ********************************************/
  // Remarks: Configure the pin of arduino as input.
  /************************************************************************************/
  MQ7.init();

  /*
    //If the RL value is different from 10K please assign your RL value with the following method:
    MQ7.setRL(10);
  */
  /*****************************  MQ CAlibration ********************************************/
  // Explanation:
  // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrating please wait.");
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
  MQ7.serialDebug(true); // uncomment if you want to print the table on the serial port
}

void loop()
{
  if ((millis() - starttime) > sampletime_ms)
  {
    Serial.println("============================");
    // MQ7.serialDebug();           // Will print the table on the serial port
    // MQ-7
    MQ7.update();
    mq7Value = MQ7.readSensor();
    Serial.print("Carbon monoxide     : ");
    Serial.print(mq7Value);
    Serial.println(" ppm");
    Serial.println("============================");
    starttime = millis();
  }
}