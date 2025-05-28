#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "SPIFFS.h"

// === PIN DEFINITIONS ===
#define CAN_CS_PIN 5
#define CAN_INT_PIN 4
#define CAN_SCK 18
#define CAN_MISO 19
#define CAN_MOSI 23

MCP_CAN CAN(CAN_CS_PIN);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// === GLOBAL DISPLAY STATE ===
char displayBuffer[32];
bool updated = false;
const int NUM_SCREENS = 4;  // Added new detailed temperature screen

// DME1 (0x316)
int dme1_ignition = -1, dme1_crank = -1, dme1_tcs = -1;
int dme1_torque = -1, dme1_speed = -1, dme1_tq_loss = -1;

// DME2 (0x329)
int dme2_temp = -999, dme2_pressure = -999;

// DME4 (0x545)
int dme4_mil = -1, dme4_cru = -1, dme4_eml = -1;

// Display variables
const int FRAME_WIDTH = 128;
const int FRAME_HEIGHT = 64;
const int FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT / 8;
uint8_t bmw_animation[FRAME_SIZE];

// Development variables
bool show_intro = false;
int oil_temp = -99;  // Simulated oil temperature
int currentScreen = 3;  // 0 = RPM screen, 1 = Temperature screen, 2 = RPM meter screen, 3 = detailed temp screen

// RPM meter constants
const int NUM_BARS = 6;
const int RPM_THRESHOLDS[NUM_BARS] = {5250, 5500, 5750, 6000, 6250, 6500};
const int BLINK_THRESHOLD = 6500;

// Temperature history for graph
const int TEMP_HISTORY_SIZE = 32;
int intakeTempHistory[TEMP_HISTORY_SIZE];
int historyIndex = 0;

// Temperature thresholds
const int HIGH_TEMP_THRESHOLD = 100;  // Temperature threshold for warning icons
const int MIN_TEMP = 20;  // Lower minimum for intake temps
const int MAX_TEMP = 60;  // Lower maximum for intake temps

void setup() {
  Serial.begin(115200);
  SPI.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS_PIN);
  pinMode(CAN_INT_PIN, INPUT);

  // Initialize CAN
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("✅ MCP2515 initialized.");
  } else {
    Serial.println("❌ MCP2515 init failed. Check wiring.");
    while (1);
  }
  CAN.setMode(MCP_NORMAL);

  // OLED setup
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.clearBuffer();
  
  if(!SPIFFS.begin()){
	Serial.println("❌ SPIFFS Mount Failed");
  }
  else{
	Serial.println("✅ SPIFFS Mount Success");
  }
  File animFile = SPIFFS.open("/bmw_animation.bin");
  if(!animFile){
	Serial.println("❌ Failed to open file");
  }
  else{
	Serial.println("✅ File opened successfully");
  }

  if(show_intro)
  {
	for (int i = 0; i < 171; i++) {
		if(animFile.read(bmw_animation, FRAME_SIZE) != FRAME_SIZE){
			Serial.println("❌ Failed to read frame");
			break;
		}
		u8g2.clearBuffer();
		u8g2.drawXBMP(0, 0, FRAME_WIDTH, FRAME_HEIGHT, bmw_animation);
		u8g2.sendBuffer();
		delay(20);
	}
  }

// Simulated values for testing
  dme1_speed = 3596;
  dme2_temp = 96;
  dme1_torque = 68;
  dme1_tq_loss = 0;
}

void drawTemperatureScreen() {
  u8g2.clearBuffer();
  
  // Coolant Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);  // Smaller font for labels
  u8g2.drawStr(0, 15, "COOLANT");
  u8g2.setFont(u8g2_font_tenfatguys_tu);  // Bold font that supports numbers and letters
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", dme2_temp);
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
  int coolantFill = map(constrain(dme2_temp, minTemp, maxTemp), minTemp, maxTemp, 0, barWidth);
  u8g2.drawBox(barX, barY, coolantFill, barHeight);
  
  // Oil Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);  // Smaller font for labels
  u8g2.drawStr(0, 51, "OIL");
  u8g2.setFont(u8g2_font_tenfatguys_tu);  // Bold font that supports numbers and letters
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", oil_temp);
  u8g2.drawStr(80, 51, displayBuffer);
  
  // Oil Temperature Bar
  const int oilBarY = 56;
  u8g2.drawFrame(barX, oilBarY, barWidth, barHeight);
  int oilFill = map(constrain(oil_temp, minTemp, maxTemp), minTemp, maxTemp, 0, barWidth);
  u8g2.drawBox(barX, oilBarY, oilFill, barHeight);
  
  u8g2.sendBuffer();
}

void drawRPMScreen() {
  u8g2.clearBuffer();
  
  // === RPM Display ===
  u8g2.setFont(u8g2_font_logisoso22_tn);
  snprintf(displayBuffer, sizeof(displayBuffer), "%4drpm", dme1_speed);
  u8g2.drawStr(0, 22, displayBuffer);
  
  // === RPM Bar ===
  int rpmBarX = 0;
  int rpmBarY = 27;
  int rpmBarW = 128;
  int rpmBarH = 3;
  int rpmMax = 8500;
  int rpmBlinkThreshold = 6000;
  int rpmFill = map(dme1_speed, 0, rpmMax, 0, rpmBarW);
  
  static bool blinkState = false;
  static unsigned long lastBlink = 0;
  unsigned long now = millis();
  
  bool showBar = true;
  
  if (dme1_speed >= rpmBlinkThreshold) {
    if (now - lastBlink > 150) {
      blinkState = !blinkState;
      lastBlink = now;
    }
    showBar = blinkState;
  }
  u8g2.drawFrame(rpmBarX, rpmBarY, rpmBarW, rpmBarH);
  if (showBar) {
    u8g2.drawBox(rpmBarX, rpmBarY, rpmFill, rpmBarH);
    
    if (dme1_speed >= rpmBlinkThreshold) {
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
    if (dme2_temp > 90) {
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
  snprintf(displayBuffer, sizeof(displayBuffer), "TMP:%dC  IAT:%dC", dme2_temp, intakeTemp);
  u8g2.drawStr(0, 40, displayBuffer);
  
  // === Torque Info ===
  snprintf(displayBuffer, sizeof(displayBuffer), "TQ:%d%%  Loss:%d%%", dme1_torque, dme1_tq_loss);
  u8g2.drawStr(0, 52, displayBuffer);
  
  // === Footer ===
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 63, "E46 328i  A3228HX");
  u8g2.drawStr(100, 63, "13.8V");
  
  u8g2.sendBuffer();
}

void drawRPMMeterScreen() {
  u8g2.clearBuffer();
  
  // RPM Display in top left
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  snprintf(displayBuffer, sizeof(displayBuffer), "%d", dme1_speed);
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
  bool shouldBlink = dme1_speed >= BLINK_THRESHOLD;
  
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
    if (dme1_speed >= RPM_THRESHOLDS[i]) {
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
  
  int outTemp = dme2_temp - 10;  // Simulated OUT temperature
  bool tempWarning = (dme2_temp >= HIGH_TEMP_THRESHOLD) || (outTemp >= HIGH_TEMP_THRESHOLD);
  
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
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", dme2_temp);
  u8g2.drawStr(40, 15, displayBuffer);
  
  // OUT Temperature
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);
  u8g2.drawStr(0, 30, "OUT");
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", outTemp);
  u8g2.drawStr(40, 30, displayBuffer);
  
  // Draw separator line
  u8g2.drawHLine(0, 34, 128);
  
  // === INTAKE Temperature Section ===
  u8g2.setFont(u8g2_font_tenthinnerguys_tf);
  u8g2.drawStr(0, 46, "INTAKE");
  u8g2.setFont(u8g2_font_tenfatguys_tu);
  
  // Simulate varying intake temperature
  static int intakeTemp = 32;
  static unsigned long lastTempUpdate = 0;
  
  // Update temperature every 500ms
  if (now - lastTempUpdate > 500) {
    intakeTemp += random(-2, 3);  // Random change between -1 and +2
    intakeTemp = constrain(intakeTemp, MIN_TEMP, MAX_TEMP);
    lastTempUpdate = now;
  }
  
  snprintf(displayBuffer, sizeof(displayBuffer), "%d C", intakeTemp);
  u8g2.drawStr(60, 46, displayBuffer);
  
  // Update temperature history
  intakeTempHistory[historyIndex] = intakeTemp;
  historyIndex = (historyIndex + 1) % TEMP_HISTORY_SIZE;
  
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
    int y1 = graphY + graphHeight - map(intakeTempHistory[i-1], MIN_TEMP, MAX_TEMP, 0, graphHeight);
    int y2 = graphY + graphHeight - map(intakeTempHistory[i], MIN_TEMP, MAX_TEMP, 0, graphHeight);
    
    // Draw a line between points
    u8g2.drawLine(x1, y1, x2, y2);
  }
  
  u8g2.sendBuffer();
}

void loop() {
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
  
  // Simulate RPM increment
  dme1_speed += random(1, 100);
  delay(10);
  if (dme1_speed > 8000) dme1_speed = 1000;
  
  // Simulate temperature changes
  dme2_temp += random(-1, 2);
  oil_temp += random(-1, 2);
  
  // Keep temperatures in reasonable ranges
  dme2_temp = constrain(dme2_temp, 80, 110);
  oil_temp = constrain(oil_temp, 80, 110);
  
  //   if (CAN.checkReceive() == CAN_MSGAVAIL) {
  //     unsigned long rxId;
  //     unsigned char len = 0;
  //     unsigned char buf[8];
  //     CAN.readMsgBuf(&rxId, &len, buf);
  
  //     Serial.printf("📡 ID: 0x%03lX  LEN: %d  DATA:", rxId, len);
  //     for (int i = 0; i < len; i++) {
  //       Serial.printf(" %02X", buf[i]);
  //     }
  //     Serial.println();
  
  //     // === Decode known frames ===
  //     if (rxId == 0x316 && len >= 8) {
  //       dme1_ignition = (buf[0] & 0x01) > 0;
  //       dme1_crank    = (buf[0] & 0x02) > 0;
  //       dme1_tcs      = (buf[0] & 0x04) > 0;
  //       dme1_torque   = buf[1];  // Torque percentage (TQI_TQR_CAN)
  //       dme1_speed    = (buf[3] << 8) | buf[2];  // Engine RPM (N_ENG)
  //       dme1_tq_loss  = buf[5];  // Torque loss percentage (TQ_LOSS_CAN)
  //       updated = true;
  //     }
  
  //     else if (rxId == 0x329 && len >= 3) {
  //       dme2_temp     = (int)((float)buf[1] * 0.75 - 48); // Only safe if len >= 2
  //       dme2_pressure = buf[2] == 0xFF ? -1 : (int)(buf[2] * 2 + 598); // Only safe if len >= 3
  //       updated = true;
  //     }
  
  //     else if (rxId == 0x545 && len >= 1) {
  //       dme4_mil = (buf[0] & 0x02) > 0;
  //       dme4_cru = (buf[0] & 0x08) > 0;
  //       dme4_eml = (buf[0] & 0x10) > 0;
  //       updated = true;
  //     }
  //   }
}
