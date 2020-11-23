#include "Frame.h"
////////////////////////////////////FSK////////////////////////////////
////TX------VAR///////////
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
////TX------VAR///////////
////RX------VAR///////////
#ifndef cbi
#define cbi(sfr, bit)(_SFR_BYTE(sfr)&=~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit)(_SFR_BYTE(sfr)|=_BV(bit))
#endif
#define r_slope 200
int previousVoltage = 0;
int countCycle = 0;
uint16_t BAUD_COUNT = 0;
uint32_t DATA = 0;
uint16_t BIT_CHECK = -1;
bool CHECK_AMPLITUDE = false;
bool CHECK_BAUD = false;
uint32_t BAUD_BEGIN_TIME = 0;
////RX------VAR///////////
uint32_t byteString2Int(String arrays) {
  uint32_t num = 0;
  for (size_t i = 0 ; i < arrays.length() ; ++i) {
    //Serial.print((int)arrays[i]);
    num  = (num + (uint32_t)arrays[i] - 48) * 2;
  }
  return num / 2;
}
bool Timer(unsigned long currentTime, unsigned long interval) {
  return millis() - currentTime < interval;
}

void TX_Flow(String Frame) {
  //Retrieve data input
  //Choose cycle and delay then send
  uint32_t SEND_BIN_DATA = byteString2Int(Frame);
  //Serial.println("test " + (String)byteString2Int("1111111111111111"));
  Serial.println("Frame : " + Frame);
  Serial.println("SEND_DATA : " + (String)SEND_BIN_DATA);
  for (int rounds = 15; rounds > 0; rounds -= 2)
  {
    //Serial.println("Round : " + (String)rounds);
    uint32_t twoBitData = SEND_BIN_DATA & 3;
    //Serial.println("TWOBITDATA : " + (String)twoBitData);
    uint32_t usedDelay, cyclePerBaud;
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
        //Serial.println(S_DAC[nSample]);
        dac.setVoltage(S_DAC[nSample], false);
        delayMicroseconds(usedDelay);
      }
    }
    SEND_BIN_DATA >>= 2;
  }
  dac.setVoltage(0, false);
}

uint16_t RX_Flow(String resendFrame, bool RESEND) {
  unsigned long currentTime = millis();
  while (BAUD_COUNT < 8) {
    if (not Timer(currentTime, 3000) and RESEND) { // Timer : if time out resend last frame.
      Serial.println("Time out!!!");
      Serial.println("Resend Frame : " + resendFrame);
      TX_Flow(resendFrame);
      return 0;
    }

    uint32_t Voltage = analogRead(A3);//Read analog from analog pin
    if (Voltage > r_slope and previousVoltage < r_slope and not CHECK_AMPLITUDE) { //Found amplitude -> Found Baud
      CHECK_AMPLITUDE = true;
      if (not CHECK_BAUD) {
        BAUD_BEGIN_TIME = micros();
        BIT_CHECK++;
      }
    }
    if (Voltage > r_slope and CHECK_AMPLITUDE) { // Count cycle
      countCycle++;
      CHECK_BAUD = true;
      CHECK_AMPLITUDE = false;
    }
    if (Voltage < r_slope and CHECK_BAUD) {
      if (micros() - BAUD_BEGIN_TIME > 9900) {
        //Serial.println("DATA : "+Frame::BINtoString(16, DATA));
        //Serial.println("nCyvle" + (String)countCycle);
        uint32_t twoBitData = (((countCycle - 5) / 3 ) & 3 ) << (BIT_CHECK * 2);
        //Serial.println("TWOBIT : "+(String)twoBitData);
        DATA |= twoBitData;
        BAUD_COUNT++;
        if (BAUD_COUNT == 8) {
          //Serial.println("DATA : " + (String)DATA);

          Serial.println("RECIEVE FRAME : " + Frame::BINtoString(16, (uint16_t)DATA));
          Serial.flush();
          uint32_t outputData = DATA;
          DATA = 0;
          BAUD_COUNT = 0;
          BIT_CHECK = -1;
          countCycle = 0;
          CHECK_BAUD = false;
          return (uint16_t)outputData; /// return Frame data as INT
        }
        CHECK_BAUD = false;
        countCycle = 0;
      }
    }
    previousVoltage = Voltage;
  }
}
////////////////////////////////////FSK////////////////////////////////
int myseq = 0;
int mode = -1;
int angle = -1;
String frame_arr[30] ;
String UFrame;
int framecounter = 0;
long timer;
void setup() {
  dac.begin(0x62);
  Serial.begin(9600);
  sbi(ADCSRA, ADPS2); // this for increase analogRead speed
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  Serial.println("Press Enter to Scan all data");
  Serial.flush();
}
void RECEIVE(int maxFrame, int nextMode) {
  uint32_t dataFrame = RX_Flow("", false);
  while (dataFrame == 0) {
    dataFrame = RX_Flow("", false);
  }
  Serial.println("Received dataFrame : " + Frame::BINtoString(16, dataFrame));
  String ctrl, seq;
  String decodeddata = Frame::decodeFrame(Frame::BINtoString(16, dataFrame), ctrl, seq);
  if (ctrl.equals("00") and seq.equals((String)myseq) and not decodeddata.equals("Error")) {
    Serial.println("Decoded data : " + decodeddata);
    myseq = (myseq + 1) % 2;
    framecounter++;
  }
  String ACK = Frame::make_ackFrame(myseq);
  TX_Flow(ACK);
  if (framecounter == maxFrame) {
    mode = nextMode;
    framecounter = 0;
    Serial.println("MODE : "+(String)nextMode);
  }
}
void loop() {
  // put your main code here, to run repeatedly:
  if (mode == -1) {

    if (Serial.available()) {
      while (Serial.available()) {
        uint8_t temp = Serial.read();
      }
      mode = 0;
      Serial.println("MODE0");
      Serial.flush();
    }
  }
  if (mode == 0) { //sendUframe to scan/rescan
    String UFrame = Frame::make_UFrame(0);
    TX_Flow(UFrame);
    int receiveData = RX_Flow(UFrame, true);
    while (receiveACK == 0) {
      receiveData = RX_Flow(UFrame, true);
    }
    String decodedFrame = Frame::decodeFrame(Frame::BINtoString(16, receiveData), "", "");
    if(not decodedFrame.equals("Error")){
      mode = 1;
    }
  }
  if (mode == 1) { //receiving data from sender
    RECEIVE(3, 2);
    Serial.println("Pls enter something. :\n 1). Scan -45\n2). Scan 0\n3).Scan 45\n0).ReScan");
  }
  if (mode == 2) { //wait for next command
    if (Serial.available()) {
      String readin = Serial.readStringUntil('\n');
      if (readin.equals("0")) { //reset scanning
        Serial.println("rescanning");
        mode = 0;
        framecounter = 0;
      } else if (readin.equals("1")) {//get -45 data
        Serial.println("scanning -45");
        angle = 1;
        mode = 3;
        framecounter = 0;
      } else if (readin.equals("2")) {//get -45 data
        Serial.println("scanning 0");
        angle = 2;
        mode = 3;
        framecounter = 0;
      } else if (readin.equals("3")) {//get -45 data
        Serial.println("scanning +45");
        angle = 3;
        mode = 3;
        framecounter = 0;
      } else {
        Serial.println("Wrong Input");
      }

    }
  }
  if (mode == 3) { //send specific scan command
    String UFrame = Frame::make_UFrame(angle);
    TX_Flow(UFrame);
    int receiveData = RX_Flow(UFrame, true);
    while (receiveACK == 0) {
      receiveData = RX_Flow(UFrame, true);
    }
    String decodedFrame = Frame::decodeFrame(Frame::BINtoString(16, receiveData), "", "");
    if(not decodedFrame.equals("Error")){
      mode = 4;
    }
  }
  if (mode == 4) {
   RECIEVE(20, 2);
   //Display data
   Serial.println("Pls enter something. :\n 1). Scan -45\n2). Scan 0\n3).Scan 45\n0).ReScan");
  }
}
