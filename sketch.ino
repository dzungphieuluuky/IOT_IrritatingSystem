#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h> 
#include <LiquidCrystal_I2C.h>

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
bool auto_watering = false;

// pin port
const int led = 2;
const int led2 = 4;
const int button = 14;
const int pump = 5;
const int LDR = 33;
const int dht_pin = 15;   
const int potPin = 34; 
DHT dht(dht_pin, DHT22);
int currentLedBrightness = 0;
unsigned long lastUserSetTime = 0;
int lastPotValue = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

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
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      mqttClient.subscribe("/23127003/led_dashboard");
      mqttClient.subscribe("/23127003/pump");
      mqttClient.subscribe("/23127003/autowater");
      mqttClient.subscribe("/23127005/led2");
      mqttClient.subscribe("/23127121/led_brightness_set");
      mqttClient.subscribe("/23127005/lcd_temperature");
      mqttClient.subscribe("/23127005/lcd_humidity");

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
    currentLedBrightness = 0;
    if (ledState == true)
    {
      strcpy(led_state_message, "True");
      currentLedBrightness = 255;
    }
    analogWrite(led, currentLedBrightness);
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
      auto_watering = false;
      mqttClient.publish("/23127003/pump", "ON");
      start_water = millis();
      end_water = start_water + water_time * 1000;
    }
  }

  // tự động tưới nước theo model
  else if (topic_string == "/23127003/autowater" and msg.length() > 0)
  {

    int sepIndex = msg.indexOf('_');
    if (sepIndex != -1) 
    {
      String timeStr = msg.substring(sepIndex + 1);
      water_time = timeStr.toInt();

      digitalWrite(pump, HIGH);
      auto_watering = true;
      mqttClient.publish("/23127003/autowater", "ON");
      start_water = millis();
      end_water = start_water + water_time * 1000;
    }
  }

  // tự động bật đèn khi trời tối
  else if (topic_string == "/23127005/led2")
  {
    if (msg == "ON") {
      digitalWrite(led2, HIGH);
    } 
    else if (msg == "OFF") {
      digitalWrite(led2, LOW);
    }
  }
  // else if (topic_string == "/23127121/led_brightness_set") {
  //   int brightness = msg.toInt();
  //   brightness = constrain(brightness, 0, 255);
  //   currentLedBrightness = brightness;
  //   ledState = (brightness > 0);
  //   analogWrite(led, brightness);
  //   lastUserSetTime = millis();
  // }
  else if (topic_string == "/23127121/led_brightness_set") {
    String payload = msg;
    bool isValidNumber = true;
    for (unsigned int i = 0; i < payload.length(); i++) {
      if (!isDigit(payload.charAt(i))) {
        isValidNumber = false;
        break;
      }
    }

    if (isValidNumber && payload.length() > 0) {
      int brightness = payload.toInt();
      brightness = constrain(brightness, 0, 255);
      currentLedBrightness = brightness;
      ledState = (currentLedBrightness > 0);
      analogWrite(led, currentLedBrightness);
      
      // lastUserSetTime = millis();
    }
  }
  // lcd temperature
  else if(topic_string == "/23127005/lcd_temperature")
  {
    lcd.setCursor(9,0);
    lcd.print("       ");
    lcd.setCursor(9,0);
    lcd.print(msg);
    lcd.write(byte(223));
    lcd.print("C");
  }
  // lcd humidity
  else if(topic_string == "/23127005/lcd_humidity")
  {
    lcd.setCursor(6,1);
    lcd.print("      ");
    lcd.setCursor(6,1);
    lcd.print(msg);
    lcd.print("%");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  pinMode(button, INPUT);
  pinMode(LDR, INPUT);
  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(led2, OUTPUT);
  dht.begin(); 
  analogWriteResolution(led, 8); 
  lcd.init(); 
  lcd.backlight(); 
  lcd.setCursor(0, 0);
  lcd.print("Nhiet do: ");
  lcd.setCursor(0, 1);
  lcd.print("Do am: ");

  digitalWrite(pump, LOW);

  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );

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


  // 1. Đọc cảm biến DHT
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)){
    char payload[100];
    snprintf(payload, sizeof(payload), "{\"temperature\":%.2f, \"humidity\":%.2f}", temperature, humidity);
    mqttClient.publish("/23127121/temperature_humidity", payload);
   // Serial.print("Published: ");
   // Serial.println(payload);
  } else {
    Serial.println("Failed to read from DHT sensor");
  }


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

    if (ledState) {
      analogWrite(led, currentLedBrightness > 0 ? currentLedBrightness : 255);
    } else {
      analogWrite(led, 0);
    }
    // digitalWrite(led, ledState);
    
    char led_state_message[] = "False";
    if (ledState == true)
      strcpy(led_state_message, "True");
    mqttClient.publish("/23127003/led_wokwi", led_state_message);
  }
  lastButtonState = buttonState;

  // check hết thời gian tưới nước
  if (millis() >= end_water and end_water > 0)
  {
    digitalWrite(pump, LOW);
    if (auto_watering)
    {
      mqttClient.publish("/23127003/autowater", "OFF");
      auto_watering = false;
    }
    else
      mqttClient.publish("/23127003/pump", "OFF");
    start_water = 0;
    end_water = 0;
  }

  //quang trở
  int lightValue = analogRead(LDR);  // đọc từ quang trở (0-4095)
  char buffer[10];
  sprintf(buffer, "%d", lightValue);
  mqttClient.publish("/23127005/LDR", buffer);

  // Đọc biến trở điều chỉnh độ sáng
  int potValue = analogRead(potPin);
  if (potValue != lastPotValue)
  {
    int brightness = map(potValue, 0, 4095, 0, 255);
    // Cập nhật LED nếu đèn đang bật
    if (ledState) {
      analogWrite(led, brightness);
      currentLedBrightness = brightness;
    } else {
      analogWrite(led, 0);
      currentLedBrightness = 0;
    }
  }
  lastPotValue = potValue;

  // Gửi trạng thái độ sáng biến trở lên Web mỗi 500ms
  static unsigned long lastPublish = 0;
  if (ledState && millis() - lastPublish > 500) {
    // Nếu chưa qua 2s kể từ lúc người dùng chỉnh slider thì bỏ qua gửi giá trị từ biến trở
    // if (millis() - lastUserSetTime > 2000) {
      char brightnessMsg[10];
      sprintf(brightnessMsg, "%d", currentLedBrightness);
      mqttClient.publish("/23127121/led_brightness_status", brightnessMsg);
    // }
    lastPublish = millis();
  }

  //lcd

  // delay(500);
}
