#include <avr/io.h>
#include <avr/interrupt.h>

#include <SPI.h>
#include <DMD.h>
#include <Arial14.h>
#include <Arial_black_16.h>
#include <Arial_Black_16_ISO_8859_1.h>
#include <SystemFont5x7.h>

#include <RTClib.h>

#define ROW 1
#define COL 1

DMD disp(ROW, COL);
RTC_DS3231 rtc;

volatile unsigned int update_count = 0;
volatile unsigned int switch_count = 0;
int switch_limit = 3000; //123s
bool switch_state = true, mode = false, beep_mode = false;
DateTime rtc_now;
char nameoftheday[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char month_name[12][12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
char hr[3], min[3], sec[3], date_char[26];
char beep_case [4][6] = {"12:10", "11:55", "3:55", "4:55"};
int i = 32 + 5, beep_i = 0, beep_case_i = 0;

int main() {

  TCCR1B = 0x01; //clock source with no prescaler
  TIMSK1 = 0x01; //Enable Timer1 interrupt

  sei();
  Serial.begin(9600);
  Serial.println("WORKING");
  disp.scanDisplayBySPI();
  disp.clearScreen(true);   //true is normal (all pixels off), false is negative (all pixels on);

  if (!rtc.begin()) {
    //warn("NO RTC");
  }

  if (!rtc.lostPower()) {
    //warn("RESET");
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  while (1) {
    //Serial.println("loop");
    if (update_count >= 24) { //1s non block delay
    rtc_now = rtc.now();
    //Serial.println(rtc_now.timestamp());
    if (switch_state) dispTime();
    else dispDate();
    update_count = 0;
  }
    normal();
//    exam();

    if (Serial.available()) {
      Serial.println(Serial.readString());
      if (Serial.readString() == "EXAM_MODE") {
        mode = true;
      }
      else if (Serial.readString() == "RESET_MODE") {
        mode = false;
      }
    }

  }
  return 0;
}

ISR(TIMER1_OVF_vect) {
  disp.scanDisplayBySPI();
  update_count++;
  switch_count++;
  beep_i++;
}

void warn(char *warn) {
  disp.selectFont(SystemFont5x7);
  disp.drawString(1, 1, warn, 1, GRAPHICS_NORMAL);
}

void dispTime() {
  disp.selectFont(ArialBlackFont16);

  String(rtc_now.twelveHour()).toCharArray(hr, 3);
  disp.drawString(1, 1, hr, 2, GRAPHICS_NORMAL);
  if (rtc_now.twelveHour() < 10) {
    disp.drawString(1, 1, "0", 1, GRAPHICS_NORMAL);
    disp.drawString(10, 1, hr, 1, GRAPHICS_NORMAL);
  }
  else {
    disp.drawString(1, 1, hr, 2, GRAPHICS_NORMAL);
  }

  disp.selectFont(SystemFont5x7);

  String(rtc_now.minute()).toCharArray(min, 3);
  if (rtc_now.minute() < 10) {
    disp.drawString(19, 0, "0", 1, GRAPHICS_NORMAL);
    disp.drawString(25, 0, min, 1, GRAPHICS_NORMAL);
  }
  else {
    disp.drawString(19, 0, min, 2, GRAPHICS_NORMAL);
  }

  String(rtc_now.second()).toCharArray(sec, 3);
  if (rtc_now.second() < 10) {
    disp.drawString(19, 9, "0", 1, GRAPHICS_NORMAL);
    disp.drawString(25, 9, sec, 1, GRAPHICS_NORMAL);
  }
  else {
    disp.drawString(19, 9, sec, 2, GRAPHICS_NORMAL);
  }
}

void dispDate() {
  i--;
  String date = "";
  //Serial.println(nameoftheday[rtc_now.dayOfTheWeek()]);
  date += String(nameoftheday[rtc_now.dayOfTheWeek()]) + " ";
  date += String(rtc_now.day()) + " ";
  date += String(month_name[rtc_now.month()]) + " ";
  date += String(rtc_now.year());
  date.toCharArray(date_char, date.length() + 2);
  //Serial.println(date);
  date = "";

  disp.selectFont(SystemFont5x7);
  disp.drawString(i, 4, date_char, strlen(date_char), GRAPHICS_NORMAL);

  if (i <= ~200) {
    disp.clearScreen(true);
  }
}

void normal() {

  if (switch_count == switch_limit) {
    if (switch_state) {
      i = 32 + 5;
      switch_state = false;
      switch_limit = 4000;//123s
      disp.clearScreen(true);
    }
    else {
      switch_state = true;
      switch_limit = 6000;//185s
      disp.clearScreen(true);
    }
    switch_count = 0;
  }
}

//void exam() {
//
//  String beep_case_var = "";
//  if (mode) {
//    beep_case_var += String(rtc_now.twelveHour()) + ":" + String(rtc_now.minute());
//    for (int y = 0; y < 4; y++) {
//      if (String(beep_case[y][6]) == beep_case_var) {
//        //ON Beep
//        Serial.println("Beep ON");
//        beep_mode = true;
//        beep_case_i = 0;
//      }
//    }
//  }
//
//  if ((beep_i == 49) & beep_mode) {
//    //OFF Beep
//    Serial.println("Beep OFF");
//    if (beep_case_i == 1) {
//      //ON Beep
//      Serial.println("Beep ON");
//      //beep_mode = false;
//    }
//    if (beep_case_i == 2) beep_mode = false;
//
//    beep_case_i++;
//    beep_i = 0;
//  }
//}
