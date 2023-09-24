#define LED_PIN 2
#define TRIGGER_PIN 0
#define DSM_PM25_PIN 35 // DSM501 Pin 4 of DSM501a

// DSM501a Sensor support
unsigned long dsm_lowPulse = 0;
unsigned long starttime;
unsigned long sampletime_ms = 5 * 1000; // Smaple time 3s

float ratioPM25 = 0;
float concentrationPM25 = 0;

/* DSM501a sensor connection. Please note that someteimes the color wires change...
 * https://www.elecrow.com/download/DSM501.pdf
 * 1 Black  - Not used
 * 2 Red    - Vout2 - 1 microns (PM1.0)
 * 3 White  - Vcc 5V
 * 4 Yellow - Vout1 - 2.5 microns (PM2.5)
 * 5 Orange - GND
 */

// Inisialisasi variabel untuk EMA
float emaConcentrationPM25 = 0.0; // Nilai awal EMA, bisa disesuaikan
float emaparticlePM25 = 0.0;      // Nilai awal EMA, bisa disesuaikan
float alpha = 0.2;                // Koefisien alpha, bisa disesuaikan (biasanya 0.1 - 0.3)

void setup_hardware()
{
    pinMode(DSM_PM25_PIN, INPUT);

    // wait 60s for DSM501 to warm up
    Serial.println("wait 20s for DSM501 to warm up)");
    Serial.print("Warming up please wait.");
    for (int i = 1; i <= 20; i++)
    {
        delay(1000); // 1s
        Serial.print(".");
    }
    Serial.println("  done!.");
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

String setAirQuality(float pm25)
{
    if (pm25 < 1000)
    {
        return "Clean";
    }

    if (pm25 < 10000)
    {
        return "Good";
    }

    if (pm25 < 20000)
    {
        return "Acceptable";
    }

    if (pm25 < 50000)
    {
        return "Bad";
    }
    return "Hazzard!";
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("Starting");

    pinMode(2, OUTPUT);
    pinMode(0, INPUT);

    Serial.println("Setting up the Hardware");
    setup_hardware();
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);

    dsm_lowPulse += pulseIn(DSM_PM25_PIN, LOW);

    if ((millis() - starttime) > sampletime_ms)
    { // Check if we've sampled at least the defined sample time
        Serial.println("============================");
        ratioPM25 = dsm_lowPulse / (sampletime_ms * 10.0); // Integer percentage 0=>100

        concentrationPM25 = 1.1 * pow(ratioPM25, 3) - 3.8 * pow(ratioPM25, 2) + 520 * ratioPM25 + 0.62; // using spec sheet curve
        String airQuality = setAirQuality(concentrationPM25);
        float particlePM25 = getParticlemgm3(ratioPM25);

        // emaConcentrationPM25 = (alpha * concentrationPM25) + ((1 - alpha) * emaConcentrationPM25);
        // emaparticlePM25 = (alpha * particlePM25) + ((1 - alpha) * emaparticlePM25);

        Serial.print("Air Quality         : ");
        Serial.println(airQuality);

        Serial.print("Concentration PM2.5 : ");
        Serial.print(concentrationPM25);
        Serial.println(" ppm");

        // Serial.print("Smoothed Concentration PM2.5 : ");
        // Serial.print(emaConcentrationPM25);
        // Serial.println(" ppm");

        Serial.print("Particles PM2.5     : ");
        Serial.print(particlePM25);
        Serial.println(" μg/m³");

        dsm_lowPulse = 0;
        starttime = millis();
        Serial.println("============================");
    }
    digitalWrite(LED_PIN, LOW);
    delay(300);
}