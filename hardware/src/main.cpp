
#include "firebase.h"
#include "tasks.h"

#define WIFI_SSID "TI-01 0380"
#define WIFI_PASSWORD "b#0642R2"
#define TEST_DIR "test/"
#define BASE_DIR "base/"

String doorStatePath= BASE_DIR + String("doorState");
String doorOpenTimePath= BASE_DIR + String("doorOpenTime");
String humidityPath= BASE_DIR + String("humidity");
String temperaturePath= BASE_DIR + String("temperature");
String posPath= BASE_DIR + String("pos");

//Firebase project API Key
#define API_KEY "AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao"
//RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://proactive-ae334-default-rtdb.firebaseio.com/" 

const int DOOR_SENSOR_PIN = 23;
const int BUZZER_PIN = 19;
const int LED_PIN = 18;
const int DHT_PIN = 15;
const int MOTOR_PIN = 25;
const int SERVO_PIN = 26;

Servo servoMotor;
int pos = 90;

#define DHTTYPE DHT22
DHT dhtSensor(DHT_PIN, DHTTYPE);

int doorState;
unsigned long doorOpenTime = 0;
unsigned long currentTime = 0;

void setup()
{
  Serial.begin(115200);

  initWiFi(WIFI_SSID, WIFI_PASSWORD);
  conectarFirebase(API_KEY, DATABASE_URL);
  
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  dhtSensor.begin();
  pinMode(MOTOR_PIN, OUTPUT);
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(pos);
  // Configuração do PWM
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PIN, 0);
}

void loop()
{

  readDoorSensor();
  readTemperatureHumidity();
  readServoMotor();
  servoControl();
  delay(1000);
}
