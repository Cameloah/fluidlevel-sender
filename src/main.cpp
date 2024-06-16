#include "main.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include "driver/rtc_io.h"


// --------------------------------- LoRa E220 TTL --------------------------------- //
#define PIN_E220_AUX                                      GPIO_NUM_15
#define PIN_E220_M0                                       GPIO_NUM_19
#define PIN_E220_M1                                       GPIO_NUM_21

#define SEND_INTERVAL                                     2000

#define DESTINATION_ADDL                                  3

LoRa_E220 e220ttl(
    &Serial2,
    PIN_E220_AUX,
    PIN_E220_M0,
    PIN_E220_M1
);



// --------------------------------- AJSR04M --------------------------------- //

#define SENSOR_RANGE_MIN                                  20	
#define SENSOR_RANGE_MAX                                  450
#define SENSOR_TIMEOUT                                    3

#define PIN_AJSR04M_ECHO                                  5
#define PIN_AJSR04M_TRIG                                  4



// --------------------------------- APP DATA --------------------------------- //

struct tankData {
  int tank1_level;
  int tank2_level;
};

tankData currentTankData = {0, 0};
 
int measure_distance();



void setup() {
  // ------------- pin configuration ---------------- //
  pinMode(PIN_AJSR04M_TRIG, OUTPUT);
  pinMode(PIN_AJSR04M_ECHO, INPUT);
  

  // --------------- system setup ------------------- //
  Serial.begin(9600);


  // --------------- lora setup --------------------- //
  e220ttl.begin();
  e220ttl.setMode(MODE_2_WOR_RECEIVER);


  // ------------- send measurement ----------------- //
  e220ttl.setMode(MODE_0_NORMAL);

  currentTankData.tank1_level = measure_distance();
  currentTankData.tank2_level = 75;
  ResponseStatus rs = e220ttl.sendFixedMessage(0, DESTINATION_ADDL, 18, &currentTankData, sizeof(currentTankData));


  // ------------- go to sleep ---------------------- //
  e220ttl.setMode(MODE_2_WOR_RECEIVER);
  Serial2.end();

  esp_sleep_enable_ext0_wakeup(PIN_E220_AUX, LOW); 
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  gpio_deep_sleep_hold_en();

  esp_deep_sleep_start();
  delay(100);
  Serial.println("This will never be printed");
}
 
void loop() {}





int measure_distance(){
  int duration = 0;
  int distance = 0;
  int retries = 0;

  // Clears the trigPin condition
  digitalWrite(PIN_AJSR04M_TRIG, LOW);  //
  delayMicroseconds(2);

  // retry measurement until it is within acceptable range or until timeout is reached
  while((distance < SENSOR_RANGE_MIN || distance > SENSOR_RANGE_MAX) &&
        retries <= SENSOR_TIMEOUT) {
  
    // Sets the trigPin HIGH (ACTIVE) for 1000 microseconds to wake module
    digitalWrite(PIN_AJSR04M_TRIG, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PIN_AJSR04M_TRIG, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(PIN_AJSR04M_ECHO, HIGH);
    distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (there and back)

    // Displays the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance < SENSOR_RANGE_MIN || distance > SENSOR_RANGE_MAX) {
      Serial.println("Distance out of range.");
      distance = -1;
      if (retries < SENSOR_TIMEOUT) {
        delay(50); // wait for module to settle before retrying
      } 
    }
    ++retries;
  }

  return distance;
}