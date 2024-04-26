#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"//Provide the RTDB payload printing info and other helper functions.

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

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

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

void initWiFi(void)
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(WIFI_SSID);
  Serial.println("Aguarde");
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(300);

  Serial.println();
  Serial.print("Conectado com o IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  }
}

void conectarFirebase(void)
{
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Conectado ao Firebase");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

    /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendDataToFirebaseFloat(float value, String path)
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, path, value))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void sendDataToFirebaseInt(int value, String path)
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setInt(&fbdo, path, value))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void sendDataToFirebaseString(String value, String path)
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setString(&fbdo, path, value))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}


int getDataFromFirebaseInt(String path)
{
  if (Firebase.ready() && signupOK)
  {
    if (Firebase.RTDB.getInt(&fbdo, path))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      return fbdo.intData();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  return -1;
}

float getDataFromFirebaseFloat(String path)
{
  if (Firebase.ready() && signupOK)
  {
    if (Firebase.RTDB.getFloat(&fbdo, path))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      return fbdo.floatData();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  return -1;
}

String getDataFromFirebaseString(String path)
{
  if (Firebase.ready() && signupOK)
  {
    if (Firebase.RTDB.getString(&fbdo, path))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      return fbdo.stringData();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  return "";
}

void setup()
{
  Serial.begin(115200);

  initWiFi();
  conectarFirebase();
  
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

  //TESTE DB:
  float test = 0.01 + random(0,100);
  sendDataToFirebaseFloat(test, TEST_DIR + String("float"));
  Serial.println("Valor de teste enviado: " + String(test));
  delay(300);
  
  
  doorState = digitalRead(DOOR_SENSOR_PIN);
  sendDataToFirebaseInt(doorState, doorStatePath);

  if (doorState == HIGH)
  {
    digitalWrite(LED_PIN, HIGH);
    currentTime = millis();
    sendDataToFirebaseInt(doorState, doorStatePath);
    if (doorOpenTime == 0)
    {
      doorOpenTime = currentTime;
    }
    else if (currentTime - doorOpenTime >= 60000)
    {
      digitalWrite(BUZZER_PIN, HIGH);
    }
  }
  else
  {
    doorOpenTime = 0;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  float humidity = dhtSensor.readHumidity();
  float temp = dhtSensor.readTemperature();

  if (isnan(humidity) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    sendDataToFirebaseString("Failed to read from DHT sensor!", humidityPath);
  }

  Serial.print("Temp. = ");
  Serial.println(temp);
  sendDataToFirebaseFloat(temp, temperaturePath);

  if (temp > 30) {
    ledcWrite(0, 255);
  } else {
    ledcWrite(0, 125);
  }

  if (Serial.available() > 0) {
    char command = Serial.read();
    bool isChanged = false; // Flag para verificar mudança de posição

    if (command == 'u' && pos < 180) {
      pos += 10;
      isChanged = true;
    } else if (command == 'd' && pos > 0) {
      pos -= 10;
      isChanged = true;
    }

    if (isChanged) {
      servoMotor.write(pos);
      Serial.println(pos);
      sendDataToFirebaseInt(pos, posPath);
    }
  }

  delay(100);
}
