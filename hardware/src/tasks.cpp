#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>

void readDoorSensor()
{
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
}

void readTemperatureHumidity()
{
  float humidity = dhtSensor.readHumidity();
  float temp = dhtSensor.readTemperature();

  if (isnan(humidity) || isnan(temp))
  {
    Serial.println("Failed to read from DHT sensor!");
    sendDataToFirebaseString("Failed to read from DHT sensor!", humidityPath);
  }

  Serial.print("Temp. = ");
  Serial.println(temp);
  sendDataToFirebaseFloat(temp, temperaturePath);

  if (temp > 30)
  {
    ledcWrite(0, 255);
  }
  else
  {
    ledcWrite(0, 125);
  }
}

void servoControl()
{
  if (Serial.available() > 0)
  {
    char command = Serial.read();
    bool isChanged = false; // Flag para verificar mudança de posição

    if (command == 'u' && pos < 180)
    {
      pos += 10;
      isChanged = true;
    }
    else if (command == 'd' && pos > 0)
    {
      pos -= 10;
      isChanged = true;
    }

    if (isChanged)
    {
      servoMotor.write(pos);
      Serial.println(pos);
      sendDataToFirebaseInt(pos, posPath);
    }
  }
}
