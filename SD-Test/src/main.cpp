#define K32_SET_NODEID        107   // board unique id    (necessary one time only)
#define K32_SET_HWREVISION    2   // board HW revision  (necessary one time only)

#include "K32.h"

K32* k32;


void setup() {

  k32 = new K32();

  k32->init_stm32();

    delay(5000);
  k32->init_sd();

}

void loop() {

  delay(2000);

  byte* buffer;
  int size = k32->sd->readFile("test.bmp", buffer);

  LOG("File read complete, size: "+String(size) );
  LOG("File content:");
  for(int k = 0; k<size; k++) {
    LOGINL(buffer[k]);
    LOGINL(" ");
  }
  LOG();

  // Serial.println(ESP.getFreeHeap()); // print la memoire ram dispo 

} 