#ifndef FIREBASE_H
#define FIREBASE_H

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"//Provide the RTDB payload printing info and other helper functions.


extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern unsigned long sendDataPrevMillis;
extern int count;
extern bool signupOK;

void initWiFi(String WIFI_SSID, String WIFI_PASSWORD);
void conectarFirebase(String API_KEY, String DATABASE_URL);
void sendDataToFirebaseFloat(float value, String path);
void sendDataToFirebaseInt(int value, String path);
void sendDataToFirebaseString(String value, String path);
int getDataFromFirebaseInt(String path);
float getDataFromFirebaseFloat(String path);
String getDataFromFirebaseString(String path);

#endif // FIREBASE_H
