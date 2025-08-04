#include <WiFi.h>
#include <PubSubClient.h>

// WIFI init
const char* ssid = "Wokwi-GUEST";
const char* password = "";

//***Set server***
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

// WIFI and Client init
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// global state variable
bool ledState = false;
bool lastButtonState = false;
unsigned long start_water = 0;
unsigned long end_water = 0;
unsigned int water_time = 0;

// pin port
const int led = 2;
const int led2 = 4;
const int button = 14;
const int pump = 5;
const int LDR = 33;

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void mqttConnect() {
  while(!mqttClient.connected()) {
    Serial.println("Attemping MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      mqttClient.subscribe("/23127003/led_dashboard");
      mqttClient.subscribe("/23127003/pump");
      mqttClient.subscribe("/23127003/autowater");
      mqttClient.subscribe("/23127005/led2");
    }
    else {
      Serial.print(mqttClient.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Topic: ");
  Serial.println(topic);
  String msg;
  for(int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.print("msg: ");
  Serial.println(msg);

  String topic_string = String(topic);

  //***Code here to process the received package***
  // bật tắt đèn từ xa
  if (topic_string == "/23127003/led_dashboard" and msg == "toggle")
  {
    ledState = !ledState;
    digitalWrite(led, ledState);

    // gửi thông báo trạng thái đèn lên mqtt
    char led_state_message[] = "False";
    if (ledState == true)
      strcpy(led_state_message, "True");
    mqttClient.publish("/23127003/led_wokwi", led_state_message);
  }

  // tưới nước theo mức nước và thời gian tuỳ chỉnh
  else if (topic_string == "/23127003/pump" and msg.length() > 0)
  {
    int sepIndex = msg.indexOf('_');
    if (sepIndex != -1) 
    {
      String timeStr = msg.substring(sepIndex + 1);
      water_time = timeStr.toInt();

      digitalWrite(pump, HIGH);
      mqttClient.publish("/23127003/pump", "ON");
      start_water = millis();
      end_water = start_water + water_time * 1000;
    }
  }
  else if (topic_string == "/23127005/led2")
  {
    if (msg == "ON") {
      digitalWrite(led2, HIGH);
    } 
    else if (msg == "OFF") {
      digitalWrite(led2, LOW);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );

  pinMode(button, INPUT);
  pinMode(LDR, INPUT);
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(led2, OUTPUT);
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Reconnecting to WiFi");
    wifiConnect();
  }
  if(!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  //***Publish data to MQTT Server***
  // int temperature = random(0,100);
  // char buffer[50];
  // sprintf(buffer, "%d", temperature);
  // mqttClient.publish("/MSSV/temperature", buffer);

  // nút bấm bật tắt đèn
  bool buttonState = digitalRead(button);
  if (lastButtonState == false and buttonState == true)
  {
    ledState = !ledState;
    digitalWrite(led, ledState);
    
    char led_state_message[] = "False";
    if (ledState == true)
      strcpy(led_state_message, "True");
    mqttClient.publish("/23127003/led_wokwi", led_state_message);
  }
  lastButtonState = buttonState;

  if (millis() >= end_water and end_water > 0)
  {
    digitalWrite(pump, LOW);
    mqttClient.publish("/23127003/pump", "OFF");
    start_water = 0;
    end_water = 0;
  }

  int lightValue = analogRead(LDR);  // đọc từ quang trở (0-4095)
  char buffer[10];
  sprintf(buffer, "%d", lightValue);
  mqttClient.publish("/23127005/LDR", buffer);

  delay(500);
}
