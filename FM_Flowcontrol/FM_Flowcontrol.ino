#include "Frame.h"

////////////////////////////////////FSK////////////////////////////////
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400
#include<Wire.h>
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;
const uint16_t S_DAC[4] = {1000, 2000, 1000, 0};
const int delay0 = (1000000 / f0 - 1000000 / defaultFreq) / 4;
const int delay1 = (1000000 / f1 - 1000000 / defaultFreq) / 4;
const int delay2 = (1000000 / f2 - 1000000 / defaultFreq) / 4;
const int delay3 = (1000000 / f3 - 1000000 / defaultFreq) / 4;
const int setSample = 4;
int byteString2Int(String arrays) {
  int num = 0;
  for (size_t i = 0 ; i < arrays.length() ; ++i) {
    //Serial.print((int)arrays[i]);
    num  = (num + (int)arrays[i] - 48) * 2;
  }
  return num / 2;
}
void TX_Flow(String Frame) {
  //Retrieve data input
  //Choose cycle and delay then send
  int SEND_BIN_DATA = byteString2Int(Frame);
  Serial.println("Frame : " + Frame);
  Serial.println("SEND_DATA : " + (String)SEND_BIN_DATA);
  for (size_t rounds = 15; rounds > 0; rounds -= 2)
  {
    int twoBitData = SEND_BIN_DATA & 3;
    Serial.print("TWOBITDATA : " + String(twoBitData));
    int usedDelay, cyclePerBaud;
    if (twoBitData == 0)
    {
      cyclePerBaud = 5;
      usedDelay = delay0;
    }
    else if (twoBitData == 1)
    {
      cyclePerBaud = 8;
      usedDelay = delay1;
    }
    else if (twoBitData == 2)
    {
      cyclePerBaud = 11;
      usedDelay = delay2;
    }
    else
    {
      cyclePerBaud = 14;
      usedDelay = delay3;
    }
    for (size_t nCycle = 0 ; nCycle < cyclePerBaud ; ++nCycle) {
      for (size_t nSample = 0 ; nSample < setSample ; ++nSample) {
        dac.setVoltage(S_DAC[nSample], false);
      }
    }
  }
  SEND_BIN_DATA >>= 2;
  dac.setVoltage(0, false);
}
////////////////////////////////////FSK////////////////////////////////
int myseq = 0;
int mode = -1;
int angle = -1;
String frame_arr[30] ;
int framecounter = 0;
void setup() {
  Serial.begin(9600);
  String test = Frame::make_UFrame(0);
  Serial.print("Press Enter to Scan all data");
}

void loop() {
  // put your main code here, to run repeatedly:
  while (mode == -1) {
    if (Serial.available()) {
      while (Serial.available()) {
        uint8_t temp = Serial.read();
      }
    }
  }
  while (mode == 0) { //insert command INIT to start scan

    String data = Frame::make_UFrame(0); // send setframe
    TX_Flow(data);
    //flushRX(); //w/ for implementation
    long timer = millis();
    while (mode == 0) {
      while (!Serial.available()) {//w/ for datain
        if (millis() - timer >= 3000) {
          Serial.println("Timeout...retransmiting");
          TX_Flow(data);//retransmit
          //flushRX();//w/ for implementation
          timer = millis();
        }
      }
      String receiverack;// = RX.getString();//w/ for implementation
      String ctrl, seq;
      String inp = Frame::decodeFrame(receiverack, &ctrl, &seq);
      if (ctrl.equals("01")) { //check act is correctly receive
        mode = 1;
      }
    }
    framecounter = 0;
    for (int i = 0; i < sizeof(frame_arr); i++) { //reset frame array
      frame_arr[i] = "";
    }
    timer = millis();
  }

  while (mode == 1) { //receiving data from sender
    //waitforserial();//waiting for implementation
    if (Serial.available()) {//available from read

      String receivedata;// = RX.read(); //w/ for implementation
      String seq, ctrl;
      String decodeddata = Frame::decodeFrame(receivedata, &ctrl, &seq);
      if (seq.equals(String(myseq))) {
        if (!decodeddata.equals("Error")) {
          frame_arr[framecounter] = decodeddata;
          framecounter += 1;
          myseq = (myseq + 1) % 2;
          String ACK = Frame::make_ackFrame(myseq);
          TX_Flow(ACK);
          long timer = millis();
          while (!Serial.available()) {
            if (millis() - timer > 3000) {
              Serial.println("Timeout...retransmiting");
              TX_Flow(ACK);
              //flushRX();//w/ for implementation
              timer = millis();
            }
          }
        }
      }
    }
    if (framecounter == 2) {
      mode = 2;
      framecounter = 0;
      //displayalldata();//w/ for implementation
    }
  }
  while (mode == 2) { //choose next command
    if (Serial.available()) {
      String readin = Serial.readStringUntil('\n');
      if (readin.equals("0")) { //reset scanning
        Serial.println("rescanning");
        mode = 0;
      } else if (readin.equals("1")) {//get -45 data
        Serial.println("scanning -45");
        angle = 1;
        mode = 3;
      } else if (readin.equals("2")) {//get -45 data
        Serial.println("scanning 0");
        angle = 2;
        mode = 3;
      } else if (readin.equals("3")) {//get -45 data
        Serial.println("scanning +45");
        angle = 3;
        mode = 3;
      } else {
        Serial.println("Wrong Input");
      }

    }
  }
  while (mode == 3) {

  }
}
