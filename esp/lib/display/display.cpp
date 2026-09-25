#include "display.h"
#include "sensor_Function.h"
#include "gyro.h"
#include "encoder.h"
#include "comms.h"

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

void Display_Telemetry(uint16_t rawL, uint16_t rawF, uint16_t rawR, 
                       bool wL, bool wF, bool wR, 
                       float gyro, int32_t encL, int32_t encR, uint16_t cells) {
    display.clearDisplay();
    
    // --- Graphical Top-Down View ---
    // Robot center
    int cx = 64;
    int cy = 24;
    
    // Draw robot (small filled rectangle)
    display.fillRect(cx - 6, cy - 8, 12, 16, SSD1306_WHITE);
    // Draw direction indicator (triangle pointing up)
    display.fillTriangle(cx, cy - 10, cx - 4, cy - 4, cx + 4, cy - 4, SSD1306_BLACK);

    // Draw detected walls (Thick lines)
    if (wF) display.fillRect(cx - 16, cy - 20, 32, 4, SSD1306_WHITE); // Front Wall
    if (wL) display.fillRect(cx - 20, cy - 16, 4, 32, SSD1306_WHITE); // Left Wall
    if (wR) display.fillRect(cx + 16, cy - 16, 4, 32, SSD1306_WHITE); // Right Wall

    // --- Text Distances ---
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Front distance and Cells (Top center)
    display.setCursor(cx - 16, 0);
    display.print(rawF == 9999 ? "---" : String(rawF));
    display.print(" | C:");
    display.print(cells);
    
    // Left distance (Left side)
    display.setCursor(0, cy - 4);
    display.print(rawL == 9999 ? "---" : String(rawL));
    
    // Right distance (Right side)
    display.setCursor(cx + 26, cy - 4);
    display.print(rawR == 9999 ? "---" : String(rawR));

    // --- Bottom Info Section ---
    display.setCursor(0, 44);
    display.print("WALLS:");
    display.print(wL ? " [L]" : "    ");
    display.print(wF ? " [F]" : "    ");
    display.print(wR ? " [R]" : "    ");

    display.setCursor(0, 54);
    display.printf("G:%4.1f E:%ld,%ld", gyro, (long)encL, (long)encR);

    display.display();
}

void Update_UI(void) {
    RobotParams &p = getParams();
    int32_t leftTicks = getLeftEncCount() * p.enc_L_direction;
    int32_t rightTicks = getRightEncCount() * p.enc_R_direction;
    float gyroZ = getGyroZ();
    
    readSensors();
    bool wFront = isWallFront(p.wall_front_thresh);
    bool wRight = isWallRight(p.wall_side_thresh);
    bool wLeft = isWallLeft(p.wall_side_thresh);

    // Serial
    Serial.printf("[TEL] 90R=%4d 45R=%4d 0R=%4d 0L=%4d 45L=%4d 90L=%4d  encL=%6ld encR=%6ld  GyroZ=%6.1f\r\n",
                  sensorMM[SENSOR_90R], sensorMM[SENSOR_45R], sensorMM[SENSOR_0R],
                  sensorMM[SENSOR_0L], sensorMM[SENSOR_45L], sensorMM[SENSOR_90L],
                  (long)leftTicks, (long)rightTicks, gyroZ);

    extern uint16_t cells_driven;
    // Display
    uint16_t frontAvg = (sensorMM[SENSOR_0L] == 9999 || sensorMM[SENSOR_0R] == 9999) ? 9999 : (sensorMM[SENSOR_0L] + sensorMM[SENSOR_0R]) / 2;
    Display_Telemetry(sensorMM[SENSOR_90L], frontAvg, sensorMM[SENSOR_90R],
                      wLeft, wFront, wRight, gyroZ, leftTicks, rightTicks, cells_driven);
}
