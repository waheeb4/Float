#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MS5837.h"
#include "esp_system.h"
#include <ESP32Time.h>

ESP32Time rtc(0);

const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";

MS5837 sensor;

int data_counter = 0;
float pressure = 0.0;
float p = 0.0;
float last_pressure = 0.0;

WebServer server(80);
bool rising = false;
bool Start = true;
bool dataSent = false;
bool dataReceived = false;
String data = "";
String Total_Data = "";
String initial_reading = "";

unsigned long t_now = 0;
unsigned long t_prv = 0;
unsigned long t_reg = 0;

bool flag1 = false;
bool flag2 = false;
bool adjflag = false;
bool adjflag2 = false;

void handleData() {
    if (Start && !dataReceived && data_counter == 0) {
        sensor.read();
        p = sensor.pressure() / 10;
        if (p > 80 && p < 145) {
            pressure = p;
            initial_reading = String(pressure, 2) + "/" + rtc.getTime("%H:%M:%S");
            Serial.println("Sending initial reading: " + initial_reading);
            server.send(200, "text/plain", initial_reading);
            return;
        }
    }
    if (Start) {
        Serial.println("Requesting to Start the descend ......");
        server.send(200, "text/plain", "starting the descend");
    }
    if (!dataReceived && data_counter >= 30) {
        Serial.println("Sending: " + Total_Data);
        server.send(200, "text/plain", Total_Data);
        Serial.println("Data sent to client, waiting for acknowledgment...");
    }
}

void handleAck() {
    if (Start) {
        Start = false;
        server.send(200, "text/plain", "ACK received");
        Serial.println("Station acknowledged the first descend");
        Serial2.println("Start");
        t_prv = millis();
        t_reg = millis();
        flag2 = true;
        last_pressure = pressure;
    } else {
        dataReceived = true;
        server.send(200, "text/plain", "ACK received");
        Serial.println("Laptop acknowledged data receipt! Stopping data transmission.");
        esp_restart();
    }
}

void setup() {
    Serial.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    Wire.begin(21, 22);
    while (!sensor.init()) {
        Serial.println("Init failed! Check SDA/SCL connections.");
        delay(5000);
    }
    sensor.setFluidDensity(1225);
    WiFi.softAP(ssid, password);
    Serial.println("ESP32 Access Point Started");
    server.on("/data", handleData);
    server.on("/ack", handleAck);
    server.begin();
    Serial.println("Server Started");
    rtc.setTime(0, 15, 3, 24, 4, 2025);
}

void loop() {
    server.handleClient();
    t_now = millis();
    sensor.read();
    p = sensor.pressure() / 10;
    if(p > 80 && p < 145){
      pressure = p;
    }
    Serial.println(pressure);
    
    if(flag2 && (pressure - last_pressure) < 3 && t_now-t_reg >= 12000){
      Serial2.println("1down");
      Serial.println("1down1");
      t_reg = t_now;
      last_pressure = pressure;
    } else if(flag2 && (pressure - last_pressure) > 10){
      Serial2.println("lab");
      Serial.println("1down2");
      flag2 = false;
      flag1 = true;
      adjflag = true;
      t_reg = t_now;
      last_pressure = pressure;
    }
    if(pressure > 120 && adjflag && !rising){
      Serial.println("lab");
      adjflag = false;
      t_reg = t_now;
      last_pressure = pressure;
    }
    if(pressure > 122 && t_now-t_reg >= 4000 && pressure - last_pressure > 0.3 && !rising){
      Serial2.println("lab");
      Serial.println("lab");
      adjflag2 = true;
      t_reg = t_now;
      last_pressure = pressure;
    }
    if(adjflag2 && pressure < 123 && last_pressure - pressure > 0.3 && t_now-t_reg >= 4000 && !rising){
      Serial2.println("1down");
      Serial.println("1down3");
      t_reg = t_now;
      last_pressure = pressure;
    }
    if (data_counter >= 30 && !rising) {
        adjflag = false;
        rising = true;
        adjflag2 = false;
        delay(50);
        Serial2.println("Up");
    }
    if (pressure > 0 && pressure < 140) {
        if (flag1 && data_counter < 30 && (t_now-t_prv >= 5000)) {
            data = String(pressure, 2) + "/" + rtc.getTime("%H:%M:%S");
            if (Total_Data.length() > 0) {
                Total_Data += ", ";
            }
            Total_Data += data;
            data_counter++;
            Serial.println(data_counter);
            Serial.println("Adding: " + data);
            t_prv = t_now;
        }
    }
}
