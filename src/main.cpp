#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>

const char* SSID = "";
const char* PASSWORD = "";

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

void initWiFi(void);
void reconnectWiFi(void);

void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");
  reconnectWiFi();
}

void reconnectWiFi(void) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado com sucesso!");
}

void setup()
{
  Serial.begin(115200);
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
  doorState = digitalRead(DOOR_SENSOR_PIN);

  if (doorState == HIGH)
  {
    digitalWrite(LED_PIN, HIGH);
    currentTime = millis();
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
    return;
  }

  Serial.print("Temp. = ");
  Serial.println(temp);

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
    }
  }

  delay(100);
}
