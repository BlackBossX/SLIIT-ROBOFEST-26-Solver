#include "display.h"

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the default Wire library (21, 22 on ESP32)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool Display_Init(void) {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    // Note: Wire.begin() is already called in Sensor_Configuration()
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("[OLED] SSD1306 allocation failed or not found");
        return false;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Micromouse OLED");
    display.println("Initializing...");
    display.display();
    
    Serial.println("[OLED] Initialized SSD1306");
    return true;
}

void Display_Message(const char* msg) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Micromouse Status:");
    display.println();
    display.println(msg);
    display.display();
}

void Display_Telemetry(uint16_t wL, uint16_t wF, uint16_t wR, float gyro, int32_t encL, int32_t encR) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    // Draw walls
    display.print("L:"); display.print(wL == 9999 ? "---" : String(wL)); 
    display.print(" F:"); display.print(wF == 9999 ? "---" : String(wF));
    display.print(" R:"); display.println(wR == 9999 ? "---" : String(wR));
    
    display.println();
    
    // Draw Gyro
    display.print("Gyro Z: "); display.print(gyro, 1); display.println(" deg/s");
    
    // Draw Encoders
    display.print("EncL: "); display.println(encL);
    display.print("EncR: "); display.println(encR);

    display.display();
}
