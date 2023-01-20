#include <Arduino.h>
#include <U8g2lib.h>

// U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,  /*SCL*/  22,  /*SDA*/  21,   /*reset*/  U8X8_PIN_NONE);//构造
// typedef u8g2_uint_t u8g_uint_t;
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);  // ESP32 Thing, HW I2C with pin remapping
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

#include <Ticker.h>
Ticker serTicker;
Ticker beeTicker;
#define led1 32
#define led2 33

#define but 39
#define bee 13

#define but_next 35

int step = 0;
byte getrfid[] = { 0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0x00, 0x64, 0xB0, 0x7E };
// byte getrfid[] = { 0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0x03, 0xE8, 0x37, 0x7E }; //群读
// byte getrfid[] = { 0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E };

byte setRate[] = { 0xBB, 0x00, 0xB6, 0x00, 0x02, 0x07, 0xD0, 0x8F, 0x7E };
void beee(int delayTime, int repeat) {
  for (int i = 0; i < repeat; i++) {
    analogWrite(bee, 100);
    delay(delayTime);
    analogWrite(bee, 0);
    delay(delayTime);
  }
}
void setup() {
  // put your setup code here, to run once:
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(bee, OUTPUT);
  pinMode(but, INPUT);
  pinMode(but_next, INPUT);
  Serial.begin(115200);
  Serial2.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_wqy14_t_gb2312);

  beee(100, 1);
  serTicker.attach_ms(100, []() {
    serRead();
  });
  delay(1000);
  Serial2.write(setRate, sizeof(setRate));
  Serial.println("start");
}

unsigned char incomingByte;
String value;
int countI = 0;
char incomingByteArr[1024];
char rfidArr[24];

#define rfidListDataLen 10
char* rfidListData[rfidListDataLen] = {
  "e28068940050193f17ed1f",
  "e28068940040193f17ed1d",
  "e28068940050193f17ed2f",
  "e28068940050193f17ed29",
  "e28068940040193f17ed2d",
  "e28068940040193f17ed2b",
  "e28068940050193f17ed25",
  "e28068940040193f17ed27",
  "e28068940040193f17ed21",
  "e28068940050193f17ed23"
};

#define rfidListLen 64
String rfidList[rfidListLen];
#define missListLen 20
String missList[missListLen];
int rfidListKey = 0;
bool isStart = false;
String tempVar;
bool hasData = false;
void serRead() {
  if (step != 1) {
    return;
  }
  while (Serial2.available() > 0) {
    incomingByte = Serial2.read();
    incomingByteArr[countI] = incomingByte;
    countI++;
    // value += String(incomingByte, HEX);
    // value += " ";
  }
  if (countI > 12) {
    isStart = false;
    // String tempStr;
    int j = 0;
    for (int i = 0; i < sizeof incomingByteArr; i++) {
      if (incomingByteArr[i] == 0xBB && incomingByteArr[i + 1] == 0x02 && incomingByteArr[i + 2] == 0x22) {
        // 过滤
        isStart = true;
        // tempStr = "";
      }

      // Serial.println("r2");
      if (incomingByteArr[i] == 0x7E) {
        // Serial.println(tempStr);
        isStart = false;
        j = 0;
        if (rfidArr[8] != 0x00 and rfidArr[20] != 0x00 and rfidArr[19] != 0x00) {
          // 过滤
          tempVar = "";
          for (int h = 8; h < 20; h++) {
            tempVar += String(rfidArr[h], HEX);
          }
          // 加入array
          hasData = false;
          for (int h = 0; h < sizeof rfidList; h++) {
            if (rfidList[h] == tempVar) {
              hasData = true;
            }
          }
          if (hasData == false) {
            // Serial.println("false");
            // char charBuf[50];
            // tempVar.toCharArray(charBuf, 50);
            rfidList[rfidListKey] = tempVar;
            rfidListKey++;
          }
        }
      }

      // Serial.println("r3");
      if (isStart == true) {
        rfidArr[j] = incomingByteArr[i];
        j++;
        // tempStr += String(incomingByteArr[i],HEX)+" ";
      }
    }
    memset(rfidArr, 0, sizeof rfidArr);
    memset(incomingByteArr, 0, sizeof incomingByteArr);
    countI = 0;
  }
}
int missIndex = 0;
int searchCount = 5;
int rfidCount = 0;
void loop() {
  if (step == 0) {
    // 待机
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
    u8g2.firstPage();
    do {
      u8g2.setCursor(5, 20);
      u8g2.print("RFID 准备就绪");
    } while (u8g2.nextPage());
  } else if (step == 1) {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
    u8g2.firstPage();
    do {
      u8g2.setCursor(5, 20);
      u8g2.print("搜索中..");
      u8g2.setCursor(5, 40);
      String str = "等待" + String(searchCount) + "秒";
      u8g2.print(str);
      Serial.println("step1 1");
      searchCount--;
      if (searchCount == 0) {
        step = 2;
        searchCount = 5;
        //算总数
        for (int i = 0; i < rfidListLen; i++) {
          if (rfidList[i].length() > 10) {
            Serial.println(rfidList[i]);
            rfidCount++;
          }
        }
        //找缺少
        int h = 0;
        for (int i = 0; i < rfidListDataLen; i++) {
          bool dataExist = false;
          for (int j = 0; j < rfidListLen; j++) {
            if (String(rfidListData[i]) == rfidList[j]) {
              // 有就别加了
              dataExist = true;
              break;
            }
          }
          if (dataExist == false) {
            missList[h] = String(rfidListData[i]);
            h++;
          }
        }
      }

    } while (u8g2.nextPage());
    Serial2.write(getrfid, sizeof(getrfid));
    delay(1000);
  } else if (step == 2) {
    Serial.println("step2");
    if (rfidCount == 10) {
      digitalWrite(led1, HIGH);
      digitalWrite(led2, LOW);
      beeTicker.once_ms(100, []() {
        beee(100, 2);
      });
    } else {
      digitalWrite(led1, LOW);
      digitalWrite(led2, HIGH);
      beeTicker.once_ms(100, []() {
        beee(100, 1);
      });
    }
    step = 3;
    readData();
  }
  if (digitalRead(but) == 0) {
    if (step == 3) {
      ESP.restart();
    }
    Serial.println("click");
    step = 1;
    rfidCount = 0;
    for (int i = 0; i < rfidListLen; i++) {
      rfidList[i] = "";
    }
    // rfidList[rfidListLen];
    delay(100);

  } else {
    delay(100);
  }
  if (digitalRead(but_next) == 0) {
    // 翻页
    if (step == 3) {
      missIndex++;
      readData();
    }
    delay(100);
  }
}

void readData() {
  //读取不存在的
  u8g2.firstPage();
  do {
    u8g2.setCursor(5, 20);
    String str = "搜索到" + String(rfidCount) + "个标签";
    u8g2.print(str);
    u8g2.setCursor(5, 40);
    str = String(rfidCount) + "/10";
    u8g2.print(str);

    String missBook = "";
    if (missIndex < missListLen and missList[missIndex] != "") {
      missBook = "缺少:" + booklist(missList[missIndex]);
    } else {
      missIndex = 0;
      missBook = "缺少:" + booklist(missList[missIndex]);
    }
    u8g2.setCursor(5, 60);
    u8g2.print(missBook);

  } while (u8g2.nextPage());
}
String booklist(String key) {
  if (key == "e28068940050193f17ed1f") {
    return "语文";
  } else if (key == "e28068940040193f17ed1d") {
    return "数学";
  } else if (key == "e28068940050193f17ed2f") {
    return "英语";
  } else if (key == "e28068940050193f17ed29") {
    return "体育";
  } else if (key == "e28068940040193f17ed2d") {
    return "品德";
  } else if (key == "e28068940040193f17ed2b") {
    return "化学";
  } else if (key == "e28068940050193f17ed25") {
    return "物理";
  } else if (key == "e28068940040193f17ed27") {
    return "健康";
  } else if (key == "e28068940040193f17ed21") {
    return "地理";
  } else if (key == "e28068940050193f17ed23") {
    return "生物";
  } else {
    return "未知";
  }
}
