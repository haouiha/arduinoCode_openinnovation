#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <BH1750.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <StreamUtils.h>

//////////////////////////////////////////////

byte STX = 02;
byte ETX = 03;

uint8_t START_PATTERN[3] = {0, 111, 222};

//https://arduinojson.org/v6/assistant/
const size_t capacity = JSON_OBJECT_SIZE(7) + 320;

DynamicJsonDocument jsonDoc(capacity);

String mqtt_server ,
       mqtt_Client ,
       mqtt_password ,
       mqtt_username ,
       password ,
       mqtt_port,
       ssid ;


char msg[5000];

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Adafruit_SHT31 sht31 = Adafruit_SHT31();
BH1750 lightMeter;


float outputValueMin = 40;  //the lowest value that comes out of the potentiometer
float outputValueMax = 440; //the highest value that comes out of the potentiometer.
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

int sensorValue = 0;  // value read from the pot
int outputValue = 0;  // value to output to a PWM pin

int Relay0 = 25, Relay1 = 4, Relay2 = 16 , Relay3 = 17;
int LEDR = 26, LEDY = 27;   //LED 26= Blue ,LED 27 = Yenllow

int max_soil0, max_soil1, max_soil2, max_soil3 = 0;
int min_soil0, min_soil1, min_soil2, min_soil3 = 0;

int max_temp0, max_temp1, max_temp2, max_temp3 = 0;
int min_temp0, min_temp1, min_temp2, min_temp3 = 0;

int check_relay0, check_relay1, check_relay2, check_relay3 = 0;

int open_by_m0, open_by_m1, open_by_m2, open_by_m3 = 0;

//sensor soil status
int open_by_s00, open_by_s01, open_by_s02, open_by_s03 = 0;

//sensor temp status
int open_by_s10, open_by_s11, open_by_s12, open_by_s13 = 0;


int open_by_t0, open_by_t1, open_by_t2, open_by_t3 = 0;
int value_timer0, value_timer1, value_timer2, value_timer3 = 0; //value timer(6)  ex. {value_timer0:{1},{2},{3},{4},{5},{6}}     {1-6}= unix time format
int status_timer0, status_timer1, status_timer2, status_timer3 = 0;   //actve inactive

int status_error = 0; // check bug error

const unsigned long eventInterval = 2000;
const unsigned long reconnectInterval = 5000;
unsigned long previousTime = 0;

int notify_status = 0;
int notify_relay0, notify_relay1, notify_relay2, notify_relay3 = 0;

int text = 0;   // text to show HDS

float soil = 0;
float temp = 0;
float humidity = 0;
float lux = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == "@msg/notify_status") {
    if (message == "0" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_status\" : \"0\"}}");
      Serial.println("notify_status = 0");
      notify_status = 0;
    }
    else if (message == "1") {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_status\" : \"1\"}}");
      Serial.println("notify_status = 1");
      notify_status = 1;
    }
  }


  if (String(topic) == "@msg/notify_relay0") {
    if (message == "0" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay0\" : \"0\"}}");
      Serial.println("notify_relay0 = 0");
      notify_relay0 = 0;
    }
    else if (message == "1") {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay0\" : \"1\"}}");
      Serial.println("notify_relay0 = 1");
      notify_relay0 = 1;
    }
  }


  if (String(topic) == "@msg/notify_relay1") {
    if (message == "0" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay1\" : \"0\"}}");
      Serial.println("notify_relay1 = 0");
      notify_relay1 = 0;
    }
    else if (message == "1") {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay1\" : \"1\"}}");
      Serial.println("notify_relay1 = 1");
      notify_relay1 = 1;
    }
  }


  if (String(topic) == "@msg/notify_relay2") {
    if (message == "0" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay2\" : \"0\"}}");
      Serial.println("notify_relay2 = 0");
      notify_relay2 = 0;
    }
    else if (message == "1") {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay2\" : \"1\"}}");
      Serial.println("notify_relay2 = 1");
      notify_relay2 = 1;
    }
  }

  if (String(topic) == "@msg/notify_relay3") {
    if (message == "0" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay3\" : \"0\"}}");
      Serial.println("notify_relay2 = 0");
      notify_relay3 = 0;
    }
    else if (message == "1") {
      client.publish("@shadow/data/update", "{\"data\" : {\"notify_relay3\" : \"1\"}}");
      Serial.println("notify_relay2 = 1");
      notify_relay3 = 1;
    }
  }

  //--------------------------------------------------------------------------------------//
  if (String(topic) == "@msg/led0") {
    if (message == "on" ) {
      digitalWrite(Relay0, 1);
      client.publish("@shadow/data/update", "{\"data\" : {\"led0\" : \"on by manual\"}}");
      Serial.println("Relay Manual ON");
      check_relay0 = 1;
      open_by_m0 = 1;
    }
    else if (message == "off") {
      digitalWrite(Relay0, 0);
      client.publish("@shadow/data/update", "{\"data\" : {\"led0\" : \"off by manual\"}}");
      Serial.println("Relay Manual OFF");
      check_relay0 = 0;
      open_by_m0 = 0;
    }
  }

  if (String(topic) == "@msg/led1") {
    if (message == "on" ) {
      digitalWrite(Relay1, 1);
      client.publish("@shadow/data/update", "{\"data\" : {\"led1\" : \"on by manual\"}}");
      Serial.println("Relay Manual ON");
      check_relay1 = 1;
      open_by_m1 = 1;
    }
    else if (message == "off") {
      digitalWrite(Relay1, 0);
      client.publish("@shadow/data/update", "{\"data\" : {\"led1\" : \"off by manual\"}}");
      Serial.println("Relay Manual OFF");
      check_relay1 = 0;
      open_by_m1 = 0;
    }
  }

  if (String(topic) == "@msg/led2") {
    if (message == "on" ) {
      digitalWrite(Relay2, 1);
      client.publish("@shadow/data/update", "{\"data\" : {\"led2\" : \"on by manual\"}}");
      Serial.println("Relay Manual ON");
      check_relay2 = 1;
      open_by_m2 = 1;
    }
    else if (message == "off") {
      digitalWrite(Relay2, 0);
      client.publish("@shadow/data/update", "{\"data\" : {\"led2\" : \"off by manual\"}}");
      Serial.println("Relay Manual OFF");
      check_relay2 = 0;
      open_by_m2 = 0;
    }
  }

  if (String(topic) == "@msg/led3") {
    if (message == "on" ) {
      digitalWrite(Relay3, 1);
      client.publish("@shadow/data/update", "{\"data\" : {\"led3\" : \"on by manual\"}}");
      Serial.println("Relay Manual ON");
      check_relay3 = 1;
      open_by_m3 = 1;
    }
    else if (message == "off") {
      digitalWrite(Relay3, 0);
      client.publish("@shadow/data/update", "{\"data\" : {\"led3\" : \"off by manual\"}}");
      Serial.println("Relay Manual OFF");
      check_relay3 = 0;
      open_by_m3 = 0;
    }
  }

  //--------------------------------------------------------------------------------------//
  if (String(topic) == "@msg/status_timer0") {
    if (message == "active" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer0\" : \"status_timer0 = active\"}}");
      status_timer0 = 1;
    }
    else if (message == "inactive") {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer0\" : \"status_timer0 = in active\"}}");
      status_timer0 = 0;
    }
  }

  if (String(topic) == "@msg/status_timer1") {
    if (message == "active" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer1\" : \"status_timer1 = active\"}}");
      status_timer1 = 1;
    }
    else if (message == "inactive") {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer1\" : \"status_timer1 = in active\"}}");
      status_timer1 = 0;
    }
  }

  if (String(topic) == "@msg/status_timer2") {
    if (message == "active" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer2\" : \"status_timer2 = active\"}}");
      status_timer2 = 1;
    }
    else if (message == "inactive") {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer2\" : \"status_timer2 = in active\"}}");
      status_timer2 = 0;
    }
  }

  if (String(topic) == "@msg/status_timer3") {
    if (message == "active" ) {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer3\" : \"status_timer3 = active\"}}");
      status_timer3 = 1;
    }
    else if (message == "inactive") {
      client.publish("@shadow/data/update", "{\"data\" : {\"status_timer3\" : \"status_timer3 = in active\"}}");
      status_timer3 = 0;
    }
  }

  //--------------------------------------------------------------------------------------//
  //  if (String(topic) == "@msg/value_timer0") {
  //    value_timer0 = message.toInt();
  //    Serial.println("value_timer0");
  //    Serial.println(value_timer0);
  //  } else {
  //    Serial.println("ERROR function value timer0");
  //  }
  //
  //  if (String(topic) == "@msg/value_timer1") {
  //    value_timer1 = message.toInt();
  //    Serial.println("value_timer1");
  //    Serial.println(value_timer1);
  //  } else {
  //    Serial.println("ERROR function value timer1");
  //  }
  //
  //  if (String(topic) == "@msg/value_timer2") {
  //    value_timer2 = message.toInt();
  //    Serial.println("value_timer2");
  //    Serial.println(value_timer2);
  //  } else {
  //    Serial.println("ERROR function value timer2");
  //  }
  //
  //  if (String(topic) == "@msg/value_timer3") {
  //    value_timer3 = message.toInt();
  //    Serial.println("value_timer3");
  //    Serial.println(value_timer3);
  //  } else {
  //    Serial.println("ERROR function value timer3");
  //  }

  //--------------------------------------------------------------------------------------//
  if (String(topic) == "@msg/max_soil0") {
    max_soil0 = message.toInt();
    Serial.println("Max_soil Change!");
    Serial.println(max_soil0);
  }
  else if (String(topic) == "@msg/min_soil0") {
    min_soil0 = message.toInt();
    Serial.println("Min_soil Change!");
    Serial.println(min_soil0);
  }

  if (String(topic) == "@msg/max_soil1") {
    max_soil1 = message.toInt();
    Serial.println("Max_soil Change!");
    Serial.println(max_soil1);
  }
  else if (String(topic) == "@msg/min_soil1") {
    min_soil1 = message.toInt();
    Serial.println("Min_soil Change!");
    Serial.println(min_soil1);
  }

  if (String(topic) == "@msg/max_soil2") {
    max_soil2 = message.toInt();
    Serial.println("Max_soil Change!");
    Serial.println(max_soil2);
  }
  else if (String(topic) == "@msg/min_soil2") {
    min_soil2 = message.toInt();
    Serial.println("Min_soil Change!");
    Serial.println(min_soil2);
  }

  if (String(topic) == "@msg/max_soil3") {
    max_soil3 = message.toInt();
    Serial.println("Max_soil Change!");
    Serial.println(max_soil3);
  }
  else if (String(topic) == "@msg/min_soil3") {
    min_soil3 = message.toInt();
    Serial.println("Min_soil Change!");
    Serial.println(min_soil3);
  }


  //--------------------------------------------------------------------------------------//
  if (String(topic) == "@msg/max_temp0") {
    max_temp0 = message.toInt();
    Serial.println("max_temp Change!");
    Serial.println(max_temp0);
  }
  else if  (String(topic) == "@msg/min_temp0") {
    min_temp0 = message.toInt();
    Serial.println("min_temp Change!");
    Serial.println(min_temp0);
  }

  if (String(topic) == "@msg/max_temp1") {
    max_temp1 = message.toInt();
    Serial.println("max_temp Change!");
    Serial.println(max_temp1);
  }
  else if  (String(topic) == "@msg/min_temp1") {
    min_temp1 = message.toInt();
    Serial.println("min_temp Change!");
    Serial.println(min_temp1);
  }

  if (String(topic) == "@msg/max_temp2") {
    max_temp2 = message.toInt();
    Serial.println("max_temp Change!");
    Serial.println(max_temp2);
  }
  else if  (String(topic) == "@msg/min_temp2") {
    min_temp2 = message.toInt();
    Serial.println("min_temp Change!");
    Serial.println(min_temp2);
  }

  if (String(topic) == "@msg/max_temp3") {
    max_temp3 = message.toInt();
    Serial.println("max_temp Change!");
    Serial.println(max_temp3);
  }
  else if  (String(topic) == "@msg/min_temp3") {
    min_temp3 = message.toInt();
    Serial.println("min_temp Change!");
    Serial.println(min_temp3);
  }

}

//-------------------------------------------------------------------------------------------------------//

void call_function_relay0() {
  if (open_by_s10 == 0 && open_by_t0 == 0 && open_by_m0 == 0) {
    check0();
    open_by_soil0();
  } else if (open_by_s00 == 0 && open_by_t0 == 0 && open_by_m0 == 0) {
    check0();
    open_by_temp0();
  } else if (open_by_s10 == 0 && open_by_s00 == 0 && open_by_m0 == 0) {
    check0();
    open_by_timer0();
  } else if (open_by_m0 == 1) {
    check0();
    Serial.println("function call relay 0 on by Manual  ");
  } else {
    Serial.println("function call relay 0 error ");
  }
}


void call_function_relay1() {
  if (open_by_s11 == 0 && open_by_t1 == 0 && open_by_m1 == 0) {
    open_by_soil1();
  } else if (open_by_s01 == 0 && open_by_t1 == 0 && open_by_m1 == 0) {
    open_by_temp1();
  } else if (open_by_s11 == 0 && open_by_s01 == 0 && open_by_m1 == 0) {
    open_by_timer1();
  } else if (open_by_m1 == 1) {
    Serial.println("function call relay 1 on by Manual ");
  } else {
    Serial.println("function call relay 1 error ");
  }
}


void call_function_relay2() {
  if (open_by_s12 == 0 && open_by_t2 == 0 && open_by_m2 == 0) {
    open_by_soil2();
  } else if (open_by_s02 == 0 && open_by_t2 == 0 && open_by_m2 == 0) {
    open_by_temp2();
  } else if (open_by_s12 == 0 && open_by_s02 == 0 && open_by_m2 == 0) {
    open_by_timer2();
  } else if (open_by_m2 == 1) {
    Serial.println("function call relay 2 on by Manual ");
  } else {
    Serial.println("function call relay 2 error ");
  }
}


void call_function_relay3() {
  if (open_by_s13 == 0 && open_by_t3 == 0 && open_by_m3 == 0) {
    open_by_soil3();
  } else if (open_by_s03 == 0 && open_by_t3 == 0 && open_by_m3 == 0) {
    open_by_temp3();
  } else if (open_by_s13 == 0 && open_by_s03 == 0 && open_by_m3 == 0) {
    open_by_timer3();
  } else if (open_by_m3 == 1) {
    Serial.println("function call relay 3 on by Manual ");
  } else {
    Serial.println("function call relay 3 error ");
  }
}

void check0() {
  if (open_by_m0 == 1) {
    open_by_s00 = 0;
    open_by_s10 = 0;
    open_by_t0 = 0;
  }
  else if (open_by_s00 == 1) {
    open_by_m0 = 0;
    open_by_s10 = 0;
    open_by_t0 = 0;
  }
  else if (open_by_s10 == 1) {
    open_by_m0 = 0;
    open_by_s00 = 0;
    open_by_t0 = 0;
  } else if (open_by_t0 == 1) {
    open_by_m0 = 0;
    open_by_s00 = 0;
    open_by_s10 = 0;
  } else {
    //    Serial.println("Relay 0 All Status = 0 ");
  }
}

//-------------------------------------------------------------------------------------------------------//
//void open_timer00() {
//  if (String timeClient.getEpochTime() = timer1) {
//    Serial.print("555555555555555555555555555555555555555");
//    digitalWrite(Relay0, 1);
//  } else if (String timeClient.getEpochTime() != timer2) {
//    Serial.print("555555555555555555555555555555555555555");
//    digitalWrite(Relay0, 0);
//  } else {
//    Serial.println("function open_timer00 error");
//  }
//}

//void open_timer01() {
//  if (real_time <= timer3) {
//    digitalWrite(relay0, 0);
//  }
//  else if (timer3 <= real_time <= timer4) {
//    digitalWrite(relay0, 1)
//  } else if (timer4 <= real_time) {
//    digitalWrite(relay0, 0)
//  } else {
//    Serial.println("function open_timer01 error");
//  }
//}
//
//void open_timer02() {
//  if (real_time <= timer5) {
//    digitalWrite(relay0, 0);
//  }
//  else if (timer5 <= real_time <= timer6) {
//    digitalWrite(relay0, 1)
//  } else if (timer6 <= real_time) {
//    digitalWrite(relay0, 0)
//  } else {
//    Serial.println("function open_timer02 error");
//  }
//}
//
//void open_timer10() {
//  if (real_time <= timer1) {
//    digitalWrite(relay1, 0);
//  }
//  else if (timer1 <= real_time <= timer2) {
//    digitalWrite(relay1, 1)
//  } else if (timer2 <= real_time) {
//    digitalWrite(relay1, 0)
//  } else {
//    Serial.println("function open_timer10 error");
//  }
//}
//
//void open_timer11() {
//  if (real_time <= timer3) {
//    digitalWrite(relay1, 0);
//  }
//  else if (timer3 <= real_time <= timer4) {
//    digitalWrite(relay1, 1)
//  } else if (timer4 <= real_time) {
//    digitalWrite(relay1, 0)
//  } else {
//    Serial.println("function open_timer11 error");
//  }
//}
//
//void open_timer12() {
//  if (real_time <= timer5) {
//    digitalWrite(relay1, 0);
//  }
//  else if (timer5 <= real_time <= timer6) {
//    digitalWrite(relay1, 1)
//  } else if (timer6 <= real_time) {
//    digitalWrite(relay1, 0)
//  } else {
//    Serial.println("function open_timer12 error");
//  }
//}
//
//
//void open_timer20() {
//  if (real_time <= timer1) {
//    digitalWrite(relay2, 0);
//  }
//  else if (timer1 <= real_time <= timer2) {
//    digitalWrite(relay2, 1)
//  } else if (timer2 <= real_time) {
//    digitalWrite(relay2, 0)
//  } else {
//    Serial.println("function open_timer20 error");
//  }
//}
//
//void open_timer21() {
//  if (real_time <= timer3) {
//    digitalWrite(relay2, 0);
//  }
//  else if (timer3 <= real_time <= timer4) {
//    digitalWrite(relay2, 1)
//  } else if (timer4 <= real_time) {
//    digitalWrite(relay2, 0)
//  } else {
//    Serial.println("function open_timer21 error");
//  }
//}
//
//void open_timer22() {
//  if (real_time <= timer5) {
//    digitalWrite(relay2, 0);
//  }
//  else if (timer5 <= real_time <= timer6) {
//    digitalWrite(relay2, 1)
//  } else if (timer6 <= real_time) {
//    digitalWrite(relay2, 0)
//  } else {
//    Serial.println("function open_timer22 error");
//  }
//}
//
//
//void open_timer30() {
//  if (real_time <= timer1) {
//    digitalWrite(relay3, 0);
//  }
//  else if (timer1 <= real_time <= timer2) {
//    digitalWrite(relay3, 1)
//  } else if (timer2 <= real_time) {
//    digitalWrite(relay3, 0)
//  } else {
//    Serial.println("function open_timer30 error");
//  }
//}
//
//void open_timer31() {
//  if (real_time <= timer3) {
//    digitalWrite(relay3, 0);
//  }
//  else if (timer3 <= real_time <= timer4) {
//    digitalWrite(relay3, 1)
//  } else if (timer4 <= real_time) {
//    digitalWrite(relay3, 0)
//  } else {
//    Serial.println("function open_timer31 error");
//  }
//}
//
//void open_timer32() {
//  if (real_time <= timer5) {
//    digitalWrite(relay3, 0);
//  }
//  else if (timer5 <= real_time <= timer6) {
//    digitalWrite(relay3, 1)
//  } else if (timer6 <= real_time) {
//    digitalWrite(relay3, 0)
//  } else {
//    Serial.println("function open_timer32 error");
//  }
//}
//--------------------------------------------------------------------------------------------------//
void CheckTimer_relay0() {
  //  open_timer00();
  //  open_timer01();
  //  open_timer02();
}

void CheckTimer_relay1() {
  //  open_timer10();
  //  open_timer11();
  //  open_timer12();
}

void CheckTimer_relay2() {
  //  open_timer20();
  //  open_timer21();
  //  open_timer22();
}

void CheckTimer_relay3() {
  //  open_timer30();
  //  open_timer31();
  //  open_timer32();
}


void open_by_timer0() {
  if (status_timer0 == 1 && open_by_m0 == 0 && open_by_s00 == 0 && open_by_s10 == 0) {
    CheckTimer_relay0();
  }
  else if (status_timer0 == 0) {
    open_by_t0 = 0;
  } else {
    Serial.println("function error timer0");
  }
}

void open_by_timer1() {
  if (status_timer1 == 1 && open_by_m1 == 0 && open_by_s01 == 0 && open_by_s11 == 0) {
    CheckTimer_relay1();
  }
  else if (status_timer1 == 0) {
    open_by_t1 = 0;
  } else {
    Serial.println("function error timer1");
  }
}

void open_by_timer2() {
  if (status_timer2 == 1 && open_by_m2 == 0 && open_by_s02 == 0 && open_by_s12 == 0) {
    CheckTimer_relay2();
  }
  else if (status_timer2 == 0) {
    open_by_t2 = 0;
  } else {
    Serial.println("function error timer2");
  }
}

void open_by_timer3() {
  if (status_timer3 == 1 && open_by_m3 == 0 && open_by_s03 == 0 && open_by_s13 == 0) {
    CheckTimer_relay3();
  }
  else if (status_timer3 == 0) {
    open_by_t3 = 0;
  } else {
    Serial.println("function error timer3");
  }
}

void open_by_temp0() {
  if (open_by_s00 == 0 && open_by_t0 == 0 && open_by_m0 == 0) {
    CheckSensor_Relay_temp0();
  } else {
    Serial.println("ERROR function sensor temp0");
    status_error = 510;
  }
}

void open_by_temp1() {
  if (open_by_s01 == 0 && open_by_t1 == 0 && open_by_m1 == 0) {
    CheckSensor_Relay_temp1();
  } else {
    Serial.println("ERROR function sensor temp1");
    status_error = 511;
  }
}

void open_by_temp2() {
  if (open_by_s02 == 0 && open_by_t2 == 0 && open_by_m2 == 0) {
    CheckSensor_Relay_temp2();
  } else {
    Serial.println("ERROR function sensor temp2");
    status_error = 512;
  }
}

void open_by_temp3() {
  if (open_by_s03 == 0 && open_by_t3 == 0 && open_by_m3 == 0) {
    CheckSensor_Relay_temp3();
  } else {
    Serial.println("ERROR function sensor temp3");
    status_error = 513;
  }
}

//-------------------------------------------------------------------------------------------------------//
void open_by_soil0() {
  if (open_by_m0 == 0 && open_by_s10 == 0 && open_by_t0 == 0 ) {
    CheckSensor_Relay_soil0();
  }
  else {
    Serial.println("ERROR soil0");
    status_error = 500;
  }
}

void open_by_soil1() {
  if (open_by_m1 == 0 && open_by_s11 == 0 && open_by_t1 == 0) {
    CheckSensor_Relay_soil1();
  }
  else {
    Serial.println("ERROR soil1");
    status_error = 501;
  }
}

void open_by_soil2() {
  if (open_by_m2 == 0 && open_by_s12 == 0 && open_by_t2 == 0) {
    CheckSensor_Relay_soil2();
  }
  else {
    Serial.println("ERROR soil2");
    status_error = 502;
  }
}

void open_by_soil3() {
  if (open_by_m3 == 0 && open_by_s13 == 0 && open_by_t3 == 0) {
    CheckSensor_Relay_soil3();
  }
  else {
    Serial.println("ERROR soil3");
    status_error = 503;
  }
}

void CheckSensor_Relay_soil0() {
  if (soil >= max_soil0 ) {
    if (max_soil0 >= 100) {
      max_soil0 = 100;
    }
    digitalWrite(Relay0, 0);
    open_by_s00 = 0;
    check_relay0 = 0;
  }
  else if (soil <= min_soil0 ) {
    if (min_soil0 <= 0 ) {
      min_soil0 = 0;
    }
    digitalWrite(Relay0, 1);
    open_by_s00 = 1;
    check_relay0 = 1;
  }
  else if (max_soil0 >= soil && soil >= min_soil0 )
  {
    Serial.println("mid length ");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_soil0");
  }
}

void CheckSensor_Relay_soil1() {
  if (soil >= max_soil1 ) {
    if (max_soil1 >= 100) {
      max_soil1 = 100;
    }
    digitalWrite(Relay1, 0);
    open_by_s01 = 0;
    check_relay1 = 0;
  }
  else if (soil <= min_soil1 ) {
    if (min_soil1 <= 0) {
      min_soil1 = 0;
    }
    digitalWrite(Relay1, 1);
    open_by_s01 = 1;
    check_relay1 = 1;
  }
  else if (max_soil1 >= soil && soil >= min_soil1 )
  {
    Serial.println("mid length ");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_soil1");
  }
}

void CheckSensor_Relay_soil2() {
  if (soil >= max_soil2 ) {
    if (max_soil2 >= 100) {
      max_soil2 = 100;
    }
    digitalWrite(Relay2, 0);
    open_by_s02 = 0;
    check_relay2 = 0;
  }
  else if (soil <= min_soil2 ) {
    if (min_soil2 <= 0) {
      min_soil2 = 0;
    }
    digitalWrite(Relay2, 1);
    open_by_s02 = 1;
    check_relay2 = 1;
  }
  else if (max_soil2 >= soil && soil >= min_soil2 )
  {
    Serial.println("mid length ");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_soil2");
  }
}

void CheckSensor_Relay_soil3() {
  if (soil >= max_soil3 ) {
    if (max_soil3 >= 100) {
      max_soil3 = 100;
    }
    digitalWrite(Relay3, 0);
    open_by_s03 = 0;
    check_relay3 = 0;
  }
  else if (soil <= min_soil3 ) {
    if (min_soil3 <= 0) {
      min_soil3 = 0;
    }
    digitalWrite(Relay3, 1);
    open_by_s03 = 1;
    check_relay3 = 1;
  }
  else if (max_soil3 >= soil && soil >= min_soil3 )
  {
    Serial.println("mid length ");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_soil2");
  }
}

void CheckSensor_Relay_temp0() {
  if (temp >= max_temp0) {
    if (max_temp0 >= 100) {
      max_temp0 = 100;
    }
    digitalWrite(Relay0, 0);
    check_relay0 = 0;
    open_by_s10 = 0;
  }
  else if (temp <= min_temp0) {
    if (min_temp0 <= 0) {
      min_temp0 = 0;
    }
    digitalWrite(Relay0, 1);
    check_relay0 = 1;
    open_by_s10 = 1;
  }
  else if (max_temp0 >= temp && temp >= min_temp0) {
    Serial.println("mide length temp0");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_temp0");
  }
}

void CheckSensor_Relay_temp1() {
  if (temp >= max_temp1) {
    if (max_temp1 >= 100) {
      max_temp1 = 0;
    }
    digitalWrite(Relay1, 0);
    check_relay1 = 0;
    open_by_s11 = 0;
  }
  else if (temp <= min_temp1) {
    if (min_temp1 <= 0) {
      min_temp0 = 0;
    }
    digitalWrite(Relay1, 1);
    check_relay1 = 1;
    open_by_s11 = 1;
  }
  else if (max_temp1 >= temp && temp >= min_temp1) {
    Serial.println("mide length temp1");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_temp1");
  }
}

void CheckSensor_Relay_temp2() {
  if (temp >= max_temp2) {
    if (max_temp2 >= 100) {
      max_temp2 = 100;
    }
    digitalWrite(Relay2, 0);
    check_relay2 = 0;
    open_by_s12 = 0;
  }
  else if (temp <= min_temp2) {
    if (min_temp2 <= 0) {
      min_temp2 = 0;
    }
    digitalWrite(Relay2, 1);
    check_relay2 = 1;
    open_by_s12 = 1;
  }
  else if (max_temp2 >= temp && temp >= min_temp2) {
    Serial.println("mide length temp2");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_temp2");
  }
}

void CheckSensor_Relay_temp3() {
  if (temp >= max_temp3) {
    if (max_temp3 >= 100) {
      max_temp3 = 100;
    }
    digitalWrite(Relay3, 0);
    check_relay3 = 0;
    open_by_s13 = 0;
  }
  else if (temp <= min_temp3) {
    if (min_temp3 <= 0) {
      min_temp3 = 0;
    }
    digitalWrite(Relay3, 1);
    check_relay3 = 1;
    open_by_s13 = 1;
  }
  else if (max_temp3 >= temp && temp >= min_temp3) {
    Serial.println("mide length temp3");
  }
  else {
    Serial.println("ERROR Check function CheckSensor_Relay_temp3");
  }
}

//-------------------------------------------------------------------------------------------------------//

void webSerialJSON() {

  while (Serial.available() > 0) {
    DeserializationError err = deserializeJson(jsonDoc, Serial);
    //EepromStream eepromStream(address, size);
    EepromStream eeprom(255, 512);

    if (err == DeserializationError::Ok)
    {
      String command  =  jsonDoc["command"].as<String>();

      if (command == "restart") {
        WiFi.disconnect();
        ESP.restart();
      } else  if (command == "getInfo") {
        //-----------------READING--------------------
        deserializeJson(jsonDoc, eeprom);
        Serial.write(STX);
        serializeJsonPretty(jsonDoc, Serial);
        Serial.write(ETX);
      } else if (command == "updateInfo") {
        //------------------WRITING-----------------
        serializeJson(jsonDoc["payload"], eeprom);
        eeprom.flush();
        //        Serial.write(STX);
        //        Serial.print("saved");
        //        Serial.write(ETX);
        ESP.restart();
      } else if (command) {
        Serial.write(STX);
        Serial.print(command);
        Serial.println(", invalid command.");
        Serial.write(ETX);
      }

    }
    else
    {
      Serial.read();
    }
  }

}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  Serial.write(START_PATTERN, 3);
  Serial.flush();

  EepromStream eeprom(255, 512);
  deserializeJson(jsonDoc, eeprom);
  Serial.write(STX);
  serializeJsonPretty(jsonDoc, Serial);
  Serial.write(ETX);

  pinMode(Relay0, OUTPUT);
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDY, OUTPUT);
  digitalWrite(Relay0, 0);
  digitalWrite(Relay1, 0);
  digitalWrite(Relay2, 0);
  digitalWrite(Relay3, 0);
  max_soil0 = 85;
  min_soil0 = 70;

  mqtt_server = jsonDoc["mqtt_server"].as<String>();
  mqtt_Client = jsonDoc["mqtt_Client"].as<String>();
  mqtt_password = jsonDoc["mqtt_password"].as<String>();
  mqtt_username = jsonDoc["mqtt_username"].as<String>();
  password = jsonDoc["password"].as<String>();
  mqtt_port = jsonDoc["mqtt_port"].as<String>();
  ssid = jsonDoc["ssid"].as<String>();

  WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.waitForConnectResult();

  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  if (!jsonDoc.isNull()) {
    if ( WiFi.status() != WL_CONNECTED) {
      while ( WiFi.status() != WL_CONNECTED) {
        unsigned long currentTime = millis();
        webSerialJSON();
        if (currentTime - previousTime >= reconnectInterval) {
          WiFi.begin(ssid.c_str(), password.c_str());
          previousTime = currentTime;
        }
      }
    }
  } else {
    while (true) {
      WiFi.disconnect();
      webSerialJSON();
    }
  }
  
  client.setServer(mqtt_server.c_str(), mqtt_port.toInt());
  client.setCallback(callback);
  timeClient.begin();
  Wire.begin();
  //    lightMeter.begin();
  //    if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
  //      Serial.println("Couldn't find SHT31");
  //      while (1) delay(1);
  //    }
}

//###########################################//

void loop() {
  client.loop();
  webSerialJSON();

  unsigned long currentTime = millis();

  //  float temp = sht31.readTemperature();
  //  float humidity = sht31.readHumidity();
  //  float lux = lightMeter.readLightLevel() / 1000; //(KLux)

  float temp = 1;
  float humidity = 2;
  float lux = 3;

  timeClient.update();
  sensorValue = analogRead(analogInPin);

  float voltage1 = 0.0;
  float outputValue = 0.0;
  voltage1 = (sensorValue * 3.2) / 1023;
  outputValue = (voltage1 - 2.00) / (-0.015);

  if (outputValue != 0  ) {
    outputValue = abs(outputValue);
    if (outputValue <= outputValueMin) {
      outputValue = outputValueMin;
    } else if (outputValue >= outputValueMax) {
      outputValue = outputValueMax;
    }
  }

  soil = map(outputValue, 0, 1023, 100, 0); //persent form analogRead

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  String data = "{\"data\": {\"temperature\":" + String(temp) + ",\"humidity\":" + String(humidity) + ",\"lux\":" + String(lux) + ",\"soil\":" + String(soil) ;
  data += ",\"max_soil0\":" + String(max_soil0) + ",\"min_soil0\":" + String(min_soil0);
  data += ",\"max_soil1\":" + String(max_soil1) + ",\"min_soil1\":" + String(min_soil1);
  //  data += ",\"max_soil2\":" + String(max_soil2) + ",\"min_soil2\":" + String(min_soil2);
  //  data += ",\"max_soil3\":" + String(max_soil3) + ",\"min_soil3\":" + String(min_soil3);
  //  data += ",\"max_temp0\":" + String(max_temp0) + ",\"min_temp0\":" + String(min_temp0);
  //  data += ",\"max_temp1\":" + String(max_temp1) + ",\"min_temp1\":" + String(min_temp1);
  //  data += ",\"max_temp2\":" + String(max_temp2) + ",\"min_temp2\":" + String(min_temp2);
  //  data += ",\"max_temp3\":" + String(max_temp3) + ",\"min_temp3\":" + String(min_temp3);
  //  data += ",\"relay0\":" + String(check_relay0);
  //  data += ",\"relay1\":" + String(check_relay1);
  //  data += ",\"relay2\":" + String(check_relay2);
  //  data += ",\"relay3\":" + String(check_relay3);
  //  data += ",\"status_timer0\":" + String(status_timer0);
  //  data += ",\"status_timer1\":" + String(status_timer1);
  //  data += ",\"status_timer2\":" + String(status_timer2);
  //  data += ",\"status_timer3\":" + String(status_timer3);
  //  data += ",\"value_timer0\":" + String(value_timer0);
  //  data += ",\"value_timer1\":" + String(value_timer1);
  //  data += ",\"value_timer2\":" + String(value_timer2);
  //  data += ",\"value_timer3\":" + String(value_timer3);
  //  data += ",\"notify_status\":" + String(notify_status);
  //  data += ",\"notify_relay0\":" + String(notify_relay0);
  //  data += ",\"notify_relay1\":" + String(notify_relay1);
  //  data += ",\"notify_relay2\":" + String(notify_relay2);
  //  data += ",\"notify_relay3\":" + String(notify_relay3);
  data += ",\"UnixTime\":" + String(timeClient.getEpochTime()) + "}}";

  if (!client.connected()) {
    if (currentTime - previousTime >= reconnectInterval) {
      reconnect();
      previousTime = currentTime;
    }
  } else {
    if (currentTime - previousTime >= eventInterval) {
      digitalWrite(LEDR, 0);
      digitalWrite(LEDY, 0);
      call_function_relay0();
      call_function_relay1();
      call_function_relay2();
      call_function_relay3();
      //      Serial.println(timeClient.getEpochTime());
      //      Serial.println("Status ERROR");
      //      Serial.println(status_error);
      //      Serial.println("Status open by manual");
      //      Serial.print("open by manual 0 = ");
      //      Serial.println(open_by_m0);
      //      Serial.print("open by manual 1 = ");
      //      Serial.println(open_by_m1);
      //      Serial.print("open by manual 2 = ");
      //      Serial.println(open_by_m2);
      //      Serial.print("open by manual 3 = ");
      //      Serial.println(open_by_m3);
      //      Serial.println("Status open by sensor soil");
      //      Serial.print("open by sensor soil 0 = ");
      //      Serial.println(open_by_s00);
      //      Serial.print("open by sensor soil 1 = ");
      //      Serial.println(open_by_s01);
      //      Serial.print("open by sensor soil 2 = ");
      //      Serial.println(open_by_s02);
      //      Serial.print("open by sensor soil 3 = ");
      //      Serial.println(open_by_s03);
      //      Serial.println("Status open by sensor temp");
      //      Serial.print("open by sensor temp 0 = ");
      //      Serial.println(open_by_s10);
      //      Serial.print("open by sensor temp 1 = ");
      //      Serial.println(open_by_s11);
      //      Serial.print("open by sensor temp 2 = ");
      //      Serial.println(open_by_s12);
      //      Serial.print("open by sensor temp 3 = ");
      //      Serial.println(open_by_s13);
      //      Serial.println("Status open by timer");
      //      Serial.print("open by timer 0 = ");
      //      Serial.println(open_by_t0);
      //      Serial.print("open by timer 1 = ");
      //      Serial.println(open_by_t1);
      //      Serial.print("open by timer 2 = ");
      //      Serial.println(open_by_t2);
      //      Serial.print("open by timer 3 = ");
      //      Serial.println(open_by_t3);
      //      Serial.println("Status Active Timer / In active Timer");
      //      Serial.print("Status Timer relay 0 = ");
      //      Serial.println(status_timer0);
      //      Serial.print("Status Timer relay 1 = ");
      //      Serial.println(status_timer1);
      //      Serial.print("Status Timer relay 2 = ");
      //      Serial.println(status_timer2);
      //      Serial.print("Status Timer relay 3 = ");
      //      Serial.println(status_timer3);
      //      Serial.println("Notify_status");
      //      Serial.print("notify_status = ");
      //      Serial.println(notify_status);
      //      Serial.print("notify_relay0 = ");
      //      Serial.println(notify_relay0);
      //      Serial.print("notify_relay1 = ");
      //      Serial.println(notify_relay1);
      //      Serial.print("notify_relay2 = ");
      //      Serial.println(notify_relay2);
      //      Serial.print("notify_relay3 = ");
      //      Serial.println(notify_relay3);
      //      Serial.println(data);
      data.toCharArray(msg, (data.length() + 1));
      client.publish("@shadow/data/update", msg);
      previousTime = currentTime;
    }

  }

}



void reconnect() {
  //  Serial.print("Attempting NETPIE2020 connection…");
  digitalWrite(LEDR, 0);
  digitalWrite(LEDY, 1);
  if (client.connect(mqtt_Client.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
    //    Serial.println("NETPIE2020 connected");
    client.subscribe("@msg/led0");
    client.subscribe("@msg/led1");
    client.subscribe("@msg/led2");
    client.subscribe("@msg/led3");
    client.subscribe("@msg/max_soil0");
    client.subscribe("@msg/min_soil0");
    client.subscribe("@msg/max_soil1");
    client.subscribe("@msg/min_soil1");
    client.subscribe("@msg/max_soil2");
    client.subscribe("@msg/min_soil2");
    client.subscribe("@msg/max_soil3");
    client.subscribe("@msg/min_soil3");
    client.subscribe("@msg/max_temp0");
    client.subscribe("@msg/min_temp0");
    client.subscribe("@msg/max_temp1");
    client.subscribe("@msg/min_temp1");
    client.subscribe("@msg/max_temp2");
    client.subscribe("@msg/min_temp2");
    client.subscribe("@msg/max_temp3");
    client.subscribe("@msg/min_temp3");
    client.subscribe("@msg/status_timer0");  //active - inavtive
    client.subscribe("@msg/status_timer1");
    client.subscribe("@msg/status_timer2");
    client.subscribe("@msg/status_timer3");
    client.subscribe("@msg/value_timer0");    //valueTime (6)
    client.subscribe("@msg/value_timer1");
    client.subscribe("@msg/value_timer2");
    client.subscribe("@msg/value_timer3");
    client.subscribe("@msg/notify_status");   // 0/1
    client.subscribe("@msg/notify_relay0");   // 0/1
    client.subscribe("@msg/notify_relay1");
    client.subscribe("@msg/notify_relay2");
    client.subscribe("@msg/notify_relay3");
  }
  else {
    //    Serial.println("try again...");
  }
}
