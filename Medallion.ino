//////////////////////////////////////////////////////////////////////////
// Name:       Medallion.ino
// Created:	10/2/2018 3:51:36 PM
// Author:     quarkhopper
//
// Acknowledgments
//
// The server code and root page is heavily borrowed from Stan at:
/* https://42bots.com/tutorials/esp8266-example-wi-fi-access-point-web-server-static-ip-remote-control/ */
//
// Version: 0.2.1
// 0.2.1: increased LED brightness 50 -> 80

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <string>
#include "Utils.h"

#define PIXEL_PIN     2
#define NUM_PIXELS    20
#define MAX_BRIGHTNESS 80
#define DEFAULT_FADE_LO 0.00
#define DEFAULT_FADE_HI 0.25

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

float brightness[NUM_PIXELS];
float fadeStep[NUM_PIXELS];
float fadeSpeed[NUM_PIXELS];
float fade_lo = 0.00;
float fade_hi = 0.25;
float powerPulseStep = 0.02; // hi and lo value adjustment per tick
uint8_t baseColor[3] = {255, 0, 0}; // red
uint8_t flashColor[3] = {255, 246, 5}; // bright yellow
String baseColorName = "red";

//IPAddress AP_IP(10, 10, 10, 10); // setting a static ip causes connection problems from android devices
// default ip: 192.168.4.1

const char *ssid = "PirateMedallion";
const char *password = "treasure";
ESP8266WebServer server(80);

int32_t lastAnimationTick = -10000; // do it now
uint32_t animationTickDelay = 10; // delay between frames
bool enableAnimation = false;

void DoAnimationTick() {
	if (baseColorName == "blue") {
		// special power pulsing
		fade_hi += powerPulseStep;
		fade_lo += powerPulseStep;
		if (fade_hi > 1 || fade_lo < 0) {
			powerPulseStep *= -1;
			fade_hi += 2 * powerPulseStep;
			fade_lo += 2 * powerPulseStep;
		}
	}
	else {
		fade_lo = DEFAULT_FADE_LO;
		fade_hi = DEFAULT_FADE_HI;
	}

	for (uint8_t i = 0; i < NUM_PIXELS; i++) {
		uint8_t pixelColor[3];
		if (!random(255)) { // random chance of a flash
			pixelColor[0] = flashColor[0];
			pixelColor[1] = flashColor[1];
			pixelColor[2] = flashColor[2];
			brightness[i] = 1;
		}
		else {
			pixelColor[0] = baseColor[0];
			pixelColor[1] = baseColor[1];
			pixelColor[2] = baseColor[2];
			brightness[i] += fadeStep[i];
			if ( brightness[i] > fade_hi) {
				fadeStep[i] = float(random(1, 25))/255;
				fadeStep[i] *= -1;
				brightness[i] = fade_hi;
			}
			else if (brightness[i] < fade_lo) {
				fadeStep[i] = float(random(1, 25))/255;
				brightness[i] = fade_lo;
			}
		}
		strip.setPixelColor(i, GetFinalColor(pixelColor, brightness[i]));
	}
	strip.show();
	lastAnimationTick = millis();
}

uint32_t GetFinalColor(uint8_t* tupleColor, float brightness) {
	return Utils::GetRGB(tupleColor[0] * brightness,
	tupleColor[1] * brightness,
	tupleColor[2] * brightness);
}

void clearStrip() {
	for (uint8_t i = 0; i < NUM_PIXELS; i++) {
		strip.setPixelColor(i, 0x000000);
	}
	strip.show();
}

void handleRoot() {
	String powerArg = server.arg("power");
	String colorArg = server.arg("color");
	if (powerArg != "") {
		if (powerArg.toInt() == 1) enableAnimation = true;
		else enableAnimation = false;
	}
	else if (colorArg != "") {

	}

	if (colorArg == "blue") {
		baseColorName = "blue";
		baseColor[0] = 0; //56;
		baseColor[1] = 150; //172;
		baseColor[2] = 255;
	}
	else if (colorArg == "red") {
		baseColorName = "red";
		baseColor[0] = 255;
		baseColor[1] = 0;
		baseColor[2] = 0;
	}

	if (!enableAnimation) clearStrip();

	char PowerToggleText[80];
	char ColorToggleText[80];

	if (enableAnimation) strcpy(PowerToggleText, "Power is on. <a href=\"/?power=0\">Turn it OFF</a>");
	else strcpy(PowerToggleText, "Power is off. <a href=\"/?power=1\">Turn it ON</a>");
	if (baseColor[0] == 255) strcpy(ColorToggleText, "Color is red. <a href=\"/?color=blue\">Turn it BLUE</a>");
	else strcpy(ColorToggleText, "Color is blue. <a href=\"/?color=red\">Turn it RED</a>");
	

	char html[1000];
	snprintf(html, 1000,
	"<html>\
	<head>\
	<meta http-equiv='refresh' content='10'/>\
	<title>PirateFX Pirate Medallion</title>\
	<style>\
	body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
	h1 { Color: #AA0000; }\
	</style>\
	</head>\
	<body>\
	<h1>Pirate Medallion Master Control</h1>\
	<p>%s<p>\
	<p>%s<p>\
	<p>This page refreshes every 10 seconds. Click <a href=\"javascript:window.location.reload();\">here</a> to refresh the page now.</p>\
	</body>\
	</html>",
	PowerToggleText,
	ColorToggleText);
	server.send (200, "text/html", html);
}

void setup() {
	ESP.eraseConfig();

	randomSeed(RANDOM_REG32);

	Serial.begin(57600);
	Serial.printf("\n\n\n");

	WiFi.mode(WIFI_AP_STA);

	//WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,555,555,0)); // Unfortunately this prevents android from connecting
	
	WiFi.softAP(ssid, password);
	//	WiFi.setPhyMode(WIFI_PHY_MODE_11G);
	
	server.on("/", handleRoot);
	server.on("/?power=1", handleRoot);
	server.on("/?power=0", handleRoot);
	server.on("/?color=red", handleRoot);
	server.on("/?color=blue", handleRoot);

	
	server.begin();
	Serial.println("HTTP server started");

	strip.setBrightness(MAX_BRIGHTNESS);
	strip.begin();

	clearStrip();
	for (uint8_t i = 0; i < NUM_PIXELS; i++) {
		brightness[i] = float(random(256))/255; // 0 to 1
		fadeStep[i] = float(random(1,25))/255 * (!random(2) ? -1 : 1);
	}
}

int32_t lastDebugOutput = -10000;
uint32_t debugDelay = 3000;

void loop() {
	if (enableAnimation && millis() - lastAnimationTick > animationTickDelay) {
		DoAnimationTick();
	}

	server.handleClient();

	if (millis() - lastDebugOutput > debugDelay) {
		Serial.printf("Seconds: %i\n", int(millis() / 1000));
		WiFi.printDiag(Serial);
		Serial.printf("\n\n");
		lastDebugOutput = millis();
	}
}

