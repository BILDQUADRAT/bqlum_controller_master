#include <Arduino.h>
#include <Wire.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>

#define NUM_CHANNELS 3
int8_t valueBuf[NUM_CHANNELS];

// Network settings
byte mac[]    = { 0x00, 0x0B, 0xF4, 0x00, 0x00, 0x03 };
byte server[] = { 10, 0, 24, 2 };
byte myIp[] = { 10, 0, 24, 251 };

void callback(char* topic, byte* payload, unsigned int length) {
};

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

enum ValueType { brightness_change };
void sendChannelValue(uint8_t channel, ValueType type, int8_t value) {
  char channelStr[4];
  sprintf(channelStr, "%d", channel);

  char topic[30];
  strcpy(topic, "light/");
  strcat(topic, channelStr);
  strcat(topic, "/");
  switch(type) {
    case brightness_change:
      strcat(topic, "brightness_change");
      break;
  }

  char valueStr[4];
  sprintf(valueStr, "%d", value);
  client.publish(topic, valueStr);
}

void receiveEvent(int howMany) {
  if(Wire.available() >= 2) {
    uint8_t channel = Wire.read();
    Serial.print(channel);
    int8_t value = Wire.read();
    Serial.print(value, 10);
    if(channel < NUM_CHANNELS) {
      valueBuf[channel] += value;
    }
  }
  Serial.println();
}

void setup() {
  // initialize Ethernet
  if(!Ethernet.begin(mac)) {
    Ethernet.begin(mac, myIp);
  }

  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output
  Serial.println("Encoder Master started");
}

#define TIME_DIV 10

unsigned long lastCheck = millis() / TIME_DIV;
void maybeSendValue() {
  if( (millis()/TIME_DIV) - lastCheck > 100 ) {
    lastCheck = millis() / TIME_DIV;
    for(uint8_t i = 0; i < NUM_CHANNELS; i++) {
      if(valueBuf[i] != 0) {
        sendChannelValue(i, brightness_change, valueBuf[i]);
        valueBuf[i] = 0;
        Serial.println("Sent some value");
      }
    }
  }
}

void loop() {
  //library loop
  //reconnect if necessary
  if(!client.connected()) {
    if(client.connect("demoController")) {
      client.publish("test/sys","connected!");
    }
  }

  maybeSendValue();
  client.loop();
}
