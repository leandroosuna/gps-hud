#ifndef _MAIN_H
#define _MAIN_H

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
bool* translateSR(bool * from, int digit);
bool* getSRFromNumber(int num, bool dp);
bool* getSRFromNumberInv(int num, bool dp);
bool* getNumber(int n, bool dp, int digit);
void cacheNumbers();
void send(bool* data);
void sendSR(int d1, int d2, int d3, int speed);
#endif