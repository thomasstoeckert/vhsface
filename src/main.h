#pragma once

#include "pebble.h"
#if PBL_SDK_3
static const GPathInfo MINUTE_HAND_POINTS = {
  3,
  (GPoint []) {
    { -8, 20 },
    { 8, 20 },
    { 0, -70 }
  }
};
static const GPathInfo HOUR_HAND_POINTS = {
  3, (GPoint []){
    {-6, 20},
    {6, 20},
    {0, -50}
  }
};
static const GPathInfo PER_END_POINTS = {
  3,
  (GPoint []){
    {8, -50},
    {-8, -50},
    {0, -70}
  }
};
#elif PBL_SDK_2
static const GPathInfo MINUTE_HAND_POINTS = {
  3,
  (GPoint []) {
    { -8, 20 },
    { 8, 20 },
    { 0, -54 }
  }
};
static const GPathInfo HOUR_HAND_POINTS = {
  3, (GPoint []){
    {-6, 20},
    {6, 20},
    {0, -40}
  }
};
static const GPathInfo PER_END_POINTS = {
  3,
  (GPoint []){
    {8, -40},
    {-8, -40},
    {0, -55}  
  }
};
#endif