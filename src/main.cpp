#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


#define NUM_DRUMS 2
#define NUM_LEDS 240
#define LED_PIN 23
#define SLOWST_SPEED_HIT_PERLDS 5
#define MAX_WAVES NUM_LEDS * NUM_DRUMS * SLOWST_SPEED_HIT_PERLDS  // Max simultaneous animations
#define BUTTON_A_PIN 13  // Choose your GPIO pin
#define PIXEL_FACTOR 10
#define DELAY_POTENTIOMETER_PIN 35  // Speed control potentiometer (ADC1_CH7)
int piezoPins[NUM_DRUMS] = { 34, 36};//, 36, 39 };   // Piezo sensor pins (analog input capable)
int thresholdPotPins[NUM_DRUMS] = {32, 33};  // Threshold control potentiometers, one per drum

#define MAX_THRESHOLD_FOR_DRUMS 2000
#define MIN_THRESHOLD_FOR_DRUMS 10
#define HIT_COOLDOWN 100  // Minimum milliseconds between hits for same drum
#define READING_SAMPLES 3  // Number of samples to average
#define MIN_CONSECUTIVE_HITS 2  // Consecutive readings above threshold needed


CRGB leds[NUM_LEDS];
int consecutiveHighReadings[NUM_DRUMS] = {0};


int gThreshodl[NUM_DRUMS] = { 50};//, 350, 350 };  // Hit sensitivity - increased to reduce noise
int gWaveSpeed = 20;
unsigned long lastHitTime[NUM_DRUMS] = {0};  // Track last hit time for each drum

// Noise filtering



struct Wave {
  bool active;
  int startPixel;
  long int currentPixelTimesTen;
  CRGB color;
};

Wave waves[MAX_WAVES];

// Task handles
TaskHandle_t drumHitTaskHandle;
TaskHandle_t configTaskHandle;
TaskHandle_t ledTaskHandle;

// Forward declarations
void drumHitTask(void *parameter);
void configTask(void *parameter);
void ledTask(void *parameter);

void setup() {
  // Initialize hardware
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  
  for (int i = 0; i < NUM_DRUMS; i++) {
    pinMode(piezoPins[i], INPUT);
  }
  //pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  //pinMode(BUTTON_B_PIN, INPUT_PULLUP);

  Serial.println("Creating FreeRTOS tasks...");

  //Create Task 1: Drum hit detection (Core 1, Priority 2)
  xTaskCreatePinnedToCore(
    drumHitTask,           // Task function
    "DrumHitTask",         // Name
    4096,                  // Stack size (bytes)
    NULL,                  // Parameters
    2,                     // Priority (higher = more important)
    &drumHitTaskHandle,    // Task handle
    1                      // Core ID (1 = second core)
  );

  // Create Task 2: Configuration reading (Core 1, Priority 1)
  xTaskCreatePinnedToCore(
    configTask,
    "ConfigTask",
    2048,
    NULL,
    1,                     // Lower priority
    &configTaskHandle,
    1
  );

  // Create Task 3: LED updates (Core 0, Priority 2)
  xTaskCreatePinnedToCore(
    ledTask,
    "LEDTask",
    4096,
    NULL,
    2,
    &ledTaskHandle,
    0                      // Core 0 for LED operations
  );

  Serial.println("FreeRTOS tasks created successfully!");
}

bool isSameColor(CRGB color, int drumIndex)
{
  CRGB drumColor (drumIndex == 0 ? HUE_PURPLE : HUE_GREEN, 255, 255);
  return (drumColor == color);
}

void startWave(int drumIndex) {
  // Pick a free wave slot
  for (int i = 0; i < MAX_WAVES; i++) {
    if((waves[i].active) && ((waves[i].currentPixelTimesTen / PIXEL_FACTOR) == 0 )&& isSameColor(waves[i].color, drumIndex)){
      return;
    }
  }
  for (int i = 0; i < MAX_WAVES; i++) {
    if (!waves[i].active) {
      waves[i].startPixel = 0; //NUM_LEDS / NUM_DRUMS * drumIndex;
      waves[i].currentPixelTimesTen = 0;
      waves[i].color = CHSV(drumIndex == 0 ? HUE_PURPLE : HUE_GREEN, 255, 255);
      waves[i].active = true;
      break;
    }
  }
}


void updateWaves() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  int currentSpeed = gWaveSpeed;

  for (int i = 0; i < MAX_WAVES; i++) {
    if (!waves[i].active) continue;

    int pos = waves[i].startPixel + waves[i].currentPixelTimesTen / PIXEL_FACTOR;
    if (pos < NUM_LEDS && pos >= 0) {
      leds[pos] += waves[i].color;  // blend wave color
      waves[i].currentPixelTimesTen += currentSpeed;
    } else {
      waves[i].active = false;
    }
  }

  FastLED.show();
}

// ====================================================== //
// FreeRTOS Task Functions
// ====================================================== //

// Task 1: Read drum hits and buttons
void drumHitTask(void *parameter) {
  while (1) {
    // Check piezo sensors for drum hits
    unsigned long currentTime = millis();
    for (int i = 0; i < NUM_DRUMS; i++) {
      // Average multiple readings to reduce noise
      int sum = 0;
      for (int j = 0; j < READING_SAMPLES; j++) {
        sum += analogRead(piezoPins[i]);
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      int val = sum / READING_SAMPLES;
      
      if (val > gThreshodl[i]) {
        consecutiveHighReadings[i]++;
        
        // Only trigger if we have enough consecutive high readings and cooldown has passed
        if (consecutiveHighReadings[i] >= MIN_CONSECUTIVE_HITS && 
            currentTime - lastHitTime[i] >= HIT_COOLDOWN) {
          Serial.print("Drum ");
          Serial.print(i);
          Serial.print(" hit with value: ");
          Serial.println(val);
          startWave(i);
          lastHitTime[i] = currentTime;
          consecutiveHighReadings[i] = 0;  // Reset counter
        }
      } else {
        consecutiveHighReadings[i] = 0;  // Reset if reading drops below threshold
      }
    }

    // // Check buttons for manual triggering
    // int valA = digitalRead(BUTTON_A_PIN);
    // int valB = digitalRead(BUTTON_B_PIN);
    
    // if (valA == LOW) {
    //   Serial.println("Button A Pressed");
    //   startWave(0);
    //   vTaskDelay(pdMS_TO_TICKS(100));  // Simple debounce
    // }

    // if (valB == LOW) {
    //   Serial.println("Button B Pressed");
    //   startWave(1);
    //   vTaskDelay(pdMS_TO_TICKS(100));  // Simple debounce
    // }

    vTaskDelay(pdMS_TO_TICKS(1));  // Small delay to prevent hogging CPU
  }
}

// Task 2: Read configuration (potentiometer for speed control)
void configTask(void *parameter) {
  while (1) {
    // Read speed potentiometer
    int rawSpeed = analogRead(DELAY_POTENTIOMETER_PIN);
    int currSpeed = map(rawSpeed, 0, 4095, -20, 100);  // animation speed
    
    if (currSpeed != gWaveSpeed) {
      gWaveSpeed = currSpeed;
      Serial.print("gWaveSpeed = [");
      Serial.print(gWaveSpeed);
      Serial.println("]");
    }
   
    // Read threshold potentiometers for each drum
    for (int i = 0; i < NUM_DRUMS; i++) {
      int rawThreshold = analogRead(thresholdPotPins[i]);
      int newThreshold = map(rawThreshold, 0, 4095, MIN_THRESHOLD_FOR_DRUMS, MAX_THRESHOLD_FOR_DRUMS);  // Map to reasonable threshold range
      
      if (gThreshodl[i] != newThreshold) {
        gThreshodl[i] = newThreshold;
        Serial.print("Drum ");
        Serial.print(i);
        Serial.print(" threshold = ");
        Serial.println(newThreshold);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));  // Check config every 100ms
  }
}

// Task 3: Update and display LED animations
void ledTask(void *parameter) {
  while (1) {
    updateWaves();
    vTaskDelay(pdMS_TO_TICKS(1));  // Control animation speed
  }
}

void loop() {
  // Empty loop - all work is done in FreeRTOS tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ====================================================== //
// debugs
// ====================================================== //

#define INTERVAL 500
int nextMillisHit = 0;
void simulateHit(void) {
  if (nextMillisHit == 0 || nextMillisHit < millis()) {
    nextMillisHit = millis() + INTERVAL;
    startWave(0);
  }
}