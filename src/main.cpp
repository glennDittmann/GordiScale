#include <Arduino.h>
#include <LiquidCrystal.h>
#include <WheatstoneBridge.h>
#include <strain_gauge_shield_and_lcd_support_functions.h>
#include <characters.h>

// Note that the LCD button boundaries have been modified in the library
// file (strain_gauge_shield_and_lcd_support_functions.cpp) to match our LCD

#define MSG_DELAY 2500


// specifiy the numbers of the LCD interface pins to Arduino
// rs: register select pin
// en: enable pin
// di: data pins
// bl: back light pin
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 10;

// initialize lcd screen with the numbers of the interface pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


// calibrating values
const int CST_STRAIN_IN_MIN = 350, CST_STRAIN_IN_MAX = 650;  // raw value calibration lower and upper point
const int CST_STRAIN_OUT_MIN = 0, CST_STRAIN_OUT_MAX = 1000;  // weight calibration lower and upper point

// setting lower and upper bounds + step sizes for calculated force values
const int CST_CAL_FORCE_MIN = 0, CST_CAL_FORCE_MAX = 32000, CST_CAL_FORCE_STEP = 50, CST_CAL_FORCE_STEP_LARGE = 500;

// initialize wheatstone bridge interface objects
WheatstoneBridge wsb_strain1(A0, CST_STRAIN_IN_MIN, CST_STRAIN_IN_MAX, CST_STRAIN_OUT_MIN, CST_STRAIN_OUT_MAX);


void display_manual() {
  displayScreen("Gordis Waage, is", "online. BEEPBEEP");
  delay(MSG_DELAY);
  displayScreen("Now you calibra-"   , "te the scale.");
  delay(MSG_DELAY);
  displayScreen("Use arrow keys"   , "to change values.");
  delay(MSG_DELAY);
  displayScreen("Next with select"   , "rst restarts.");
  delay(MSG_DELAY);
}


int calibrate_force(int calibrated_force, char * message) {
  displayScreen("* Calibration *", message);
  delay(MSG_DELAY);

  calibrated_force = getValueInRange("Set force", "Force:", 7, calibrated_force,
                                      CST_CAL_FORCE_MIN, CST_CAL_FORCE_MAX,
                                      CST_CAL_FORCE_STEP, CST_CAL_FORCE_STEP_LARGE);
  return calibrated_force;
}


int calibrate_adc(char * message) {
  displayScreen("* Calibration *", message);
  delay(MSG_DELAY);

  int calibrated_adc = getValueADC("Set raw ADC", "Raw ADC:", 9, A0, btnSELECT);

  return calibrated_adc;
}


void check_buttons(int x){
  lcd.setCursor(0,1);
  if (x < 60) {
    lcd.print("Right button.   ");
  }
  else if (x < 200) {
    lcd.print("Up button.      ");
  }
  else if (x < 400) {
    lcd.print("Down button.    ");
  }
  else if (x < 600) {
    lcd.print("Left button.    ");
  }
  else if (x < 800){
    lcd.print("Select button.  ");
  }
  delay(500);

}

void setup() {

  // calibration - linear interpolation
  int calibrated_adc_low = CST_STRAIN_IN_MIN;
  int calibrated_adc_high = CST_STRAIN_IN_MAX;
  int calibrated_force_low = CST_STRAIN_OUT_MIN;
  int calibrated_force_high = CST_STRAIN_OUT_MAX;


  // initialize serial communication
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.createChar(1, can);
  lcd.createChar(2, can2);
  lcd.createChar(3, can3);
  lcd.createChar(4, gordi);


  display_manual();


  // calibrate lower bounds

  calibrated_force_low = calibrate_force(calibrated_force_low, "Low");
  calibrated_adc_low = calibrate_adc("Low");

  // calibrate upper boundsS
  calibrated_force_high = calibrate_force(calibrated_force_high, "high");
  calibrated_adc_high = calibrate_adc("High");


  wsb_strain1.linearCalibration(calibrated_adc_low, calibrated_adc_high,
                                calibrated_force_low, calibrated_force_high);

  displayScreen("[A0]:", "Weight:");
}


// time management
long display_time_step = 1000;
long display_time = 0;

// force measurement & display
int strain1_adc;
int strain1_force;
int force_pos_offset;

void loop() {
  // check if it is time for a new measurement / to update the display
  if(millis() > (display_time_step + display_time)){
    // make force measurement and obtain calibrated force value
    strain1_force = wsb_strain1.measureForce();

    // obtain the raw ADC value from the last measurement
    strain1_adc = wsb_strain1.getLastForceRawADC();

    // compute force position offset (right-aligned text)
    force_pos_offset = (4 - countDigits(strain1_force));
    if(strain1_force <= 0){
      force_pos_offset -= 1;
    }

    // display raw ADC value
    lcd.setCursor(12,0); lcd.print("       ");
    lcd.setCursor(12,0); lcd.print(strain1_adc);
    Serial.print("A[0], raw ADC: ");
    Serial.println(strain1_adc, DEC);

    // display force value
    lcd.setCursor(11,1); lcd.print("     ");
    lcd.setCursor((11+force_pos_offset),1); lcd.print(strain1_force);
    Serial.print("A[0], force: ");
    Serial.println(strain1_force, DEC);

    // print empty line & reset timer
    Serial.println("");
    display_time = millis();
  }
}
