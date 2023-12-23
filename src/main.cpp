#include "main.h"

#define GPS_TX D6 // serial RX
#define GPS_RX D8 // serial TX

#define SR_DATA D2
#define SR_LATCH D1
#define SR_CLK D0

SoftwareSerial ss(GPS_TX, GPS_RX);

void setup() {
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    pinMode(SR_DATA, OUTPUT);
    pinMode(SR_CLK, OUTPUT);
    pinMode(SR_LATCH, OUTPUT);

    Serial.begin(115200);
    while (!Serial)
        delay(10);
    cacheNumbers();

    ss.begin(9600);
}



const unsigned char UBX_HEADER[]        = { 0xB5, 0x62 };


struct NAV_VELNED {
  unsigned char cls;    // don't override
  unsigned char id;     // don't override
  unsigned short len;   // don't override
  unsigned long iTOW;
  long velN;
  long velE;
  long velD;
  unsigned long speed;
  unsigned long gSpeed; // cm/s
  long heading;
  unsigned long sAcc;
  unsigned long cAcc;
};

NAV_VELNED velned;
void calcChecksum(unsigned char* CK) {
  memset(CK, 0, 2);
  for (int i = 0; i < (int)sizeof(NAV_VELNED); i++) {
    CK[0] += ((unsigned char*)(&velned))[i];
    CK[1] += CK[0];
  }
}

bool processGPS() {
  static int fpos = 0;
  static unsigned char checksum[2];
  const int payloadSize = sizeof(NAV_VELNED);

  while ( ss.available() ) {
    byte c = ss.read();
    if ( fpos < 2 ) {
      if ( c == UBX_HEADER[fpos] )
        fpos++;
      else
        fpos = 0;
    }
    else {
      if ( (fpos-2) < payloadSize )
        ((unsigned char*)(&velned))[fpos-2] = c;

      fpos++;

      if ( fpos == (payloadSize+2) ) {
        calcChecksum(checksum);
      }
      else if ( fpos == (payloadSize+3) ) {
        if ( c != checksum[0] )
          fpos = 0;
      }
      else if ( fpos == (payloadSize+4) ) {
        fpos = 0;
        if ( c == checksum[1] ) {
          return true;
        }
      }
      else if ( fpos > (payloadSize+4) ) {
        fpos = 0;
      }
    }
  }
  return false;
}

unsigned long gSpeed = 0;
bool numD1[20][8];
bool numD2[20][8];
bool numD3[20][8];
String s = "";
int t = 0;
int c;
int d;
int u; 
int speed = 0;

int dpC= 0;
void loop() {

    
    if ( processGPS() ) {
        gSpeed = velned.gSpeed * 0.036; 
        Serial.print("speed ");       
        Serial.print(gSpeed);
        Serial.print("\n");
    }
    
    speed = gSpeed;
    
    u = speed % 10; 
    speed = speed / 10; 
    d = speed % 10; 
    speed = speed / 10; 
    c = speed; 

    if(t== 0)
    {
        // if(dpC == 0)
        //     sendSR(-u,-d,-c, gSpeed);
        // else
            sendSR(u,d,c,gSpeed);

        // Serial.print(gSpeed);
        // Serial.print("\n");
        // gSpeed++;
        // gSpeed%=999;

        dpC++;
        dpC%=5;


    }
    t++;
    t%=60;

    delay(1);

}

void send(bool* data)
{
    for(int i = 0; i < 8; i++)
    {
        digitalWrite(SR_DATA,data[7-i]);
        digitalWrite(SR_CLK,HIGH);
        digitalWrite(SR_CLK, LOW);//reset
    }
    digitalWrite(SR_LATCH, HIGH);
    digitalWrite(SR_LATCH, LOW);//reset
}
bool zero[8]= {0,0,0,0,0,0,0,0};

void sendSR(int d1, int d2, int d3, int speed)
{
    bool dp = d3 < 0;

    int a = abs(d3);

    zero[7] = dp;

    if(d3 == 0)
        send(zero);
    else
        send(getNumber(a, dp, 2));

    dp = d2 < 0;

    a = abs(d2);

    if(d2 == 0 && speed <100)
        send(zero);
    else
        send(getNumber(a, dp, 1));
       
    dp = d1 < 0;

    a = abs(d1);


    send(getNumber(a, dp, 0));
    
        
    
}

int currentPins[3][8] = {
    {6, 5, 0, 1, 7, 2, 3, 4},
    {4, 3, 6, 5, 0, 1, 2, 7}, 
    {7, 2, 3, 4, 6, 5, 0, 1}};



bool* getNumber(int n, bool dp, int digit)
{
    int i = n;

    if(dp)
        i+=10;
    switch(digit)
    {
        case 0 : return numD1[i];
        case 1 : return numD2[i];
        case 2 : return numD3[i];
        
    }
    return numD1[i];
    
}
void cacheNumbers()
{
    int index;
    bool dp;
    for(int digit = 0; digit < 3; digit++)
    {
        Serial.print("digit ");
        Serial.print(digit);
        Serial.print("\n");
        for(int i = 0; i<20; i++)
        {
            index=i;
            dp = i >=10;
            if(dp)
                index-=10;
            bool *number= translateSR(getSRFromNumberInv(index, dp), digit);
            Serial.print(i);
            Serial.print(" [");
            Serial.print((int)number);
            Serial.print(" ] ");
            for(int j = 0; j<8;j++){
                switch(digit)
                {
                    case 0 : numD1[i][j] = number[j];break;
                    case 1 : numD2[i][j] = number[j];break;
                    case 2 : numD3[i][j] = number[j];break;
                    
                }
                Serial.print(number[j]);
            }
            Serial.print("\n");
            //free(number);
        }
    }
}

bool* translateSR(bool * from, int digit)
{
    bool * res = (bool*)malloc(8 * sizeof(bool));
    for(int i = 0; i < 8; i++)
    {
        int truePin = currentPins[digit][i];
        res[i] = from[truePin];
    }
    //free(from);
    return res;
}


bool* getSRFromNumber(int num, bool dp)
{
    bool* res = (bool*)malloc(8 * sizeof(bool));
    switch (num)
    {
        case 0:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true;
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = false;
            break;
        case 1:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = false; 
            res[1] = true; 
            res[2] = true; 
            res[3] = false; 
            res[4] = false; 
            res[5] = false; 
            res[6] = false;
            break;  
        case 2:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = true; 
            res[2] = false; 
            res[3] = true; 
            res[4] = true; 
            res[5] = false; 
            res[6] = true;
            break;
        case 3:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = false; 
            res[5] = false; 
            res[6] = true;
            break;
        case 4:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = false; 
            res[1] = true; 
            res[2] = true; 
            res[3] = false; 
            res[4] = false; 
            res[5] = true; 
            res[6] = true;
            break;
        case 5:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = false; 
            res[2] = true; 
            res[3] = true; 
            res[4] = false; 
            res[5] = true; 
            res[6] = true;
            break;  
        case 6:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = false; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;

            break;
        case 7:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = false; 
            res[4] = false; 
            res[5] = false; 
            res[6] = false;
            break;
        case 8:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;
            break;
        case 9:
            //a             //b             //c            //d          //e           //f         //g
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = false; 
            res[4] = false; 
            res[5] = true; 
            res[6] = true;
            break;
            
    }
    res[7] = dp;
    return res;
}

bool* getSRFromNumberInv(int num, bool dp)
{
    bool* res = (bool*)malloc(8 * sizeof(bool));
    switch (num)
    {
        case 0:
            res[0] = true;
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = false;
            break;
        case 1:
            res[0] = false; 
            res[1] = false; 
            res[2] = false; 
            res[3] = false; 
            res[4] = true; 
            res[5] = true; 
            res[6] = false;
            break;  
        case 2:
            res[0] = true; 
            res[1] = false; 
            res[2] = true; 
            res[3] = true; 
            res[4] = false; 
            res[5] = true; 
            res[6] = true;
            break;
        case 3:
            res[0] = true; 
            res[1] = false; 
            res[2] = false; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;
            break;
        case 4:
            res[0] = false; 
            res[1] = true; 
            res[2] = false; 
            res[3] = false; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;
            break;
        case 5:
            res[0] = true; 
            res[1] = true; 
            res[2] = false; 
            res[3] = true; 
            res[4] = true; 
            res[5] = false; 
            res[6] = true;
            break;  
        case 6:
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = false; 
            res[6] = true;

            break;
        case 7:
            res[0] = true; 
            res[1] = false; 
            res[2] = false; 
            res[3] = false; 
            res[4] = true; 
            res[5] = true; 
            res[6] = false;
            break;
        case 8:
            res[0] = true; 
            res[1] = true; 
            res[2] = true; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;
            break;
        case 9:
            res[0] = true; 
            res[1] = true; 
            res[2] = false; 
            res[3] = true; 
            res[4] = true; 
            res[5] = true; 
            res[6] = true;
            break;
            
    }
    res[7] = dp;
    return res;
}
