#include <FastLED.h>
#define NUM_LEDS 240
#define LED_PIN 5
CRGB leds[NUM_LEDS];

#define NUM_DRUMS 3
int piezoPins[NUM_DRUMS] = { 32, 33, 34 };   // Example pins
int thresholds[NUM_DRUMS] = { 50, 50, 50 };  // Hit sensitivity
int gWaveSpeed = 20;

#define MAX_WAVES NUM_LEDS * NUM_DRUMS * SLOWST_SPEED_HIT_PERLDS  // Max simultaneous animations
#define BUTTON_A_PIN 13  // Choose your GPIO pin
#define BUTTON_B_PIN 12  // Choose your GPIO pin
#define PIXEL_FACTOR 10

#define SLOWST_SPEED_HIT_PERLDS 5

struct Wave {
  bool active;
  int startPixel;
  long int currentPixelTimesTen;
  CRGB color;
};

Wave waves[MAX_WAVES];

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_DRUMS; i++) {
    pinMode(piezoPins[i], INPUT);
  }
  Serial.begin(115200);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
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

  for (int i = 0; i < MAX_WAVES; i++) {
    if (!waves[i].active) continue;

    int pos = waves[i].startPixel + waves[i].currentPixelTimesTen / PIXEL_FACTOR;
    if (pos < NUM_LEDS && pos >= 0) {
      leds[pos] += waves[i].color;  // blend wave color
      waves[i].currentPixelTimesTen += gWaveSpeed;
    } else {
      waves[i].active = false;
    }
  }

  FastLED.show();
}

#define HIT_DELAY 3

void checkHits() {
  for (int i = 0; i < 1; i++) {
    //int val = analogRead(piezoPins[i]);
    int valA = digitalRead(BUTTON_A_PIN);
    int valB = digitalRead(BUTTON_B_PIN);
    //if (val > thresholds[i]) {
    if (valA == LOW) {
      Serial.println("Button A Pressed");
      startWave(0);
    } else {
      Serial.println("Button A Released");
    }

    if (valB == LOW) {
      Serial.println("Button B Pressed");
      startWave(1);
    } else {
      Serial.println("Button B Released");
    }


  }
}

#define DELAY_POTENTIOMETER_PIN 33

void checkConfigSensors()
{
  int rawSpeed = analogRead(DELAY_POTENTIOMETER_PIN);
  int currSpeed = map(rawSpeed, 0, 4095, -20, 100);         // animation speed
  if (currSpeed != gWaveSpeed)
  {
    gWaveSpeed = currSpeed;
    Serial.print("gWaveSpeed = [");
    Serial.print(gWaveSpeed);
    Serial.println("]");
  }

  
}

void loop() {
  checkHits();

  checkConfigSensors();

  //simulateHit(); for debugging the leds
  updateWaves();
  delay(5);  // control animation speed
}



// ====================================================== //
// debugs


#define INTERVAL 500
int nextMillisHit = 0;
void simulateHit(void) {

  if (nextMillisHit == 0 || nextMillisHit < millis()) {
    nextMillisHit = millis() + INTERVAL;
    startWave(0);
  }
}