#include <DS1302.h>
#define CLK_PIN 9
#define DATA_PIN 10
#define RST_PIN 11

DS1302 rtc(RST_PIN, DATA_PIN, CLK_PIN);
Time t;
int lightPin =A0;
bool isLightAbove50 = false;
long pre = 0;
long interval = 1000;
 
void setup()
{
  rtc.halt(false);
  rtc.writeProtect(false);
  Serial.begin(9600);
 
  //rtc.setDOW(THURSDAY);
  //rtc.setTime(21,58,00);
  //rtc.setDate(26,5,2024);   // //날짜 수정한 후 주석 처리하고 한 번 더 업로드
}
 
void loop()
{ 
  unsigned long cnt = millis();
  if(cnt - pre >= interval) {
    pre = cnt;
    checkOpen();
  }
}

void checkOpen(){
  int lightValue = analogRead(lightPin)/10;
  t = rtc.getTime();

  //Serial.println(analogRead(lightPin)/10);

  if (lightValue > 50 && !isLightAbove50) {
    Serial.print(t.year);
    Serial.print("년 ");
    Serial.print(t.mon);
    Serial.print("월 ");
    Serial.print(t.date);
    Serial.print("일 ");
    Serial.print(t.hour);
    Serial.print("시 ");
    Serial.print(t.min);
    //Serial.print(t.sec);
    Serial.print("분 금고가 열렸습니다.\n");
    isLightAbove50 = true; // 50을 초과했다는 것을 기록
  } else if (lightValue <= 50 && isLightAbove50) {
      isLightAbove50 = false;
  }
}
