#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"//Provide the RTDB payload printing info and other helper functions.

//Varáveis Wifi
// #define WIFI_SSID "TI-01 0380"
// #define WIFI_PASSWORD "b#0642R2"
#define WIFI_SSID "504"
#define WIFI_PASSWORD "LS457190"

//Firebase RTDB
#define API_KEY "AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao" //Firebase project API Key
#define DATABASE_URL "https://proactive-ae334-default-rtdb.firebaseio.com/" //RTDB URLefine the RTDB URL */

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

//Variáveis controle porta
const int DOOR_SENSOR_PIN = 32;
const int BUZZER_PIN = 19;
const int LED_PIN = 2;
int doorState = 0;
unsigned long doorOpenTime = 0;
unsigned long currentTime = 0;

//Variáveis controle de eventos
EventGroupHandle_t xEventGroupKey;
TaskHandle_t taskHandle_porta, taskHandle_verificaPorta;
QueueHandle_t queue_porta = xQueueCreate(2, sizeof(int));
#define EV_START (1 << 0)
#define EV_WIFI (1 << 1) //Define o bit do evento Conectado ao WI-FI
#define EV_FIRE (1 << 2) //Define o bit do evento Conectado ao Firebase

#define EV_STATUS_PORTA (1 << 10) //Define o bit do evento porta aberta
#define EV_BUZZER (1 << 11) //Define o bit do evento buzzer ativado
#define EV_T2 (1 << 5) 


void InicializaEsp(void *pvParameters)
{
  for(;;){
    Serial.println("Inicializando ESP32");
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(500));
    xEventGroupSetBits(xEventGroupKey, EV_START);
    vTaskDelete(NULL);
  }
}

void initWiFi(void *pvParameters)
{
  for(;;){
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("Conectando-se na rede: ");
      Serial.println(WIFI_SSID);
        while (WiFi.status() != WL_CONNECTED){
          Serial.print(".");
          vTaskDelay(100);
        }
      if (WiFi.status() == WL_CONNECTED){
      Serial.println();
      Serial.print("Conectado com o IP: ");
      Serial.println(WiFi.localIP());
      xEventGroupSetBits(xEventGroupKey, EV_WIFI);
      vTaskDelay(500);
      vTaskDelete(NULL);
    }  
  }
  }
}

void conectarFirebase(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_WIFI);
  EventBits_t xEventGroupValue;
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  for(;;){
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdTRUE, 0);
    if(xEventGroupValue & EV_WIFI){
      if (Firebase.signUp(&config, &auth, "", "")){
        Serial.println("Conectando ao Firebase...");
        signupOK = true;
        config.token_status_callback = tokenStatusCallback;//Assign the callback function for the long running token generation task 
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        Serial.println("Conectado ao Firebase");
        Serial.println("Ready...");
        xEventGroupSetBits(xEventGroupKey, EV_FIRE);//Configura o BIT (EV_2SEG) em 1
        vTaskDelete(NULL);
      }
      else
      {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
      }
    }
    }
}

void monitoraPorta(void *pvParameters)
{
  for(;;){
    doorState = digitalRead(DOOR_SENSOR_PIN);
    //xQueueSend(queue_porta, &doorState, 0);
    
    if(doorState == HIGH){
        xEventGroupSetBits(xEventGroupKey, EV_STATUS_PORTA);
        vTaskResume(taskHandle_porta);
      }
    else if(doorState == LOW){
      xEventGroupClearBits(xEventGroupKey, EV_STATUS_PORTA);
      vTaskResume(taskHandle_porta);
  }
  vTaskDelay(pdMS_TO_TICKS(200));
}
}

void AcionaBuzzer(void *pvParameters)
{
  for(;;){
    EventBits_t xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, EV_STATUS_PORTA, pdFALSE, pdTRUE, 0);
    //int status_porta=xQueueReceive(queue_porta, &doorState, 0);
    if (xEventGroupValue & EV_STATUS_PORTA){
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        xEventGroupSetBits(xEventGroupKey, EV_BUZZER);
    }
      else{
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        xEventGroupClearBits(xEventGroupKey, EV_BUZZER);
    }
    vTaskDelay(pdMS_TO_TICKS(300));
    vTaskSuspend(NULL);
    
  }
}
/*
void tarefaTeste2(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_FIRE|EV_WIFI|EV_T1);
  EventBits_t xEventGroupValue;
  for(;;){
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdFALSE, portMAX_DELAY);
    if (xEventGroupValue == xBitsToWaitFor){
      Serial.println("Tarefa 2");
      xEventGroupSetBits(xEventGroupKey, EV_T2);
      xEventGroupClearBits(xEventGroupKey, EV_T1);
      vTaskDelay(pdMS_TO_TICKS(300));
      }
    }
  }
*/


void setup()
{
  Serial.begin(115200);
  delay(200);

  xEventGroupKey = xEventGroupCreate();// Cria Event Group

  if(xEventGroupKey == NULL){
    Serial.printf("\n\rFalha em criar a Event Group xEventGroupKey");}

  // Criação da tarefa InicializaEsp
  if (xTaskCreatePinnedToCore(InicializaEsp, "InicializaEsp", 5000, NULL, 15, NULL,1) != pdPASS) {
    Serial.println("Falha ao criar a tarefa InicializaEsp");}
  delay(500);
  //Conexão com a internet
  if (xTaskCreatePinnedToCore(initWiFi, "initWiFi", 5000, NULL, 14, NULL,1) != pdPASS) {
    Serial.println("Falha ao criar a tarefa initWiFi");}
    // Criação da tarefa conectarFirebase
  if (xTaskCreatePinnedToCore(conectarFirebase, "conectarFirebase", 5000, NULL, 14, NULL,1) != pdPASS) {
    Serial.println("Falha ao criar a tarefa conectarFirebase");}  

    delay(2000);
    // Criação da tarefa AcionaBuzzer
  if (xTaskCreatePinnedToCore(AcionaBuzzer, "AcionaBuzzer", 5000, NULL, 2, &taskHandle_porta,1) != pdPASS) {
    Serial.println("Falha ao criar a tarefa AcionaBuzzer");}
      // Criação da tarefa VerificaPortaAberta
  if(xTaskCreatePinnedToCore(monitoraPorta, "monitoraPorta", 5000, NULL, 1, NULL,0) != pdPASS) {
    Serial.println("Falha ao criar a tarefa monitoraPorta");}

}

void loop()
{
}