#ifndef Frame_H
#define Frame_H
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#define dataframe 0 //dataframe->00
#define ack 1 //ack->01
#define choose 2 //choose->10
#define all 3 //all->11
class Frame{
    public:
    
        static const uint8_t seq = 0; //seq number
        
        static String make_dataFrame(int);
        static String make_ackFrame();
        static String make_UFrame(int);
        static String decodeFrame(String,String&,String&);
        static String BINtoString(int,int);
};
#endif
