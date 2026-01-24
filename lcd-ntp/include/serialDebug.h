#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H

#include <Arduino.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#if (DEBUG == 1) // debug
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif


#ifndef UART_SPD
#define UART_SPD 115200
#endif



void initSerial()
{
#if (DEBUG == 1)
  Serial.begin(UART_SPD);
  // delay(10);
  // Serial.flush();
  Serial.println();
#endif
}


#endif // SERIAL_DEBUG_H