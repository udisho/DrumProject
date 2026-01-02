# DrumProject - LED Drum Controller

This project uses an ESP32 to create an interactive LED display controlled by drum hits (piezo sensors) or buttons.

## Hardware Requirements

- ESP32 Development Board (esp32dev)
- WS2812B LED Strip (NeoPixel compatible) - 240 LEDs
- 2 Push Buttons (for testing/control)
- 1 Potentiometer (for speed control)
- Piezo sensors (for drum hit detection - currently not active in code)

## Pin Configuration

### LED Strip Connection

| Component    | Pin Number | Alternative Names | Notes                                                  |
|--------------|------------|-------------------|--------------------------------------------------------|
| LED Data Pin | GPIO 5     | D5, IO5           | Connect to DIN/Data In of LED strip                    |
| LED Power    | 5V         | VIN               | Power supply for LED strip (ensure adequate current)   |
| LED Ground   | GND        | Ground, G         | Common ground with ESP32                               |

**Important:** For 240 LEDs, you'll need an external 5V power supply (5-10A recommended). Don't power the LEDs directly from the ESP32's 5V pin.

### Button Connections

| Component | Pin Number | Alternative Names | Pull Configuration      | Notes                                                       |
|-----------|------------|-------------------|-------------------------|-------------------------------------------------------------|
| Button A  | GPIO 13    | D13, IO13         | INPUT_PULLUP (internal) | Connect button between pin and GND. Triggers purple wave   |
| Button B  | GPIO 12    | D12, IO12         | INPUT_PULLUP (internal) | Connect button between pin and GND. Triggers green wave    |

**Wiring:** Connect one leg of each button to the GPIO pin and the other leg to GND. The internal pull-up resistor is enabled, so no external resistor needed. Button press = LOW signal.

### Potentiometer (Speed Control)

| Component           | Pin Number | Alternative Names   | Notes                                    |
|---------------------|------------|---------------------|------------------------------------------|
| Speed Control       | GPIO 35    | A35, ADC1_CH7, IO35 | Connect to middle pin of potentiometer   |
| Potentiometer VCC   | 3.3V       | 3V3                 | Connect to one outer pin                 |
| Potentiometer GND   | GND        | Ground              | Connect to other outer pin               |

**Range:** Controls wave animation speed from -20 to 100 (mapped from 0-4095 ADC reading)

### Piezo Sensors (Drum Pads) - NOW ACTIVE

| Component | Pin Number | Alternative Names               | Threshold | Status                                   |
|-----------|------------|---------------------------------|-----------|------------------------------------------|
| Drum 1    | GPIO 32    | A32, ADC1_CH4, IO32             | 50        | Active - triggers wave animation         |
| Drum 2    | GPIO 36    | A36, ADC1_CH0, IO36, Input Only | 50        | Active - triggers wave animation         |
| Drum 3    | GPIO 39    | A39, ADC1_CH3, IO39, Input Only | 50        | Active - triggers wave animation         |

**Note:** GPIO 35, 36, 39 are input-only pins on ESP32 (no internal pull-up/pull-down resistors). Perfect for analog sensors like piezo elements.

## Wiring Diagram Summary

```
ESP32                          Components
-----------------------------------------
GPIO 5  (D5)      -----------> LED Strip Data In
5V/VIN            -----------> LED Strip VCC (via external power supply)
GND               -----------> LED Strip GND + Button Common

GPIO 13 (D13)     -----------> Button A (other leg to GND)
GPIO 12 (D12)     -----------> Button B (other leg to GND)

GPIO 32 (A32)     -----------> Piezo Sensor 1 (Drum 1) + terminal, - to GND
GPIO 36 (A36)     -----------> Piezo Sensor 2 (Drum 2) + terminal, - to GND
GPIO 39 (A39)     -----------> Piezo Sensor 3 (Drum 3) + terminal, - to GND

GPIO 35 (A35)     -----------> Potentiometer Middle Pin
3.3V              -----------> Potentiometer One Side
GND               -----------> Potentiometer Other Side
```

## Pin Name Conventions

ESP32 pins can be referred to by multiple names:
- **GPIO Number** (GPIO 5, GPIO 13) - Official designation
- **D-Number** (D5, D13) - Common on some boards
- **IO-Number** (IO5, IO13) - Alternative notation
- **ADC Channels** (ADC1_CH4, ADC1_CH5) - For analog-capable pins
- **A-Number** (A32, A33, A34) - Analog pin designation

Always use the GPIO number in code for consistency.

## LED Strip Power Considerations

- Each WS2812B LED can draw up to 60mA at full white brightness
- 240 LEDs × 60mA = 14.4A maximum (theoretical)
- Typical usage with colors: 5-8A
- **Use a dedicated 5V power supply** rated for at least 10A
- Connect ESP32 GND to power supply GND (common ground)
- Power the LED strip directly from the external supply, not through ESP32

## Current Functionality

- **Piezo Sensors** (GPIO 32, 36, 39): Detect drum hits and trigger wave animations (threshold: 50)
- **Button A** (GPIO 13): Manual trigger for purple wave animation (backup control)
- **Button B** (GPIO 12): Manual trigger for green wave animation (backup control)
- **Potentiometer** (GPIO 35): Adjusts wave animation speed (-20 to 100)
- Waves travel from pixel 0 to pixel 239
- Multiple waves can be active simultaneously
- Each drum can have different colors based on drumIndex

## Serial Monitor

- Baud rate: 115200
- Outputs:
  - Drum hit events with sensor values
  - Button press events
  - Speed changes from potentiometer
- Use for debugging and threshold tuning

## Programming/Upload

Use PlatformIO to build and upload:
```bash
platformio run --target upload
platformio device monitor
```

## Notes

- All piezo sensors are now active and functional
- GPIO pin conflicts have been resolved:
  - Potentiometer moved from GPIO 33 to GPIO 35
  - Piezo sensors use GPIO 32, 36, 39 (all ADC-capable, input-only pins)
- Buttons A and B serve as backup manual triggers
- Adjust threshold values in code if sensors are too sensitive or not sensitive enough
- Consider adding a 1MΩ resistor in parallel with each piezo sensor for better readings
