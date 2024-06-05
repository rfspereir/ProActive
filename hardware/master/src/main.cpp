#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"  //Provide the RTDB payload printing info and other helper functions.
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <driver/ledc.h>
#include <ESP32Servo.h>
#include "time.h"

// Varáveis Wifi
//#define WIFI_SSID "INTELBRAS"
//#define WIFI_PASSWORD "EF191624"
//#define WIFI_SSID "TI-01 0380"
//#define WIFI_PASSWORD "b#0642R2"
#define WIFI_SSID "504"
#define WIFI_PASSWORD "LS457190"

// Define o servidor NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800; // Defina o fuso horário (em segundos) -3 horas para Brasília
const int daylightOffset_sec = 0; // Horário de verão

// Firebase RTDB
#define API_KEY "AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao"                   // Firebase project API Key
#define DATABASE_URL "https://proactive-ae334-default-rtdb.firebaseio.com/" // RTDB URLefine the RTDB URL */

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// Variáveis controle porta
const int DOOR_SENSOR_PIN = 32;
const int BUZZER_PIN = 2;
const int LED_PIN_1 = 18;
const int LED_PIN_2 = 19;

// Variáveis DHT
#define DHTTYPE DHT22
const int DHT_PIN = 4;
DHT dhtSensor(DHT_PIN, DHTTYPE);

// Variáveis PWM(Ventilador)
const int MOTOR_PIN = 16;
const int LEDC_CHANNEL = 0;
const int LEDC_FREQUENCY = 1000;               // Frequência em Hz
const int LEDC_RESOLUTION = LEDC_TIMER_13_BIT; // Resolução de 13 bits

// #define PWM_CHANNEL 0  // Canal PWM a ser usado (0-15)
// #define PWM_FREQ 1000  // Frequência PWM (em Hz)
// #define PWM_RESOLUTION 8 // Resolução PWM (8 bits = 256 níveis)

// Variáveis Servo
const int SERVO_PIN = 26;
Servo servoMotor;
int pos = 90;

// Variáveis controle de eventos
EventGroupHandle_t xEventGroupKey;
TaskHandle_t taskHandlePorta, taskHandleVerificaPorta;
QueueHandle_t queuePortaStatus = xQueueCreate(1, sizeof(int));
QueueHandle_t queuePortaTimer = xQueueCreate(1, sizeof(unsigned long));
QueueHandle_t queueTemperatura = xQueueCreate(1, sizeof(float));
QueueHandle_t queueUmidade = xQueueCreate(1, sizeof(float));

#define EV_START (1 << 0)
#define EV_WIFI (1 << 1) // Define o bit do evento Conectado ao WI-FI
#define EV_FIRE (1 << 2) // Define o bit do evento Conectado ao Firebase

#define EV_STATUS_PORTA (1 << 10) // Define o bit do evento porta aberta
#define EV_BUZZER (1 << 11)       // Define o bit do evento buzzer ativado

void InicializaEsp(void *pvParameters)
{
  for (;;)
  {
    Serial.println("Inicializando ESP32");

    dhtSensor.begin(); // Inicializa o sensor DHT

    ledcSetup(LEDC_CHANNEL, LEDC_FREQUENCY, LEDC_RESOLUTION); // Inicializa o PWM
    ledcAttachPin(MOTOR_PIN, LEDC_CHANNEL);

    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);
    pinMode(MOTOR_PIN, OUTPUT);

    ledcAttachPin(MOTOR_PIN, LEDC_CHANNEL);

    servoMotor.attach(SERVO_PIN);
    servoMotor.write(pos);

    vTaskDelay(pdMS_TO_TICKS(500));
    xEventGroupSetBits(xEventGroupKey, EV_START);
    vTaskDelete(NULL);
  }
}

void initWiFi(void *pvParameters)
{
  for (;;)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("Conectando-se na rede: ");
      Serial.println(WIFI_SSID);
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println();
        Serial.print("Conectado com o IP: ");
        Serial.println(WiFi.localIP());
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        struct tm timeinfo;
        if(!getLocalTime(&timeinfo)){
          Serial.println("Failed to obtain time");
          }else{
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
          }     
          
        xEventGroupSetBits(xEventGroupKey, EV_WIFI);
        vTaskDelay(500);
        vTaskDelete(NULL);
      }
    }
  }
}

void monitorWiFi(void *pvParameters) {
    for (;;) {
       EventBits_t xEventGroupValue = xEventGroupGetBits(xEventGroupKey);
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi desconectado. Tentando reconectar...");
             // Conexão com a internet

            if ((xEventGroupValue & EV_WIFI)) { 
              if (xTaskCreatePinnedToCore(initWiFi, "initWiFi", 5000, NULL, 14, NULL, 1) == pdPASS)
              {
                xEventGroupClearBits(xEventGroupKey, EV_WIFI); // Configura o BIT (EV_WIFI) em 0
              }
              else{
                Serial.println("Falha ao criar a tarefa initWiFi");
              }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check every 60 seconds
    }
}


void conectarFirebase(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_WIFI);
  EventBits_t xEventGroupValue;
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  for (;;)
  {
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdTRUE, 0);
    if (xEventGroupValue & EV_WIFI)
    {
      if (Firebase.signUp(&config, &auth, "", ""))
      {
        Serial.println("Conectando ao Firebase...");
        signupOK = true;
        config.token_status_callback = tokenStatusCallback; // Assign the callback function for the long running token generation task
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        Serial.println("Conectado ao Firebase");
        Serial.println("Ready!");
        xEventGroupSetBits(xEventGroupKey, EV_FIRE); // Configura o BIT (EV_2SEG) em 1
        vTaskDelete(NULL);
      }
      else
      {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
      }
    }
  }
}

void monitorFirebase(void *pvParameters) {
    for (;;) {
        EventBits_t xEventGroupValue = xEventGroupGetBits(xEventGroupKey);
        if (!signupOK || !Firebase.ready()) {
            Serial.println("Firebase desconectado. Tentando reconectar...");
            
            // Verifica se o evento EV_FIRE está desativado
            if ((xEventGroupValue & EV_FIRE)) {     
              if (xTaskCreatePinnedToCore(conectarFirebase, "conectarFirebase", 5000, NULL, 14, NULL, 1) == pdPASS) {
                xEventGroupClearBits(xEventGroupKey, EV_FIRE); // Configura o BIT (EV_2SEG) em 0
              } 
                else { 
                  Serial.println("Falha ao criar a tarefa conectarFirebase");
                }
          }
        }
        vTaskDelay(pdMS_TO_TICKS(90000)); // Check every 60 seconds
    }
}


void monitoraPorta(void *pvParameters)
{
  int doorState = 0;
  unsigned long doorOpenTime = 0;
  for (;;)
  {
    doorState = digitalRead(DOOR_SENSOR_PIN);
    xQueueSend(queuePortaStatus, &doorState, 0);
    if (doorState == HIGH)
    {
      EventBits_t xEventGroupValue = xEventGroupGetBits(xEventGroupKey);
      if ((xEventGroupValue & EV_STATUS_PORTA) == 0)
      {
        doorOpenTime = millis();
        xQueueSend(queuePortaTimer, &doorOpenTime, 0);
      }
      xEventGroupSetBits(xEventGroupKey, EV_STATUS_PORTA);
    }
    else if (doorState == LOW)
    {
      xEventGroupClearBits(xEventGroupKey, EV_STATUS_PORTA);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void monitoraTemperaura(void *pvParameters)
{
  for (;;)
  {
    float humidity = dhtSensor.readHumidity();
    float temp = dhtSensor.readTemperature();

    if (isnan(humidity) || isnan(temp))
    {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    else
    {
      Serial.print("Temperatura: ");
      Serial.println(temp);
      Serial.print(" °C\t Umidade: ");
      Serial.print("Umidade: ");
      Serial.println(humidity);
      xQueueSend(queueTemperatura, &temp, 0);
      xQueueSend(queueUmidade, &humidity, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void AcionaBuzzer(void *pvParameters)
{
  unsigned long currentTime = 0;
  unsigned long doorOpenTime = 0;
  for (;;)
  {
    EventBits_t xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, EV_STATUS_PORTA, pdFALSE, pdTRUE, 0);
    if (xEventGroupValue & EV_STATUS_PORTA)
    {
      xQueueReceive(queuePortaTimer, &doorOpenTime, 0);
      digitalWrite(LED_PIN_1, HIGH);
      digitalWrite(LED_PIN_2, HIGH);
      currentTime = millis();
      if (currentTime - doorOpenTime >= 60000)
      {
        digitalWrite(BUZZER_PIN, HIGH);
        xEventGroupSetBits(xEventGroupKey, EV_BUZZER);
      }
    }
    else
    {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN_1, LOW);
      digitalWrite(LED_PIN_2, LOW);
      xEventGroupClearBits(xEventGroupKey, EV_BUZZER);
    }
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void controlaVentilador(void *pvParameters)
{
  for (;;)
  {
    float temp = 0;
    int items_waiting = uxQueueMessagesWaiting(queueTemperatura);
    if (items_waiting > 0)
    {
      // Ler o próximo item sem removê-lo
      if (xQueuePeek(queueTemperatura, &temp, 0) == pdTRUE)
      {
        if (temp > 30)
        {
          ledcWrite(0, 4096);
        }
        else if (temp < 20)
        {
          ledcWrite(0, 1024);
        }
        else {
          ledcWrite(0, map(temp, 20, 30, 1024, 4096));
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void controlaServo(void *pvParameters) {
  for (;;)  {
    int pos = 0;
    if (Serial.available() > 0)
    {
      int command = Serial.read();
      bool isChanged = false; // Flag para verificar mudança de posição

      if (command == 'u')
      {
        pos += 30;
        isChanged = true;
      }
      else if (command == 'd')
      {
        pos -= 30;
        isChanged = true;
      }

      if (isChanged)
      {
        servoMotor.write(pos);
        Serial.print("Pos=");
        Serial.println(pos);
        pos = 90;
        vTaskDelay(500);
        servoMotor.write(pos);
        Serial.println("Parou");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void receberDadosFirebase(void *pvParameters)
{
  for (;;)
  {
    if (signupOK)
    {
      if (Firebase.get(fbdo, "/"))
      {
        Serial.println("Recebendo dados do Firebase");
        Serial.println(fbdo.payload());
      }
      else
      {
        Serial.println("Falha ao receber dados do Firebase");
        Serial.println(fbdo.errorReason());
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void enviarDadosFirebase(void *pvParameters)
{
  for (;;)
  {
    if (signupOK)
    {
      if (millis() - sendDataPrevMillis > 5000)
      {
        sendDataPrevMillis = millis();
        count++;
        if (Firebase.set(fbdo, "/count", count))
        {
          Serial.println("Enviando dados para o Firebase");
          Serial.println(count);
        }
        else
        {
          Serial.println("Falha ao enviar dados para o Firebase");
          Serial.println(fbdo.errorReason());
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  xEventGroupKey = xEventGroupCreate(); // Cria Event Group
  if (xEventGroupKey == NULL)
  {
    Serial.printf("\n\rFalha em criar a Event Group xEventGroupKey");
  }

  // Criação da tarefa InicializaEsp
  if (xTaskCreatePinnedToCore(InicializaEsp, "InicializaEsp", 5000, NULL, 15, NULL, 1) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa InicializaEsp");
  }
  delay(500);
  // Conexão com a internet
  if (xTaskCreatePinnedToCore(initWiFi, "initWiFi", 5000, NULL, 14, NULL, 1) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa initWiFi");
  }
  // Criação da tarefa conectarFirebase
  if (xTaskCreatePinnedToCore(conectarFirebase, "conectarFirebase", 5000, NULL, 14, NULL, 1) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa conectarFirebase");
  }
  delay(2000);
  //Tarefa monitoraWiFi
  if (xTaskCreatePinnedToCore(monitorWiFi, "monitorWiFi", 5000, NULL, 14, NULL, 1) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitorWiFi");
  }
  // Criação da tarefa AcionaBuzzer
  if (xTaskCreatePinnedToCore(AcionaBuzzer, "AcionaBuzzer", 5000, NULL, 1, &taskHandlePorta, 1) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa AcionaBuzzer");
  }
  // Criação da tarefa VerificaPortaAberta
  if (xTaskCreatePinnedToCore(monitoraPorta, "monitoraPorta", 5000, NULL, 2, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitoraPorta");
  }
  // Criação da tarefa monitoraTemperaura
  if (xTaskCreatePinnedToCore(monitoraTemperaura, "monitoraTemperaura", 5000, NULL, 2, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitoraTemperaura");
  }
  // Criação da tarefa controlaVentilador
  if (xTaskCreatePinnedToCore(controlaVentilador, "controlaVentilador", 5000, NULL, 1, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa controlaVentilador");
  }
  // Criação da tarefa controlaServo
  if (xTaskCreatePinnedToCore(controlaServo, "controlaServo", 5000, NULL, 1, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa controlaServo");
  }

  if (xTaskCreatePinnedToCore(receberDadosFirebase, "receberDadosFirebase", 5000, NULL, 1, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa receberDadosFirebase");
  }

  if (xTaskCreatePinnedToCore(enviarDadosFirebase, "enviarDadosFirebase", 5000, NULL, 1, NULL, 0) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa enviarDadosFirebase");
  }  
}

void loop()
{
}