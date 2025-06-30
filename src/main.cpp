// TODO: (DME4) from instrument cluster https://forums.bimmerforums.com/forum/showthread.php?1887229-E46-Can-bus-project
// - add a way to request vin from the instrument cluster
// - add a way to request cluster data from the instrument cluster
// - add a way to request cluster data from the instrument cluster
#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "SPIFFS.h"
#include "BMW_CAN.h"
#include "Kawasaki_CAN.h"
#include "CAN_Reader.h"
#include "Serial_Handler.h"
#include "FakeDataGenerator.h"

// === PIN DEFINITIONS ===
#define CAN_CS_PIN 5
#define CAN_INT_PIN 4
#define CAN_SCK 18
#define CAN_MISO 19
#define CAN_MOSI 23

// === DISPLAY CONFIGURATION ===
const int FRAME_WIDTH = 128;
const int FRAME_HEIGHT = 64;
const int FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT / 8;
const int NUM_SCREENS = 4;

// === DEVELOPMENT CONFIGURATION ===
bool dev_mode = true;      // Development mode flag
bool show_intro = false;   // Show intro animation flag
int currentScreen = 3;     // Current display screen (0=RPM, 1=Temp, 2=RPM Meter, 3=Detailed Temp)

// === VEHICLE CONFIGURATION ===
VehicleType_t vehicleType = VEHICLE_BMW;  // Set to BMW by default

// === BMW CAN DATA ===
BMW_DME1_t dme1 = {0};
BMW_DME2_t dme2 = {0};
BMW_DME4_t dme4 = {0};
BMW_MS42_Temp_t ms42_temp = {0};
BMW_MS42_Status_t ms42_status = {0};
BMW_Kombi_t kombi = {0};

BMW_CAN_Context_t bmw_ctx = {&dme1, &dme2, &dme4, &ms42_temp, &ms42_status, &kombi};

// === KAWASAKI CAN DATA ===
Kawasaki_CAN_Data_t kawasaki_data = {0};

// === CAN READER CONTEXT ===
CAN_Reader_Context_t can_reader_ctx;

// === SERIAL HANDLER CONTEXT ===
Serial_Handler_Context_t serial_handler_ctx;

// === DISPLAY STATE ===
char displayBuffer[32];
bool displayUpdated = false;
uint8_t bmw_animation[FRAME_SIZE];

// === RPM METER CONFIGURATION ===
const int NUM_BARS = 6;
const int RPM_THRESHOLDS[NUM_BARS] = {5250, 5500, 5750, 6000, 6250, 6500};
const int BLINK_THRESHOLD = 6500;

// === TEMPERATURE CONFIGURATION ===
const int TEMP_HISTORY_SIZE = 32;
int tempHistory[TEMP_HISTORY_SIZE];
int tempHistoryIndex = 0;

const int HIGH_TEMP_THRESHOLD = 100;  // Temperature threshold for warning icons
const int MIN_INTAKE_TEMP = 20;       // Minimum intake temperature
const int MAX_INTAKE_TEMP = 60;       // Maximum intake temperature

// === HARDWARE OBJECTS ===
MCP_CAN CAN(CAN_CS_PIN);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Function prototypes
void drawIntro();
void drawTemperatureScreen();
void drawRPMScreen();
void drawRPMMeterScreen();
void drawDetailedTemperatureScreen();
void updateFakeData();
void emptyAllData();

// Serial Handler callback functions
void handleModeChange(bool devMode);
void handleIntroShow();
void handleVINRequest();
void handleVehicleTypeChange(VehicleType_t vehicleType);
void handleVehicleStatus(VehicleType_t vehicleType);

// Callback function implementations
void handleModeChange(bool devMode) {
    if (devMode) {
        updateFakeData();
    } else {
        emptyAllData();
    }
}

void handleIntroShow() {
    drawIntro();
}

void handleVINRequest() {
    // TODO: Implement VIN request functionality
    // This would typically send a CAN message to request VIN from instrument cluster
}

void handleVehicleTypeChange(VehicleType_t vehicleType) {
    // Vehicle type change is handled in the Serial_Handler
    // This callback can be used for additional actions when vehicle type changes
}

void handleVehicleStatus(VehicleType_t vehicleType) {
    // Vehicle status is handled in the Serial_Handler
    // This callback can be used for additional status reporting
}

void setup() {
  Serial.begin(115200);
  SPI.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS_PIN);
  pinMode(CAN_INT_PIN, INPUT);

  // Initialize CAN
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 initialized.");
  } else {
    Serial.println("MCP2515 init failed. Check wiring.");
    while (1);
  }
  CAN.setMode(MCP_NORMAL);

  // Initialize CAN Reader
  CAN_Reader_init(&can_reader_ctx, vehicleType, &CAN, &displayUpdated);

  // Initialize Serial Handler
  Serial_Handler_init(&serial_handler_ctx, 
                     &currentScreen, 
                     &dev_mode, 
                     &vehicleType,
                     &can_reader_ctx,
                     &CAN,
                     &displayUpdated,
                     nullptr,  // screenChangeCallback (not needed as we handle it directly)
                     handleModeChange,
                     handleIntroShow,
                     handleVINRequest,
                     handleVehicleTypeChange,
                     handleVehicleStatus);

  // OLED setup
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.clearBuffer();
  
  if(!SPIFFS.begin()){
	Serial.println("SPIFFS Mount Failed");
  }
  else{
	Serial.println("SPIFFS Mount Success");
  }

  if(show_intro)
  {
    drawIntro();
  }

  // Initialize values based on mode
  if (dev_mode) {
    updateFakeData();
  } else {
    emptyAllData();
  }

  // Print initial help message
  Serial.println("\nBMW Screen Simulator");
  Serial.println("Type 'help' for available commands");
  Serial.print("> "); // Show initial prompt
}

void drawIntro() {
    u8g2.clearBuffer();
    File animFile = SPIFFS.open("/bmw_animation.bin");
    if(!animFile){
    Serial.println("Failed to open file");
    }
    else{
    Serial.println("File opened successfully");
    }
  	for (int i = 0; i < 171; i++) {
		if(animFile.read(bmw_animation, FRAME_SIZE) != FRAME_SIZE){
			Serial.println("Failed to read frame");
			break;
		}
		u8g2.clearBuffer();
		u8g2.drawXBMP(0, 0, FRAME_WIDTH, FRAME_HEIGHT, bmw_animation);
		u8g2.sendBuffer();
		delay(20);
    }
}
void drawTemperatureScreen() {
  u8g2.clearBuffer();
  
  // Coolant Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);  // Smaller font for labels
  u8g2.drawStr(0, 15, "COOLANT");
  u8g2.setFont(u8g2_font_tenfatguys_tu);  // Bold font that supports numbers and letters
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", dme2.coolantTemp);
  u8g2.drawStr(80, 18, displayBuffer);
  
  // Coolant Temperature Bar
  const int barY = 23;
  const int barHeight = 8;
  const int barWidth = 120;
  const int barX = 4;
  const int minTemp = 80;
  const int maxTemp = 110;
  
  // Draw bar background
  u8g2.drawFrame(barX, barY, barWidth, barHeight);
  
  // Calculate fill width based on temperature
  int coolantFill = map(constrain(dme2.coolantTemp, minTemp, maxTemp), minTemp, maxTemp, 0, barWidth);
  u8g2.drawBox(barX, barY, coolantFill, barHeight);
  
  // Oil Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);  // Smaller font for labels
  u8g2.drawStr(0, 51, "OIL");
  u8g2.setFont(u8g2_font_tenfatguys_tu);  // Bold font that supports numbers and letters
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", ms42_temp.oilTemp);
  u8g2.drawStr(80, 51, displayBuffer);
  
  // Oil Temperature Bar
  const int oilBarY = 56;
  u8g2.drawFrame(barX, oilBarY, barWidth, barHeight);
  int oilFill = map(constrain(ms42_temp.oilTemp, minTemp, maxTemp), minTemp, maxTemp, 0, barWidth);
  u8g2.drawBox(barX, oilBarY, oilFill, barHeight);
  
  u8g2.sendBuffer();
}

void drawRPMScreen() {
  u8g2.clearBuffer();
  
  // === RPM Display ===
  u8g2.setFont(u8g2_font_logisoso22_tn);
  snprintf(displayBuffer, sizeof(displayBuffer), "%4drpm", dme1.rpm);
  u8g2.drawStr(0, 22, displayBuffer);
  
  // === RPM Bar ===
  int rpmBarX = 0;
  int rpmBarY = 27;
  int rpmBarW = 128;
  int rpmBarH = 3;
  int rpmMax = 8500;
  int rpmBlinkThreshold = 6000;
  int rpmFill = map(dme1.rpm, 0, rpmMax, 0, rpmBarW);
  
  static bool blinkState = false;
  static unsigned long lastBlink = 0;
  unsigned long now = millis();
  
  bool showBar = true;
  
  if (dme1.rpm >= rpmBlinkThreshold) {
    if (now - lastBlink > 150) {
      blinkState = !blinkState;
      lastBlink = now;
    }
    showBar = blinkState;
  }
  u8g2.drawFrame(rpmBarX, rpmBarY, rpmBarW, rpmBarH);
  if (showBar) {
    u8g2.drawBox(rpmBarX, rpmBarY, rpmFill, rpmBarH);
    
    if (dme1.rpm >= rpmBlinkThreshold) {
      // Draw warning triangle
      int centerX = 108;
      int topY = 2;  // Moved down slightly
      int symbolSize = 22;  // Made smaller
      int width = 12;  // Reduced width of triangle base
      
      // Triangle points
      int x1 = centerX;                // Top point
      int y1 = topY;
      int x2 = centerX - width;        // Bottom left
      int y2 = topY + symbolSize;
      int x3 = centerX + width;        // Bottom right
      int y3 = topY + symbolSize;
      
      // Draw triangle
      u8g2.drawLine(x1, y1, x2, y2);      // Left side
      u8g2.drawLine(x2, y2, x3, y3);      // Bottom
      u8g2.drawLine(x3, y3, x1, y1);      // Right side
      
      // Exclamation mark centered inside
      u8g2.setFont(u8g2_font_7x13B_tf);   // Bold and readable
      u8g2.drawStr(centerX - 3, topY + 17, "!");
    }
    
    // Draw temperature warning if over 90°C
    if (dme2.coolantTemp > 90) {
      // Engine Temp Warning Icon with Waves
      int tempX = 78;   // X position of thermometer
      int tempY = 2;    // Start near the top
      int waveWidth = 16;
      
      // Shortened thermometer stem
      u8g2.drawLine(tempX, tempY, tempX, tempY + 9);
      u8g2.drawLine(tempX + 1, tempY, tempX + 1, tempY + 9);
      
      // Smaller bulb
      u8g2.drawCircle(tempX, tempY + 12, 3, U8G2_DRAW_ALL);
      u8g2.drawDisc(tempX, tempY + 12, 1);
      
      // Side ticks
      u8g2.drawPixel(tempX - 3, tempY + 2);
      u8g2.drawPixel(tempX - 3, tempY + 5);
      u8g2.drawPixel(tempX - 3, tempY + 7);
      
      // Compact waves (tighter and lower)
      for (int x = 0; x < waveWidth; x += 4) {
        u8g2.drawLine(tempX + x - 8, tempY + 17, tempX + x - 6, tempY + 16);
        u8g2.drawLine(tempX + x - 6, tempY + 16, tempX + x - 4, tempY + 17);
      }
      for (int x = 0; x < waveWidth; x += 4) {
        u8g2.drawLine(tempX + x - 8, tempY + 19, tempX + x - 6, tempY + 18);
        u8g2.drawLine(tempX + x - 6, tempY + 18, tempX + x - 4, tempY + 19);
      }
    }
  }
  
  // === Engine Temp & Intake Temp ===
  u8g2.setFont(u8g2_font_6x12_tr);
  int intakeTemp = 32; // placeholder
  snprintf(displayBuffer, sizeof(displayBuffer), "TMP:%dC  IAT:%dC", dme2.coolantTemp, ms42_temp.intakeTemp);
  u8g2.drawStr(0, 40, displayBuffer);
  
  // === Torque Info ===
  snprintf(displayBuffer, sizeof(displayBuffer), "TQ:%d%%  Loss:%d%%", dme1.torque, dme1.torqueLoss);
  u8g2.drawStr(0, 52, displayBuffer);
  
  // === Footer ===
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 63, "TEST TEST RPMLINK");
  u8g2.drawStr(100, 63, "12.8V");
  
  u8g2.sendBuffer();
}

void drawRPMMeterScreen() {
  u8g2.clearBuffer();
  
  // RPM Display in top left
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  snprintf(displayBuffer, sizeof(displayBuffer), "%d", dme1.rpm);
  u8g2.drawStr(0, 15, displayBuffer);
  
  // Small RPM text under the number
  u8g2.setFont(u8g2_font_lucasfont_alternate_tf);  // Very small font
  u8g2.drawStr(24, 24, "RPM");
  
  // Bar parameters
  const int startX = 4;
  const int startY = 20;
  const int barSpacing = 4;
  const int maxBarHeight = 54;
  const int barWidth = 16;
  const int baseHeight = 24;  // Starting height for first bar
  const int heightStep = 6;   // How much taller each bar gets
  const int bigStep = 12;     // Bigger step for bars 5 and 6
  
  // Calculate if we should blink (above 6500 RPM)
  static bool blinkState = false;
  static unsigned long lastBlink = 0;
  unsigned long now = millis();
  bool shouldBlink = dme1.rpm >= BLINK_THRESHOLD;
  
  if (shouldBlink && (now - lastBlink > 100)) {
    blinkState = !blinkState;
    lastBlink = now;
  }
  
  // Draw bars
  for (int i = 0; i < NUM_BARS; i++) {
    int barHeight;
    if (i < 4) {
      // Bars 1-4: gradual increase
      barHeight = baseHeight + (i * heightStep);
    } else {
      // Bars 5-6: bigger step
      barHeight = baseHeight + (3 * heightStep) + ((i - 3) * bigStep);
    }
    int x = startX + (i * (barWidth + barSpacing));
    int y = startY + (maxBarHeight - barHeight);  // Align to bottom
    
    // Draw bar background
    u8g2.drawFrame(x, y, barWidth, barHeight);
    
    // Fill bar if RPM is above threshold
    if (dme1.rpm >= RPM_THRESHOLDS[i]) {
      if (!shouldBlink || blinkState) {
        u8g2.drawBox(x, y, barWidth, barHeight);
      }
    }
  }
  
  
  u8g2.sendBuffer();
}

void drawDetailedTemperatureScreen() {
  u8g2.clearBuffer();
  
  // === Temperature Warning Icon (if either IN or OUT is too high) ===
  static bool blinkState = false;
  static unsigned long lastBlink = 0;
  unsigned long now = millis();
  
  bool tempWarning = (dme2.coolantTemp >= HIGH_TEMP_THRESHOLD) || (ms42_temp.outletTemp >= HIGH_TEMP_THRESHOLD);
  
  if (tempWarning) {
    if (now - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = now;
    }
    if (blinkState) {
      // Draw larger, more detailed temperature warning icon
      int iconX = 95;  // Position on the right side
      int iconY = 2;   // Start from top
      
      // Thermometer stem (thicker)
      u8g2.drawLine(iconX, iconY, iconX, iconY + 16);
      u8g2.drawLine(iconX + 1, iconY, iconX + 1, iconY + 16);
      u8g2.drawLine(iconX + 2, iconY, iconX + 2, iconY + 16);
      
      // Thermometer bulb (larger)
      u8g2.drawCircle(iconX + 1, iconY + 20, 5, U8G2_DRAW_ALL);
      u8g2.drawDisc(iconX + 1, iconY + 20, 2);
      
      // Temperature waves (more visible)
      for (int i = 0; i < 3; i++) {
        int waveY = iconY + 26 + (i * 4);
        u8g2.drawLine(iconX - 4, waveY, iconX + 6, waveY);
        u8g2.drawLine(iconX - 2, waveY - 1, iconX + 4, waveY - 1);
      }
      
      // Exclamation mark
      u8g2.setFont(u8g2_font_7x13B_tf);
      u8g2.drawStr(iconX - 2, iconY + 12, "!");
    }
  }
  
  // === IN/OUT Temperatures Section ===
  // IN Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);
  u8g2.drawStr(0, 15, "IN");
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", dme2.coolantTemp);
  u8g2.drawStr(40, 15, displayBuffer);
  
  // OUT Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);
  u8g2.drawStr(0, 30, "OUT");
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", ms42_temp.outletTemp);
  u8g2.drawStr(40, 30, displayBuffer);
  
  // Draw separator line
  u8g2.drawHLine(0, 34, 128);
  
  // === INTAKE Temperature Section ===
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);
  u8g2.drawStr(0, 46, "INTAKE");
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", ms42_temp.intakeTemp);
  u8g2.drawStr(60, 46, displayBuffer);
  
  // Update temperature history
  tempHistory[tempHistoryIndex] = ms42_temp.intakeTemp;
  tempHistoryIndex = (tempHistoryIndex + 1) % TEMP_HISTORY_SIZE;
  
  // Draw temperature history graph
  const int graphX = 0;
  const int graphY = 50;
  const int graphWidth = 128;
  const int graphHeight = 14;
  
  // Draw graph background
  u8g2.drawFrame(graphX, graphY, graphWidth, graphHeight);
  
  // Draw temperature history with lines instead of pixels
  for (int i = 1; i < TEMP_HISTORY_SIZE; i++) {
    int x1 = graphX + ((i-1) * graphWidth / TEMP_HISTORY_SIZE);
    int x2 = graphX + (i * graphWidth / TEMP_HISTORY_SIZE);
    int y1 = graphY + graphHeight - map(tempHistory[i-1], MIN_INTAKE_TEMP, MAX_INTAKE_TEMP, 0, graphHeight);
    int y2 = graphY + graphHeight - map(tempHistory[i], MIN_INTAKE_TEMP, MAX_INTAKE_TEMP, 0, graphHeight);
    
    // Draw a line between points
    u8g2.drawLine(x1, y1, x2, y2);
  }
  
  u8g2.sendBuffer();
}


void emptyAllData() {
    // Reset DME1 values
    dme1.ignition = false;
    dme1.cranking = false;
    dme1.tcs = false;
    dme1.torque = -1;
    dme1.rpm = -1;
    dme1.torqueLoss = -1;
    
    // Reset DME2 values
    dme2.coolantTemp = -999;
    dme2.manifoldPressure = -999;
    
    // Reset DME4 values
    dme4.mil = false;
    dme4.cruise = false;
    dme4.eml = false;
    
    // Reset MS42 values
    ms42_temp.intakeTemp = -999;
    ms42_temp.oilTemp = -999;
    ms42_temp.outletTemp = -999;
    ms42_status.fuelPressure = -999;
    ms42_status.lambda = -999;
    ms42_status.maf = -999;

    // Reset VIN data
    memset(kombi.vin, 0, sizeof(kombi.vin));
    kombi.vinReceived = false;
}

void loop() {
  // Handle any serial input
  Serial_Handler_processInput(&serial_handler_ctx);
  
  // Draw appropriate screen
  if (currentScreen == 1) {
    drawTemperatureScreen();
  } else if (currentScreen == 2) {
    drawRPMMeterScreen();
  } else if (currentScreen == 3) {
    drawDetailedTemperatureScreen();
  } else {
    drawRPMScreen();
  }
  
  if (dev_mode) {
    FakeDataGenerator_updateBMW(&bmw_ctx);
    // FakeDataGenerator_updateKawasaki(&kawasaki_data);
  } else {
    CAN_Reader_readMessages(&can_reader_ctx, &bmw_ctx, &kawasaki_data);
    delay(100);
  }
}
