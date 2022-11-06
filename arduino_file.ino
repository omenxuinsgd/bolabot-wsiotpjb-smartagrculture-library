#include <ESP8266MQTTClient.h>
#include <ESP8266WiFi.h>
#include <dht11.h>
#include <Servo.h>
#include <pt.h>

#define SOILPIN A0
#define DHT11PIN D0
#define SERVOPIN D1
#define PUMPPIN D2
#define LAMPPIN D3

MQTTClient mqtt;
dht11 DHT11;
Servo myservo;

static struct pt pt1, pt2, pt3;

void setup() {
  Serial.begin(115200);
  WiFi.begin("Pak Ay dan Arya", "ushu3953");
  pinMode(LAMPPIN, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(SOILPIN, INPUT);
  pinMode(DHT11PIN, INPUT);

  myservo.attach(SERVOPIN);

  PT_INIT(&pt1);
  PT_INIT(&pt2);
  PT_INIT(&pt3);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  mqtt.onDisconnect([]() {
    Serial.printf("MQTT: disconnect\r\n");
  });

  //topic, data, data is continuing
  mqtt.onData([](String topic, String data, bool cont) {
    Serial.printf("Data received, topic: %s, data: %s\r\n", topic.c_str(), data.c_str());
    led(topic.c_str(), data.c_str());
    pompa(topic.c_str(), data.toFloat());
// 
   mqtt.unSubscribe("/qos0");
  });

  mqtt.onSubscribe([](int sub_id) {
    Serial.printf("Subscribe topic id: %d ok\r\n", sub_id);
//    mqtt.publish("/qos0", "qos0", 0, 0);
  });
  mqtt.onConnect([]() {
    Serial.printf("MQTT: Connected\r\n");
//    Serial.printf("Subscribe id: %d\r\n", mqtt.subscribe("/qos0", 0));
    mqtt.subscribe("bolabot/v1/kelembapan", 1);
    mqtt.subscribe("bolabot/v1/suhu", 1);
    mqtt.subscribe("bolabot/v1/led", 1);
    mqtt.subscribe("bolabot/v1/tanah", 1);
    mqtt.subscribe("bolabot/v1/servo", 1);
//    mqtt.subscribe("/qos2", 2);
  });

  mqtt.begin("mqtt://test.mosquitto.org:1883");
//  mqtt.begin("mqtt://test.mosquitto.org:1883", {.lwtTopic = "hello", .lwtMsg = "offline", .lwtQos = 0, .lwtRetain = 0});
//  mqtt.begin("mqtt://user:pass@mosquito.org:1883");
//  mqtt.begin("mqtt://user:pass@mosquito.org:1883#clientId");

}

static int protothread1(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) { // never stop 
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp
    float nilaiSoil = analogRead(SOILPIN);
    mqtt.publish("bolabot/v1/tanah", String(nilaiSoil), 0, 0);
  }
  PT_END(pt);
}

static int protothread2(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) { // never stop
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp
    int chk = DHT11.read(DHT11PIN);
    float humi = (float)DHT11.humidity;
    float temp = (float)DHT11.temperature;
    Serial.print("Humidity (%): ");
    Serial.println(humi, 2); 
    Serial.print("Temperature (C): ");
    Serial.println(temp, 2);
    mqtt.publish("bolabot/v1/kelembapan", String(humi), 0, 0);
    mqtt.publish("bolabot/v1/suhu", String(temp), 0, 0);
  }
  PT_END(pt);
}

static int protothread3(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) { // never stop 
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp
    myservo.write(180);
    delay(500);
    myservo.write(30);
    delay(500);
    Serial.println("servo aktif");
  }
  PT_END(pt);
}

void led(String topic, String data){
  if(topic == "bolabot/v1/led"){
    if(data == "ON"){
      digitalWrite(LAMPPIN, LOW);
     }else if(data == "OFF"){
      digitalWrite(LAMPPIN, HIGH);
     }
    }
  }

void pompa(String topic, float data){
  if(topic == "bolabot/v1/tanah"){
    if(data >= 500){
      digitalWrite(PUMPPIN, LOW);
      Serial.println("Pompa Nyala");
     }else{
      digitalWrite(PUMPPIN, HIGH);
      Serial.println("Pompa Mati");
     }
    }
  }

void loop() {
  mqtt.handle();
  protothread1(&pt1, 10000); // schedule the two protothreads
  protothread2(&pt2, 10000); // schedule the two protothreads
  protothread3(&pt3, 10000); // schedule the two protothreads
}
