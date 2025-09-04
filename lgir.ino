#include <Encoder.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "wifi-config.h"

#define PIN_IR_DAT 0
#define PIN_LED_BUILTIN 2
#define PIN_SW 12
#define PIN_ENCODER_DT 13
#define PIN_ENCODER_CLK 14

#define BITS 32
#define COMMAND_DOWN 0x20DF827D
#define COMMAND_EXIT 0x20DFDA25
#define COMMAND_INPUT 0x20DFD02F
#define COMMAND_LEFT 0x20DFE01F
#define COMMAND_OK 0x20DF22DD
#define COMMAND_RIGHT 0x20DF609F
#define COMMAND_SETTINGS 0x20DFC23D

#define DELAY_SETUP 5000
#define DELAY_WIFI 10000
#define DELAY_SW 500
#define DELAY_COMMAND 300
#define DELAY_MENU 1500

Encoder encoder(PIN_ENCODER_CLK, PIN_ENCODER_DT);
IRsend irsend(PIN_IR_DAT);
ESP8266WebServer server(80);

int lastEncoderPosition = 0;
bool brightnessDialogue = false;

const char *brightnessArg = "b";


void sendCommand(unsigned int command) {
  digitalWrite(PIN_LED_BUILTIN, LOW);
  Serial.print("Sending command ");
  Serial.println(command);
  irsend.sendNEC(command);
  digitalWrite(PIN_LED_BUILTIN, HIGH);
}


void selectBrightnessEntry() {
  sendCommand(COMMAND_SETTINGS);
  delay(DELAY_MENU);
  sendCommand(COMMAND_OK);
  delay(DELAY_MENU);
  sendCommand(COMMAND_DOWN);
  delay(DELAY_COMMAND);
  sendCommand(COMMAND_DOWN);
  delay(DELAY_COMMAND);
  sendCommand(COMMAND_DOWN);
  delay(DELAY_COMMAND);
  sendCommand(COMMAND_EXIT);
}


void switchSource() {
  sendCommand(COMMAND_INPUT);
  delay(DELAY_MENU);
  sendCommand(COMMAND_INPUT);
  delay(DELAY_COMMAND);
  sendCommand(COMMAND_OK);
}


void enterBrightnessDialogue() {
  sendCommand(COMMAND_SETTINGS);
  delay(DELAY_MENU);
  sendCommand(COMMAND_OK);
  delay(DELAY_MENU);
  sendCommand(COMMAND_OK);
  delay(DELAY_COMMAND);
  brightnessDialogue = true;
}


void increaseBrightness(int variation) {
  int command = variation < 0 ? COMMAND_LEFT : COMMAND_RIGHT;
  int abs_variation = abs(variation);
  for (int n = 0; n < abs_variation; n++) sendCommand(command);
}


void exitBrightnessDialogue() {
  brightnessDialogue = false;
  sendCommand(COMMAND_EXIT);
  delay(DELAY_SW);
}


void setup() {
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  encoder.write(lastEncoderPosition);

  Serial.begin(9600);

  irsend.begin();

  digitalWrite(PIN_LED_BUILTIN, LOW);
  Serial.print("Press SW in ");
  Serial.print(DELAY_SETUP);
  Serial.println(" ms to load menus");
  long brightnessSetupMillis = millis();
  while (millis() - brightnessSetupMillis < DELAY_SETUP) {
    int encoderPosition = encoder.read();
    if (digitalRead(PIN_SW) == LOW) {
      selectBrightnessEntry();
      break;
    }
    yield();
  }
  digitalWrite(PIN_LED_BUILTIN, HIGH);

  Serial.print("Trying to connect to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  long wiFiSetupMillis = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wiFiSetupMillis < DELAY_WIFI) {
    digitalWrite(PIN_LED_BUILTIN, !digitalRead(PIN_LED_BUILTIN));
    Serial.print(".");
    delay(500);
  }

  digitalWrite(PIN_LED_BUILTIN, HIGH);

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_BUILTIN, LOW);
    Serial.print("\nConnected. IP: ");
    Serial.println(WiFi.localIP());
    server.on("/select-brightness-entry", []() {
      if (!brightnessDialogue) {
        server.send(200, "text/plain", "OK");
        selectBrightnessEntry();
      } else server.send(403, "text/plain", "Brightness is being changed");
    });
    server.on("/switch-source", []() {
      if (!brightnessDialogue) {
        server.send(200, "text/plain", "OK");
        switchSource();
      } else server.send(403, "text/plain", "Brightness is being changed");
    });
    server.on("/enter-brightness-dialogue", []() {
      server.send(200, "text/plain", "OK");
      enterBrightnessDialogue();
    });
    server.on("/increase-brightness", []() {
      if (server.hasArg(brightnessArg)) {
        if (brightnessDialogue) {
          server.send(200, "text/plain", "OK");
          int variation = server.arg(brightnessArg).toInt();
          increaseBrightness(variation);
        } else server.send(403, "text/plain", "Enter brightness dialogue first by calling /enter-brightness-dialogue");
      } else server.send(400, "text/plain", "Specify the \"b\" query argument (integer)");
    });
    server.on("/exit-brightness-dialogue", []() {
      if (brightnessDialogue) {
        server.send(200, "text/plain", "OK");
        exitBrightnessDialogue();
      } else server.send(403, "text/plain", "Enter brightness dialogue first by calling /enter-brightness-dialogue");
    });
    server.begin();
    digitalWrite(PIN_LED_BUILTIN, HIGH);
    Serial.println("Server started");
  } else {
    Serial.println("\nCouldn't connect");
  }

  Serial.println("Setup done");
}


void loop() {
  server.handleClient();

  if (digitalRead(PIN_SW) == LOW) {
    if (brightnessDialogue) exitBrightnessDialogue();
    else switchSource();
  }

  int encoderPosition = encoder.read();
  if (encoderPosition != lastEncoderPosition) {
    if (brightnessDialogue) increaseBrightness(encoderPosition - lastEncoderPosition);
    else enterBrightnessDialogue();
    lastEncoderPosition = encoderPosition;
  }
}