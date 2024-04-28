#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"//Provide the RTDB payload printing info and other helper functions.

//#define WIFI_SSID "TI-01 0380"
//#define WIFI_PASSWORD "b#0642R2"
#define WIFI_SSID "504"
#define WIFI_PASSWORD "LS457190"

#define API_KEY "AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao" //Firebase project API Key
#define DATABASE_URL "https://proactive-ae334-default-rtdb.firebaseio.com/" //RTDB URLefine the RTDB URL */


EventGroupHandle_t xEventGroupKey;
#define EV_WIFI (1 << 0) //Define o bit do evento Conectado ao WI-FI
#define EV_FIRE (1 << 1) //Define o bit do evento Conectado ao Firebase
#define EV_T1 (1 << 2) //Define o bit do evento 2 segundos
#define EV_T2 (1 << 3) //Define o bit do evento 5 segundos

//Tarefas Firebase e conexão com a internet:

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

void tarefaTeste1(void *pvParameters)
{
  const EventBits_t xBitsToWaitFor = (EV_FIRE|EV_WIFI|EV_T2);
  EventBits_t xEventGroupValue;
  for(;;){
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdFALSE, portMAX_DELAY);
    if (xEventGroupValue == xBitsToWaitFor){
      Serial.println("Tarefa 1");
      xEventGroupSetBits(xEventGroupKey, EV_T1);
      xEventGroupClearBits(xEventGroupKey, EV_T2);
      vTaskDelay(pdMS_TO_TICKS(300));
    }
  }
}

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


void initWiFi(void *pvParameters)
{
  for(;;){
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.println("------Conexao WI-FI------");
      Serial.print("Conectando-se na rede: ");
      Serial.println(WIFI_SSID);
      Serial.println("Aguarde");
        while (WiFi.status() != WL_CONNECTED){
          Serial.print(".");
          vTaskDelay(300);
        }
      if (WiFi.status() == WL_CONNECTED){
        Serial.println();
        Serial.print("Conectado com o IP: ");
        Serial.println(WiFi.localIP());
        xEventGroupSetBits(xEventGroupKey, EV_WIFI);
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
    xEventGroupValue = xEventGroupWaitBits(xEventGroupKey, xBitsToWaitFor, pdFALSE, pdFALSE, portMAX_DELAY);
    if(xEventGroupValue & EV_WIFI){
      if (Firebase.signUp(&config, &auth, "", "")){
        Serial.println("Conectando ao Firebase...");
        signupOK = true;
        config.token_status_callback = tokenStatusCallback;//Assign the callback function for the long running token generation task 
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        Serial.println("Conectado ao Firebase");
        xEventGroupSetBits(xEventGroupKey, EV_FIRE);//Configura o BIT (EV_2SEG) em 1
        xEventGroupSetBits(xEventGroupKey, EV_T2);//Configura o BIT (EV_2SEG) em 1
        vTaskDelete(NULL);
      }
      else
      {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
      }
    }
    }
}

void setup()
{
  EventBits_t x;
  EventBits_t y;
  TaskHandle_t taskHandle;
  Serial.begin(115200);
  delay(500);

  xEventGroupKey = xEventGroupCreate();// Cria Event Group

  if(xEventGroupKey == NULL){
    Serial.printf("\n\rFalha em criar a Event Group xEventGroupKey");}

  //Conexão com a internet
  if (xTaskCreate(initWiFi, "initWiFi", 5000, NULL, 1, NULL) != pdPASS) {
    Serial.println("Falha ao criar a tarefa initWiFi");}
    // Criação da tarefa conectarFirebase
  if (xTaskCreate(conectarFirebase, "conectarFirebase", 5000, NULL, 1, NULL) != pdPASS) {
    Serial.println("Falha ao criar a tarefa conectarFirebase");}  
  // Criação da tarefa tarefaTeste1
  if (xTaskCreate(tarefaTeste1, "tarefaTeste1", 5000, NULL, 1, NULL) != pdPASS) {
       Serial.println("Falha ao criar a tarefa tarefaTeste1");}
  // Criação da tarefa tarefaTeste2
  if (xTaskCreate(tarefaTeste2, "tarefaTeste2", 5000, NULL, 1, NULL) != pdPASS) {
        Serial.println("Falha ao criar a tarefa tarefaTeste2");}
}
 
void loop()
{
}