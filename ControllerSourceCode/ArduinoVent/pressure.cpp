
/*************************************************************
 * Open Ventilator
 * Copyright (C) 2020 - Dr. Bill Schmidt-J68HZ, Jack Purdum and Marcelo Varanda
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************
*/


/*
 * History:
 *
  Release 1.01  April 1, 2020, Jack Purdum, W8TEE. Adjusted mapping to 5V from 3V
    Release 1.0   March 30, 2020 by Dr. Bill Schmidt-J68HZ.  This program ws written to interface a MP3V7010xx
                  gauge pressure sensor and will calculate instantaneous average pressures for the
                  Inhale or exhale cycle.  See comments at end for integrating variables to be displayed into the LCD.
*/

#include "pressure.h"
#include "log.h"
#include "hal.h"
#include "config.h"
#include <stdint.h>

#ifdef VENTSIM
    #include<QRandomGenerator>
    #define TEST_RAND_MIN 300
    #define TEST_RAND_MAX 400

#endif

#ifdef PREESURE_ENABLE

#define SHOW_VAL

#define TM_LOG 2000
#define P_CONV 4.01463f
#define MAX_BIN_INPUT   614
#define MAX_BIN_INPUT_F 614.0

static int16_t tap_array[AVERAGE_BIN_NUMBER];
static int32_t accumulator = 0;
static uint8_t head_idx = 0;
static uint8_t tail_idx = 0;
static uint8_t ready_cnt = 0;
static uint64_t tm_press;

static int32_t av;
static int16_t rawSensorValue;
static float inH2O = 0.0f;


#ifdef SHOW_VAL
  static uint64_t tm_log;
#endif

void CalculateAveragePressure()
{
    //aV= 12, Pa=174.86... 0~614 --> -3.57~41.08 inches of water

#ifdef VENTSIM
  rawSensorValue = QRandomGenerator::global()->bounded(TEST_RAND_MIN, TEST_RAND_MAX);
#else
  rawSensorValue = analogRead(PRESSURE_SENSOR_PIN);  //Raw digital input from pressure sensor
#endif

  // clamp it to the max (max value provided by the sensor)
  if (rawSensorValue >= MAX_BIN_INPUT)
      rawSensorValue = MAX_BIN_INPUT - 1;

  if (ready_cnt >= AVERAGE_BIN_NUMBER)  {
    accumulator -= tap_array[tail_idx++] ;
    if (tail_idx >= AVERAGE_BIN_NUMBER) tail_idx = 0;
  }
  else {
    ready_cnt++;
  }

  tap_array[head_idx++] = rawSensorValue;
  if (head_idx >= AVERAGE_BIN_NUMBER) head_idx = 0;
  accumulator += rawSensorValue;

  av = accumulator/AVERAGE_BIN_NUMBER;
  inH2O = P_CONV * ((av / MAX_BIN_INPUT_F) - 0.08) / 0.09;
}


//====================================================================
void pressInit()   {
#ifndef VENTSIM
  analogReference(DEFAULT);
#endif
  tm_press = halStartTimerRef();

#ifdef SHOW_VAL
  tm_log = tm_press;
#endif
}


void pressLoop()
{
  if (halCheckTimerExpired(tm_press, PRESSURE_READ_DELAY)) {
    CalculateAveragePressure();
    tm_press = halStartTimerRef();
  }

#ifdef SHOW_VAL
  char buf[24];
  if (halCheckTimerExpired(tm_log, TM_LOG)) {
    LOGV("av = %d", av);
#ifndef VENTSIM
    dtostrf(inH2O, 8, 2, buf);
    LOGV("Pa = %s\n", buf);
#else
    LOGV("Pa = %f\n", inH2O);
#endif
    tm_log = halStartTimerRef();
  }
#endif
}

float pressGetFloatVal() // in InchH2O
{
    return inH2O;
}
int pressGetRawVal()
{
    return av;
}


//-----------------------------------------------------------------
#else
// Stubbs
void pressInit()   { }
void pressLoop() {}
float pressGetFloatVal() { return 0.0; }
int pressGetRawVal() { return 0; }

#endif //#ifndef PREESURE_ENABLE
