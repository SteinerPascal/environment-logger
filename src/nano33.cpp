 
  /*
 * This code is supposed to run on an Arduino Nano 33 BLE Sense.
 * It makes full use of all sensors on that device.
 * 
 * It reads atmospheric parameters such as:
 * - temperature
 * - pressure
 * - humidity
 * 
 * It reads other environmental parameters such as light intensity.
 * 
 * 
 * Finally, accelerometer and gyroscope data are recorded, along with magnetic field data.
 * 
 * All these measurements are taken several times per second. Individual samples are
 * averaged over one second, then are sent via USB/serial out.
 * 
 * That's all this code does. Take all measurements from all sensors
 * as fast as possible, and send those out (one line = one batch of measurements).
 * 
 * It's up to the receiver side to parse, interpret, and store that data.
 * 
 * Care was taken to prevent system freeze
 * by allowing brief pauses between sensor reads.
 * 
 * Probably not all sensor reads need to be braketed by delay().
 * But it's time-consuming to figure out which need delay().
 * It's probably easier to intersperse delay() among all sensor reads
 * and just tweak the value of the delay() parameter.
 * A minimmum of one week of uninterrupted work is considered the gold standard
 * for stability.
 * 
 * TBD: In the future some kind of watchdog needs to be implemented
 * since not all lock-up situations can be avoided with code shims such as delay().
 * 
 * A very solid code that doesn't seem to need a watchdog would be
 * a solid base for everything else. A watchdog would then be more like insurance.
 */

// HTS221 sensor library / humidity / temperature
// https://www.arduino.cc/en/Reference/ArduinoHTS221
// https://github.com/arduino-libraries/Arduino_HTS221
#include <Arduino_HTS221.h>
// LPS22HB sensor library / pressure
// https://www.arduino.cc/en/Reference/ArduinoLPS22HB
// https://github.com/arduino-libraries/Arduino_LPS22HB
#include <Arduino_LPS22HB.h>
// LSM9DS1 sensor library
// https://www.arduino.cc/en/Reference/ArduinoLSM9DS1
// https://github.com/arduino-libraries/Arduino_LSM9DS1
//#include <Arduino_LSM9DS1.h> / light
// APDS9960 sensor library
// https://github.com/arduino-libraries/Arduino_APDS9960
#include <Arduino_APDS9960.h>

// store readings from sensors
float temperature, humidity, pressure;
// store readings from light sensor
int r, g, b, c;

// line output to serial
char linebuf_all[200];

// Nr of retries for light sensor to become available
#define APDS_MAX 8

// short pause between sensor reads
short srelax = 2;
// slow down sampling loop
short endrelax = 100;

int ledState = LOW;

// watchdog timeout in seconds
// 6 seconds can be shorter than sketch upload time
// so uploads will fail when this sketch is loaded
// tap the Reset button twice quickly
// and the LED will start pulsing slowly
// then do the sketch upload
int wdt = 6;



void setup() {
  Serial.begin(9600); 

  // configure watchdog
  NRF_WDT->CONFIG         = 0x01;                  // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV            = int(wdt * 32768 + 1);  // CRV = timeout * 32768 + 1
  NRF_WDT->RREN           = 0x01;                  // Enable the RR[0] reload register
  NRF_WDT->TASKS_START    = 1;                     // Start WDT
  delay(srelax);

  // temperature and humidity
  HTS.begin();
  delay(srelax);

  // pressure
  BARO.begin();
  delay(srelax);
  // The baro sensor reads wrong first time after init
  // so let's do a throw-away read here.
  pressure = BARO.readPressure(MILLIBAR);
  delay(srelax);

  // light sensor
  APDS.begin();
  delay(srelax);

  pinMode(LED_BUILTIN, OUTPUT);

  // Let's allow things to settle down.
  delay(srelax);
}

void loop() {
  // Reload the WDTs RR[0] reload register
  // this will reset the watchdog every loop
  // as long as all goes well
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;

  // constrain the APDS readiness loop
  short apds_loop = 0;

  // always check if sensor is available before reading from it.
  while (!APDS.colorAvailable() && apds_loop <= APDS_MAX) {
    // always wait a bit after APDS.colorAvailable()
    delay(200);
    apds_loop++;
  }
  if(apds_loop <= APDS_MAX) {
    APDS.readColor(r, g, b, c);
    delay(srelax);
  } 
  temperature = HTS.readTemperature();
  delay(srelax);
  humidity = HTS.readHumidity();
  delay(srelax);
  pressure = BARO.readPressure(MILLIBAR);
  delay(srelax);

  // prepare the line output with all data
    sprintf(linebuf_all,
    "t-h-p,%.2f,%.1f,%.2f,l,%u,%u,%u,%u",
    temperature, humidity, pressure,
    r, g, b, c);
  // send data out
  Serial.println(linebuf_all);
  
  // blink the LED every cycle
  // (heartbeat indicator)
  ledState = ledState ? LOW: HIGH;
  digitalWrite(LED_BUILTIN,  ledState);

  delay(endrelax);
  }
