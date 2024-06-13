#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include "time.h"
#include <ESP32Time.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"  //Provide the RTDB payload printing info and other helper functions.
#include "base64.h"

// const char* ssid = "VIVOFIBRA-7F82";
// const char* password = "7223107F82";
// #define WIFI_SSID "INTELBRAS"
// #define WIFI_PASSWORD "EF191624"
//  #define WIFI_SSID "TI-01 0380"
//  #define WIFI_PASSWORD "b#0642R2"
//  #define WIFI_SSID "504"
//  #define WIFI_PASSWORD "LS457190"

#define WIFI_SSID "Galaxy Note1043b2"
#define WIFI_PASSWORD "fjid4169"

ESP32Time rtc(0);
// Estrutura para manter o tempo
struct tm timeinfo;
unsigned long previousMillis = 0;

// Define o servidor NTP
const char *ntpServer1 = "a.st1.ntp.br";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.google.com";
const long gmtOffset_sec = -10800; // Defina o fuso horário (em segundos) -3 horas para Brasília
const int daylightOffset_sec = 0;  // Horário de verão

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

// Variáveis controle de eventos
EventGroupHandle_t xEventGroupKey;
SemaphoreHandle_t xSemaphore;
TaskHandle_t taskHandlePorta, taskHandleVerificaPorta;
QueueHandle_t queuePortaStatus = xQueueCreate(1, sizeof(int));
QueueHandle_t queuePortaTimer = xQueueCreate(1, sizeof(unsigned long));

QueueHandle_t queueCapturarFoto = xQueueCreate(1, sizeof(int));

#define EV_START (1 << 0)
#define EV_WIFI (1 << 1) // Define o bit do evento Conectado ao WI-FI
#define EV_FIRE (1 << 2) // Define o bit do evento Conectado ao Firebase

#define EV_STATUS_PORTA (1 << 10) // Define o bit do evento porta aberta
#define EV_BUZZER (1 << 11)       // Define o bit do evento buzzer ativado

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

void InicializaEsp(void *pvParameters)
{
  for (;;)
  {
    Serial.println("Inicializando ESP32");

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // desativa o detector de brownout

    Serial.begin(115200);
    Serial.setDebugOutput(false);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 40;
    config.fb_count = 2;

    // Inicia a câmera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
      Serial.printf("Camera init failed with error 0x%x", err);
      return;
    }
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
          vTaskDelay(pdMS_TO_TICKS(100)); // Aguarde 1 segundo antes de tentar novamente
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
      fbdo.setBSSLBufferSize(16384 /* Rx buffer size in bytes from 512 - 16384 */, 16384 /* Tx buffer size in bytes from 512 - 16384 */);
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

//-------------------------------------------------------------------

void enviarDadosFirebase(void *pvParameters)
{

  for (;;)
  {
    int capturarFoto = 0;
    if (signupOK)
    {
      // Tentar pegar o semáforo
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        // Coletar dados das filas
        if ((xQueueReceive(queueCapturarFoto, &capturarFoto, pdMS_TO_TICKS(100))) == pdTRUE)
        {
          printf("Capturar foto: %d\n", capturarFoto);

          if (capturarFoto == 1)
          {
            // Obter o timestamp atual
            struct tm timeinfo = rtc.getTimeStruct();

            // Captura a imagem
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb)
            {
              Serial.println("Erro ao capturar a imagem");
              return;
            }

            // Converte a imagem para base64
            String base64_image = base64::encode((uint8_t *)fb->buf, fb->len);

            // Envia a imagem para o Firebase RTDB
            // Estruturar os dados em um formato JSON
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

            FirebaseJson json;
            json.set("timestamp", timestamp);
            json.set("base64_image", base64_image);

            // Enviar os dados para o Firebase
            String path = "/users/" + String(userUID) + "/camera/cameraData/" + String(timestamp);

            if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json))
            {
              Serial.println("Dados enviados para o Firebase");
            }
            else
            {
              Serial.println("Falha ao enviar dados para o Firebase");
              Serial.println(path);
              Serial.println(fbdo.errorReason());
            }
            // Libera a imagem
            esp_camera_fb_return(fb);
          }
        }

        // Liberar o semáforo
        xSemaphoreGive(xSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Enviar dados a cada 5 segundos
  }
}

void consultarEnviarValorInteiroFirebase(void *pvParameters)
{
  // Defina o caminho no Firebase RTDB onde você deseja consultar e enviar o valor
  String path = "/users/" + userUID + "/camera/control";
  int valorCapturado = 0;

  for (;;)
  {
    if (signupOK)
    {
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        // Consultar valor inteiro do Firebase
        if (Firebase.RTDB.getInt(&fbdo, path.c_str()))
        {
          if (fbdo.dataType() == "int")
          {
            int valorAtual = fbdo.intData();
            xQueueSend(queueCapturarFoto, &valorAtual, 0);
            Serial.printf("Valor atual consultado do Firebase: %d\n", valorAtual);

            if (valorAtual = 1)
            { // Configurar o valor como zero após a consulta
              if (Firebase.RTDB.setInt(&fbdo, path.c_str(), 0))
              {
                Serial.println("Valor configurado como zero no Firebase com sucesso");
                Serial.printf("Path: %s, Valor: 0\n", path.c_str());
              }
              else
              {
                Serial.print("Falha ao configurar o valor como zero: ");
                Serial.println(fbdo.errorReason());
              }
            }
          }
          else
          {
            Serial.println("Os dados consultados não são do tipo inteiro");
          }
        }
        else
        {
          Serial.print("Falha ao consultar valor inteiro: ");
          Serial.println(fbdo.errorReason());
        }
        xSemaphoreGive(xSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarde 5 segundos antes de consultar novamente
  }
}

//-------------------------------------------------------------------
// static esp_err_t stream_handler(httpd_req_t *req){
//   camera_fb_t * fb = NULL;
//   esp_err_t res = ESP_OK;
//   size_t _jpg_buf_len = 0;
//   uint8_t * _jpg_buf = NULL;
//   char * part_buf[64];
//   res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
//   if(res != ESP_OK){
//     return res;
//   }
//   while(true){
//     fb = esp_camera_fb_get();
//     if (!fb) {
//       Serial.println("Camera capture failed");
//       res = ESP_FAIL;
//     } else {
//       if(fb->width > 400){
//         if(fb->format != PIXFORMAT_JPEG){
//           bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
//           esp_camera_fb_return(fb);
//           fb = NULL;
//           if(!jpeg_converted){
//             Serial.println("JPEG compression failed");
//             res = ESP_FAIL;
//           }
//         } else {
//           _jpg_buf_len = fb->len;
//           _jpg_buf = fb->buf;
//         }
//       }
//     }
//     if(res == ESP_OK){
//       size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
//       res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
//     }
//     if(res == ESP_OK){
//       res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
//     }
//     if(res == ESP_OK){
//       res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
//     }
//     if(fb){
//       esp_camera_fb_return(fb);
//       fb = NULL;
//       _jpg_buf = NULL;
//     } else if(_jpg_buf){
//       free(_jpg_buf);
//       _jpg_buf = NULL;
//     }
//     if(res != ESP_OK){
//       break;
//     }
//   }
//   return res;
// }
// void startCameraServer(){
//   httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//   config.server_port = 80;
//   httpd_uri_t index_uri = {
//     .uri       = "/",
//     .method    = HTTP_GET,
//     .handler   = stream_handler,
//     .user_ctx  = NULL
//   };
//   if (httpd_start(&stream_httpd, &config) == ESP_OK) {
//     httpd_register_uri_handler(stream_httpd, &index_uri);
//   }
// }

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
  // Tarefa monitoraWiFi
  if (xTaskCreate(monitorWiFi, "monitorWiFi", 5000, NULL, 14, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa monitorWiFi");
  }

  if (xTaskCreate(enviarDadosFirebase, "enviarDadosFirebase", 16384, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa enviarDadosFirebase");
  }

  if (xTaskCreate(consultarEnviarValorInteiroFirebase, "consultarEnviarValorInteiroFirebase", 8096, NULL, 1, NULL) != pdPASS)
  {
    Serial.println("Falha ao criar a tarefa consultarEnviarValorInteiroFirebase");
  }
}

void loop()
{
}