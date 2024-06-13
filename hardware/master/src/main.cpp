#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"  //Provide the RTDB payload printing info and other helper functions.
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <driver/ledc.h>
#include <ESP32Servo.h>
#include "time.h"
#include <ESP32Time.h>

// Varáveis Wifi
// #define WIFI_SSID "INTELBRAS"
// #define WIFI_PASSWORD "EF191624"
// #define WIFI_SSID "TI-01 0380"
// #define WIFI_PASSWORD "b#0642R2"
// #define WIFI_SSID "504"
// #define WIFI_PASSWORD "LS457190"

#define WIFI_SSID "Galaxy Note1043b2"
#define WIFI_PASSWORD "fjid4169"

// Define o servidor NTP
const char *ntpServer1 = "a.st1.ntp.br";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.google.com";
const long gmtOffset_sec = -10800; // Defina o fuso horário (em segundos) -3 horas para Brasília
const int daylightOffset_sec = 0;  // Horário de verão

ESP32Time rtc(0);

// Estrutura para manter o tempo
struct tm timeinfo;
unsigned long previousMillis = 0;

// Firebase RTDB
#define API_KEY "AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao"                   // Firebase project API Key
#define DATABASE_URL "https://proactive-ae334-default-rtdb.firebaseio.com/" // RTDB URLefine the RTDB URL */
#define USER_EMAIL "proactive@proactive.com"                                // Firebase login email
#define USER_PASSWORD "Proactive@2024"                                      // Firebase login password

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
String userUID = "";

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
const int LEDC_CHANNEL = 1;
const int LEDC_FREQUENCY = 1000;               // Frequência em Hz
const int LEDC_RESOLUTION = LEDC_TIMER_13_BIT; // Resolução de 13 bits

// Variáveis Servo
const int SERVO_PIN = 26;
Servo servoMotor;
int pos = 90;

// Variáveis controle de eventos
EventGroupHandle_t xEventGroupKey;
SemaphoreHandle_t xSemaphore;
TaskHandle_t taskHandlePorta, taskHandleVerificaPorta;
QueueHandle_t queuePortaStatus = xQueueCreate(1, sizeof(int));
QueueHandle_t queuePortaTimer = xQueueCreate(1, sizeof(unsigned long));
QueueHandle_t queueTemperatura = xQueueCreate(1, sizeof(float));
QueueHandle_t queueUmidade = xQueueCreate(1, sizeof(float));
QueueHandle_t queueServoControl = xQueueCreate(1, sizeof(String));
QueueHandle_t queueControleTemperatura = xQueueCreate(1, sizeof(int));
QueueHandle_t queueCapturarFoto = xQueueCreate(1, sizeof(int));

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
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
        struct tm timeinfo;
        const int timeout = 30;
        int attempts = 0;
        Serial.print("Sincronizando a hora atual...");

        while (!getLocalTime(&timeinfo) && attempts < timeout)
        {

          Serial.print(".");

          attempts++;
          vTaskDelay(pdMS_TO_TICKS(50)); // Aguarde 1 segundo antes de tentar novamente
        }

        if (attempts < timeout)
        {
          Serial.println(&timeinfo, "Data e Hora iniciais: %Y-%m-%d %H:%M:%S");
          rtc.setTimeStruct(timeinfo);
        }
        else
        {
          Serial.println("Falha ao obter a hora NTP após várias tentativas.");
        }

        xEventGroupSetBits(xEventGroupKey, EV_WIFI);
        vTaskDelay(pdMS_TO_TICKS(500));
        vTaskDelete(NULL);
      }
    }
  }
}

void monitorWiFi(void *pvParameters)
{
  for (;;)
  {
    EventBits_t xEventGroupValue = xEventGroupGetBits(xEventGroupKey);
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi desconectado. Tentando reconectar...");
      // Conexão com a internet

      if ((xEventGroupValue & EV_WIFI))
      {
        if (xTaskCreatePinnedToCore(initWiFi, "initWiFi", 5000, NULL, 14, NULL, 1) == pdPASS)
        {
          xEventGroupClearBits(xEventGroupKey, EV_WIFI); // Configura o BIT (EV_WIFI) em 0
        }
        else
        {
          Serial.println("Falha ao criar a tarefa initWiFi");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(6000)); // Check every 60 seconds
  }
}

void conectarFirebase(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_WIFI);
  EventBits_t xEventGroupValue;
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  for (;;)
  {
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdTRUE, 0);
    if (xEventGroupValue & EV_WIFI)
    {
      Serial.println("Conectando ao Firebase...");
      signupOK = true;
      // config.token_status_callback = tokenStatusCallback; // Assign the callback function for the long running token generation task
      fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
      fbdo.setResponseSize(4096);
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);
      Serial.println("Conectado ao Firebase");
      Serial.println("Ready!");
      userUID = auth.token.uid.c_str();

      xEventGroupSetBits(xEventGroupKey, EV_FIRE); // Configura o BIT (EV_2SEG) em 1
      vTaskDelete(NULL);
    }
  }
}

void monitorFirebase(void *pvParameters)
{
  for (;;)
  {
    EventBits_t xEventGroupValue = xEventGroupGetBits(xEventGroupKey);
    if (!signupOK || !Firebase.ready())
    {
      Serial.println("Firebase desconectado. Tentando reconectar...");

      // Verifica se o evento EV_FIRE está desativado
      if ((xEventGroupValue & EV_FIRE))
      {
        if (xTaskCreatePinnedToCore(conectarFirebase, "conectarFirebase", 5000, NULL, 14, NULL, 1) == pdPASS)
        {
          xEventGroupClearBits(xEventGroupKey, EV_FIRE); // Configura o BIT (EV_2SEG) em 0
        }
        else
        {
          Serial.println("Falha ao criar a tarefa conectarFirebase");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(9000)); // Check every 60 seconds
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
      // Serial.print("Temperatura: ");
      // Serial.print(temp);
      // Serial.print(" °C\t Umidade: ");
      // Serial.println(humidity);
      xQueueSend(queueTemperatura, &temp, 0);
      xQueueSend(queueUmidade, &humidity, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
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
  int baseValue = 3; // Supondo um valor base inicial (pode ser configurado conforme necessário)
  float temp = 1;

  for (;;)
  {
    int items_waiting = uxQueueMessagesWaiting(queueTemperatura);
    if (items_waiting > 0)
    {
      if ((xQueuePeek(queueTemperatura, &temp, portMAX_DELAY) != pdTRUE) || ((xQueueReceive(queueControleTemperatura, &baseValue, portMAX_DELAY) != pdTRUE)))
      {
        Serial.println("Falha ao ler a fila de temperatura");
      }
      else
      {
        int minPWM = 256;
        int maxPWM = 512;
        // Ajustar o range de PWM com base no valor base
        switch (baseValue)
        {
        case 1:
          maxPWM = 512;
          break;
        case 2:
          maxPWM = 1024;
          break;
        case 3:
          maxPWM = 2048;
          break;
        case 4:
          maxPWM = 4096;
          break;
        case 5:
          maxPWM = 6144;
          break;
        default:
          Serial.println("Valor base inválido");
          Serial.print(baseValue);
          break;
        }
        if (temp > 30)
        {
          ledcWrite(1, maxPWM);
        }
        else if (temp < 20)
        {
          ledcWrite(1, minPWM);
        }
        else
        {
          float valor = map(temp, 20, 30, minPWM, maxPWM);
          ledcWrite(1, valor);
          // printf("Temperatura: %.2f, Valor: %.2f\n", temp, valor);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void controlaServo(void *pvParameters)
{
  for (;;)
  {
    int capturarFoto = 0;
    int pos = 0;
    char command = ' ';

    if (xQueueReceive(queueServoControl, &command, portMAX_DELAY) != pdTRUE)
    {
      Serial.println("Falha ao ler a fila de controle do servo");
    }
    else
    {
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
        vTaskDelay(pdMS_TO_TICKS(500));
        servoMotor.write(pos);
        Serial.println("Parou");

        int capturarFoto = 1;
        xQueueSend(queueCapturarFoto, &capturarFoto, 0);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void receberDadosFirebase(void *pvParameters)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void enviarDadosFirebase(void *pvParameters)
{
  float temp = 0;
  float humidity = 0;
  for (;;)
  {
    if (signupOK)
    {
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        if (xQueueReceive(queueTemperatura, &temp, 0) && xQueueReceive(queueUmidade, &humidity, 0))
        {
          struct tm timeinfo = rtc.getTimeStruct();

          char timestamp[20];
          strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

          FirebaseJson json;
          json.set("timestamp", timestamp);
          json.set("temperature", temp);
          json.set("humidity", humidity);

          String path = "/users/" + userUID + "/sensorData/" + String(timestamp);

          if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json))
          {
            Serial.println("Dados de temperatura enviados para o Firebase");
            Serial.printf("Timestamp: %s, Temperature: %.2f, Humidity: %.2f\n", timestamp, temp, humidity);
          }
          else
          {
            Serial.println("Falha ao enviar dados para o Firebase");
            Serial.println(fbdo.errorReason());
          }
        }
        xSemaphoreGive(xSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50000));
  }
}

void enviarStatusPorta(void *pvParameters)
{
  int lastDoorState = -1;
  int doorState = 0;
  unsigned long eventTime = 0;

  for (;;)
  {
    if (xQueueReceive(queuePortaStatus, &doorState, 0) == pdPASS)
    {
      if (doorState != lastDoorState)
      {
        lastDoorState = doorState;
        eventTime = millis();
        struct tm timeinfo = rtc.getTimeStruct();
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

        FirebaseJson json;
        json.set("timestamp", timestamp);
        json.set("door_status", doorState == HIGH ? "Aberta" : "Fechada");

        String path = "/users/" + userUID + "/doorStatus/" + String(timestamp);

        if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
        {
          if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json))
          {
            Serial.println("Status da porta enviado para o Firebase");
            Serial.printf("Timestamp: %s, Door Status: %s\n", timestamp, doorState == HIGH ? "open" : "closed");
          }
          else
          {
            Serial.println("Falha ao enviar status da porta para o Firebase");
            Serial.println(fbdo.errorReason());
          }
          xSemaphoreGive(xSemaphore);
        }
      }
    }
  }
  vTaskDelay(pdMS_TO_TICKS(5000));
}

void enviarValorInteiroFirebase(void *pvParameters)
{
  String path = "/users/" + userUID + "/camera/control";
  int capturarFoto = 0;

  for (;;)
  {
    if (xQueueReceive(queueCapturarFoto, &capturarFoto, portMAX_DELAY) == pdPASS)
    {
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        if (Firebase.RTDB.setInt(&fbdo, path.c_str(), capturarFoto))
        {
          Serial.println("Valor inteiro enviado para o Firebase com sucesso");
          Serial.printf("Path: %s, Valor: %d\n", path.c_str(), capturarFoto);
        }
        else
        {
          Serial.print("Falha ao enviar valor inteiro: ");
          Serial.println(fbdo.errorReason());
        }
        xSemaphoreGive(xSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void consultaDadosFirebase(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_FIRE);
  EventBits_t xEventGroupValue;

  for (;;)
  {
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdTRUE, 0);
    if (xEventGroupValue & EV_FIRE)
    {
      // Defina o caminho no Firebase RTDB de onde você deseja consultar os dados
      String path = "/users/" + userUID + "/siteData/";
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        // Consultar dados do Firebase
        if (Firebase.RTDB.getJSON(&fbdo, path.c_str()))
        {
          if (fbdo.dataType() == "json")
          {
            FirebaseJson &json = fbdo.jsonObject();
            FirebaseJsonData jsonData;

            json.get(jsonData, "/servo");
            String servoControl = jsonData.stringValue;
            json.get(jsonData, "/temperatura");
            int controleTemp = jsonData.intValue;

            String path2 = "/users/" + userUID + "/siteData/servo";

            Firebase.RTDB.setString(&fbdo, path2.c_str(), " ");

            printf("RECEBIDO: Servo: %s, Controle de Temperatura: %d\n", servoControl.c_str(), controleTemp);

            xQueueSend(queueControleTemperatura, &controleTemp, 0);
            xQueueSend(queueServoControl, &servoControl, 0);
          }
          else
          {
            Serial.println("Os dados não são do tipo JSON");
          }
        }
        else
        {
          Serial.print("Falha ao consultar dados: ");
          Serial.println(fbdo.errorReason());
        }
        xSemaphoreGive(xSemaphore);
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

  // Criação do semáforo
  xSemaphore = xSemaphoreCreateBinary();
  if (xSemaphore != NULL)
  {
    xSemaphoreGive(xSemaphore); // Libera o semáforo inicialmente
  }

  // Criação da tarefa InicializaEsp
  if (xTaskCreate(InicializaEsp, "InicializaEsp", 5000, NULL, 15, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa InicializaEsp");
  }
  delay(500);
  // Conexão com a internet
  if (xTaskCreate(initWiFi, "initWiFi", 5000, NULL, 14, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa initWiFi");
  }
  // Criação da tarefa conectarFirebase
  if (xTaskCreate(conectarFirebase, "conectarFirebase", 5000, NULL, 14, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa conectarFirebase");
  }
  delay(2000);
  // Tarefas rotina
  if (xTaskCreate(monitorWiFi, "monitorWiFi", 5000, NULL, 14, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitorWiFi");
  }
  if (xTaskCreate(AcionaBuzzer, "AcionaBuzzer", 5000, NULL, 1, &taskHandlePorta) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa AcionaBuzzer");
  }
  if (xTaskCreate(monitoraPorta, "monitoraPorta", 5000, NULL, 2, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitoraPorta");
  }
  if (xTaskCreate(monitoraTemperaura, "monitoraTemperaura", 5000, NULL, 2, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitoraTemperaura");
  }
  if (xTaskCreate(controlaVentilador, "controlaVentilador", 5000, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa controlaVentilador");
  }
  if (xTaskCreate(controlaServo, "controlaServo", 5000, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa controlaServo");
  }

  if (xTaskCreate(enviarDadosFirebase, "enviarDadosFirebase", 8192, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa enviarDadosFirebase");
  }

  if (xTaskCreate(enviarStatusPorta, "enviarStatusPorta", 8192, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa enviarStatusPorta");
  }
  if (xTaskCreate(enviarValorInteiroFirebase, "enviarValorInteiroFirebase", 8192, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa enviarValorInteiroFirebase");
  }

  if (xTaskCreate(consultaDadosFirebase, "consultaDadosFirebase", 8192, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa consultaDadosFirebase");
  }
}

void loop()
{
}