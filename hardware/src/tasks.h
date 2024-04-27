#ifndef TASKS_H
#define TASKS_H

#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>

void readDoorSensor();
void readTemperatureHumidity();
void servoControl();

#endif // TASKS_H