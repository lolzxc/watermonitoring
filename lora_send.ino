#define csPin  10         // LoRa radio chip select
#define resetPin  -1       // LoRa radio reset
#define irqPin  2         // change for your board; must be a hardware interrupt pin

#include <SPI.h>              // include libraries
#include <LoRa.h>

byte localAddress = 0xAA;     // address of this device
byte destination = 0xFF;      // destination to send to

unsigned long lastSendTime = 0;        // last send time
int interval = 5000;                   // send msg every 5 seconds

String acknowledge = "monitor";        // symbol or word to check from esp8266 reply(callback)
byte maxSentMsg = 3;                  // continue to send until max limit reached,then show "failed to send"

String msg = " ";

void LoRaSetup() {
   LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6)) {                       // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                                 // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");

  LoRa.setSpreadingFactor(12);
  LoRa.setSyncWord(0x55);

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();
}

void onTxDone() {
  LoRa_rxMode();
  Serial.print("sent status: ");
  Serial.println(msg);
  numSentMsg += 1;
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();                // active invert I and Q signals
}

void LoRa_rxMode(){
  LoRa.enableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}

void sendWaterStatus() {

  sending_display();
  if (state == sendingState) {                            //send continuously until esp8266 replied
    if (millis() - lastSendTime > interval) {    //send every interval seconds
      msg = String(temp_lastReading)+'$'+ph_lastReading+'$'+depth_lastReading;
      sendMessage(msg);
      lastSendTime = millis();                    // timestamp the message
      state = readingState;
    }

    if (numSentMsg >= maxSentMsg) {
      state = toSendState;                           //used to stop sending
      // Serial.println("sending failed, long press to resend");
      numSentMsg = 0;
      sendingFailed_display();
      sendWaterStatus();
    }
  }
}


void sendMessage(String outgoing) {
  LoRa_txMode();
  LoRa.beginPacket();                            // start packet
  LoRa.write(destination);                       // add destination address
  LoRa.write(localAddress);                      // add sender address
  LoRa.write(outgoing.length());                 // add payload length
  LoRa.print(outgoing);                          // add payload
  LoRa.endPacket(true);                              // finish packet and send it                          // increment message ID
}


void onReceive(int packetSize) {       
  // read packet header bytes:
  int receiver = LoRa.read();                  // receiver address
  byte sender = LoRa.read();                    // sender address
  byte incomingLength = LoRa.read();            // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  if (incoming == acknowledge) {
    sendingSuccess_display();     
  }
}
