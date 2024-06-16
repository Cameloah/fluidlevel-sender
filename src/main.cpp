#include "main.h"

#define PIN_E220_AUX                                      GPIO_NUM_0
#define PIN_E220_M0                                       19
#define PIN_E220_M1                                       21

#define SEND_INTERVAL                                     2000

#define SENSOR_RANGE_MIN                                  20	
#define SENSOR_RANGE_MAX                                  450
#define SENSOR_TIMEOUT                                    10

#define PIN_AJSR04M_ECHO                                  5
#define PIN_AJSR04M_TRIG                                  4

LoRa_E220 e220ttl(
  &Serial2,
  PIN_E220_AUX,
  PIN_E220_M1,              // ATTENTION: M1 and M0 pins are swapped!!
  PIN_E220_M0
  );
 
struct tankData {
  int tank1_level;
  int tank2_level;
};

tankData currentTankData = {0, 0};
 
int measure_distance();

void printWakeUpReason();



void setup() {
  // ------------- pin configuration ---------------- //
  pinMode(PIN_AJSR04M_TRIG, OUTPUT);
  pinMode(PIN_AJSR04M_ECHO, INPUT);
  
  // --------------- system setup ------------------- //
  Serial.begin(9600);
  printWakeUpReason();

  delay(2000);

  esp_sleep_enable_ext0_wakeup(PIN_E220_AUX, LOW);

  
 
  // --------------- lora setup --------------------- //
  Serial.println("Initializing LoRa module");
  e220ttl.begin();

  delay(2000);

  // lora_set_config(e220ttl);
  // lora_get_config(e220ttl);
  

  e220ttl.setMode(MODE_2_WOR_RECEIVER);
  delay(2000);


  currentTankData.tank1_level = measure_distance();
  currentTankData.tank2_level = 75;
  ResponseStatus rs = e220ttl.sendMessage(&currentTankData, sizeof(currentTankData));
  Serial.println("Send status: " + rs.getResponseDescription());
  delay(2000);
  Serial.println("Entering deep sleep.");
  esp_deep_sleep_start();             // enter deep sleep
}
 
void loop() {
}



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
  
    Serial.println("Waking up AJ-SR04M module");
    // Sets the trigPin HIGH (ACTIVE) for 1000 microseconds to wake module
    digitalWrite(PIN_AJSR04M_TRIG, HIGH);
    delayMicroseconds(1000);
    digitalWrite(PIN_AJSR04M_TRIG, LOW);

    Serial.println("Read measured distance");
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(PIN_AJSR04M_ECHO, HIGH);
    // Calculating the distance
    distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)

    // Displays the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance < SENSOR_RANGE_MIN || distance > SENSOR_RANGE_MAX) {
      Serial.println("Distance out of range.");
      distance = 0;
      if (retries < SENSOR_TIMEOUT) {
        delay(50); // wait for module to settle before retrying
      } 
    }
    ++retries;
  }

  return distance;
}

void printWakeUpReason() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

  switch (wakeupReason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("The device was awakened by an external signal using RTC_IO.");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("The device was awakened by an external signal using RTC_CNTL.");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("The device was awakened by a timer event.");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("The device was awakened by a touchpad interaction.");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("The device was awakened by a ULP (Ultra-Low-Power) program.");
      break;
    default:
      Serial.printf("The device woke up for an unknown reason: %d\n", wakeupReason);
      break;
  }
}