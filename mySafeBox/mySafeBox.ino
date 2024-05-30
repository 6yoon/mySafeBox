#include <EEPROM.h>
#include <Keypad.h>
#include <Servo.h>
#include <DS1302.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#define CLK_PIN 9
#define DATA_PIN 10
#define RST_PIN 11
#define trigPin 13
#define echoPin 12

bool isClose15 = false;

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

DS1302 rtc(RST_PIN, DATA_PIN, CLK_PIN);
Time t;
int lightPin =A0;
bool isLightAbove50 = false;
long pre = 0;
long interval = 1000;

int motor_control = A1;
Servo servo;

int incline = 0;
bool isIncline = false;

int speakerPin = A2;
int numTones = 3;
int tones [] = {330, 349, 392};
int led = A3;

int password [3]  = {0, 0, 0};
bool isPasswordSet = false;
int pCount = 0;
int newCount = 0;
int changeCount = 0;

int isSafe = 0;
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup(){
  /* for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0); // 전체 초기화
  } */
  
  //rtc.setDOW(THURSDAY);
  //rtc.setTime(21,58,00);
  //rtc.setDate(26,5,2024);   // //날짜 수정한 후 주석 처리하고 한 번 더 업로드
  
  Serial.begin(9600);
  loadPassword();
  pCount = EEPROM.read(0);
  servo.attach(motor_control);
  servo.write(160);
  pinMode(speakerPin, OUTPUT);
  pinMode(led, OUTPUT);
  rtc.halt(false);
  rtc.writeProtect(false);
  lcd.begin();
  pinMode(incline, INPUT);
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);  
}

void loop(){
  unsigned long cnt = millis();
  if(cnt - pre >= interval) {
    pre = cnt;
    checkOpen();
    checkIncline();
    checkCome();
    changePassword();
  }
  
  //Serial.println(pCount);
/*   for (int i = 0; i < 3; i++) {
    Serial.println(password[i]);
  } */
  if(password[0] ==0 && password[1] == 0 && password[2] == 0){
      if (!isPasswordSet) {
        setPassword();
        savePassword();
        isPasswordSet = true;
      }
      else {
        isPasswordSet = false;
      }
	}
  char key = keypad.getKey();
  if(key != NO_KEY){
    if(key == '#'){
      inputPassword();
    }
  }
}

void setPassword(){
  Serial.println("설정할 비밀번호를 한 자리씩 입력하시오");
  while (1) {
    if (Serial.available()) {
      int pNum = Serial.parseInt();
      Serial.println(pNum);
      password[pCount] = pNum;
      pCount++;
      if (pCount == 3) {
        Serial.println( "비밀번호 설정 완료\n아두이노를 다시 업로드 하십시오");
        updatePCount(pCount);
        break;
      }
    }
  }
}

void savePassword(){
  for (int i = 0; i < 3; i++) {
    EEPROM.write(i+1, password[i]); // EEPROM에 비밀번호 저장
  }
}

void loadPassword() {
  for (int i = 0; i < 3; i++) {
    password[i] = EEPROM.read(i+1); // EEPROM에서 비밀번호 불러오기
  }
}

void changePassword(){
  if(pCount == 3) pCount -= 1;
  //Serial.println(pCount);
	while (1) {
	  if (Serial.available()) {
	    int rNum = Serial.parseInt();
	    Serial.println(rNum);
		  if(rNum == password[pCount]){
		    changeCount++;
        pCount--;
      }
	    if (changeCount == 3) {
        pCount++;
        Serial.println( "비밀번호 재설정을 시작합니다\n");
        for (int i = 0; i < 3; i++) {
          password[i] = 0;
        }
        savePassword();
        updatePCount(pCount);
        break;
	    }
	  }else break;
  }
}

void updatePCount(int newPCount) {
  EEPROM.write(0, newPCount); // 주소 0에 새로운 pCount 값 저장
}

void inputPassword(){
  int i = 0;
	while(1){
    char key = keypad.getKey();
    if (key != NO_KEY){
		  Serial.println(key);
      if(key != '*'){
        lcd.setCursor(i, 0); 
        lcd.print(key);
      } 
      if(key - '0' == password[i]){
        Serial.println("냠");
        isSafe++;
      }
      i++;
      Serial.println(i);
      Serial.println(isSafe);
      if(i == 4 && key == '*') break;
    }
  }
  if(isSafe == 3){
	  Serial.println("맞는 비밀번호 입니다");
    lcd.setCursor(0, 1); 
	  lcd.print("correct password");
    servo.write(45);
    for (int i = 0; i < numTones; i++){
      tone(speakerPin, tones[i]);
      delay(100);
    }
    noTone(speakerPin);
    isSafe = 0;
    delay(3000);
    lcd.clear();
  }
	else{
	  Serial.println("틀린 비밀번호 입니다");
	  lcd.setCursor(0, 1); 
	  lcd.print("wrong password");
    for(int i = 0; i < 5; i++){
      digitalWrite(speakerPin, HIGH);
      digitalWrite(led, HIGH);
      delay(1000);
      digitalWrite(speakerPin, LOW);
      digitalWrite(led, LOW);
      delay(500);
    }
    isSafe = 0;
    lcd.clear();
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
      servo.write(160);
      isLightAbove50 = false;
  }
}

void checkIncline(){
  //Serial.println(digitalRead(incline));
  if(digitalRead(incline) == LOW && !isIncline ){
    Serial.println("누군가 금고를 옮기고 있습니다");
    isIncline = true;
  }
  else if(digitalRead(incline) == HIGH && isIncline){
    isIncline = false;
  }
  delay(100);
}

long microsecondsToCentimeters(long microseconds)
{
    return microseconds / 29 / 2;
}

void checkCome(){
	long duration, cm;

	digitalWrite(trigPin, LOW);
	delayMicroseconds(2); 
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10); 
	digitalWrite(trigPin, LOW);
	duration = pulseIn(echoPin, HIGH); 
	
	cm = microsecondsToCentimeters(duration);

  if (cm <= 15 && !isClose15) {
		Serial.println("누군가 금고에 접근했습니다");
    isClose15 = true;
  } else if (cm > 15  && isClose15) {
    isClose15 = false;
  }
	delay(100);   
}