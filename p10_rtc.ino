#include <avr/io.h>
#include <avr/interrupt.h>

#include <SPI.h>
#include <DMD.h>
#include <Arial14.h>
#include <Arial_black_16.h>
#include <Arial_Black_16_ISO_8859_1.h>
#include <SystemFont5x7.h>

#include <RTClib.h>
#include <util/delay.h>

#define ROW 1
#define COL 1

DMD disp(ROW, COL);
RTC_DS3231 rtc;

volatile unsigned int update_count = 0, switch_count = 0, beep_count = 0;
int switch_limit = 3000; //123s, Time/Date switch delay
bool beep_mode = false, buzzer_mode = false;
String prev_date;
DateTime rtc_now;
char nameoftheday[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char month_name[12][10] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
char hr[3], min[3], sec[3], date_char[30];
char beep_case [4][6] = {"6:35", "6:37", "6:39", "6:41"};
int i = 32 + 5, beep_i = 0, beep_limit = 110;//4.5s
int switch_state = 1;

int main() {

  Serial.begin(9600);
  TCCR1B = 0x01; //clock source with no prescaler
  TIMSK1 = 0x01; //Enable Timer1 interrupt
  DDRD |= 0x20;
  //PORTD |= 0x20;

  sei();
  disp.scanDisplayBySPI();
  disp.clearScreen(true);


  if (!rtc.begin()) {
    //warn("NO RTC");
  }

  if (!rtc.lostPower()) {
    //warn("RESET");
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  while (1) {
    normal_mode();

    beep_switch();

    buzzer_switch();

    buzzer_ctrl();
  }
  return 0;
}

ISR(TIMER1_OVF_vect) {
  disp.scanDisplayBySPI();
  update_count++;
  switch_count++;
  beep_count++;
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
  date += String(nameoftheday[rtc_now.dayOfTheWeek()]) + " ";//11 char
  date += String(rtc_now.day()) + " ";//2 char
  date += String(month_name[rtc_now.month()]) + " ";//11 char
  date += String(rtc_now.year());//4 char
  date.toCharArray(date_char, date.length() + 2);
  //Serial.println(date);
  date = "";

  disp.selectFont(SystemFont5x7);
  disp.drawString(i, 4, date_char, strlen(date_char), GRAPHICS_NORMAL);

  if (i <= ~200) {
    disp.clearScreen(true);
  }
}

void normal_mode() { //Time/Date display
  //1s delay (Non block delay)
  if (update_count >= 24) {
    rtc_now = rtc.now();
    //Serial.println(rtc_now.timestamp());
    if (switch_state == 1) dispTime();
    else if (switch_state == 2) dispDate();
    update_count = 0;
  }

  //Switch between Time/Date display
  if (switch_count >= switch_limit) {
    if (switch_state == 1) {
      i = 32 + 5;
      switch_state = 2;
      switch_limit = 4000;//164s
      disp.clearScreen(true);
    }
    else if (switch_state == 2) {
      switch_state = 1;
      switch_limit = 8000;//328s
      disp.clearScreen(true);
    } else if (switch_state == 3) {
      switch_state = 1;
      switch_limit = 8000;//328s
      disp.clearScreen(true);
    }
    switch_count = 0;
  }
}

void beep_switch() {
  //Serial.println("working");
  if (Serial.available() > 0) {
    char d = Serial.read();
    //    Serial.println(d);
    if (!beep_mode & d == 'E') {
      beep_mode = true;

      switch_state = 3;
      switch_limit = 2000; //98.4s
      disp.clearScreen(true);
      disp.selectFont(SystemFont5x7);
      disp.drawString(5, 0, "EXAM", 4, GRAPHICS_NORMAL);
      disp.drawString(5, 9, "MODE", 4, GRAPHICS_NORMAL);

    } else if (beep_mode & d == 'N') {
      beep_mode = false;
      PORTD = 0;
      switch_state = 3;
      switch_limit = 2000; //98.4s
      disp.clearScreen(true);
      disp.selectFont(SystemFont5x7);
      disp.drawString(0, 0, "NORMAL", 6, GRAPHICS_NORMAL);
      disp.drawString(5, 9, "MODE", 4, GRAPHICS_NORMAL);

    }
  }
}

void buzzer_switch() {
  String beep_time;
  beep_time += String(rtc_now.twelveHour()) + ":" + String(rtc_now.minute());
  //  Serial.println(beep_time);
  if (beep_mode & (prev_date != beep_time)) {
    for (int i = 0; i <= 3; i++) {
      if (beep_time == String(beep_case[i])) {
        buzzer_mode = true;
        beep_mode = false;
        prev_date = beep_time;
        beep_limit = 110;
        Serial.println("BEEP ON 1");
        PORTD = 0x20;
        break;
        //        Serial.print("Found: ");
        //        Serial.println(String(beep_case[i]));
      }
    }
  }
  beep_time = "";
}

void buzzer_ctrl() {
  if (beep_count >= beep_limit) {//4s delay
    if (buzzer_mode) {
      if (beep_i == 0) {
        PORTD = 0;
        beep_limit = 98;
        Serial.println("BEEP OFF 1");
      } else if (beep_i == 1) {
        PORTD = 0x20;
        Serial.println("BEEP ON 2 AND BEEP_I 1");
      } else if (beep_i == 2) {
        PORTD = 0;
        Serial.println("BEEP OFF 2 AND BEEP_I 2");
        beep_mode = true;
        buzzer_mode = false;
        beep_i = 0;
      }
      if (buzzer_mode)beep_i++;
    }
    beep_count = 0;
  }
}
