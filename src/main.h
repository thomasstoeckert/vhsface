#pragma once

#include "pebble.h"
  
static const GPathInfo MINUTE_HAND_POINTS = {
  3,
  (GPoint []) {
    { -8, 20 },
    { 8, 20 },
    { 0, -60 }
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
    {8, -50},
    {-8, -50},
    {0, -70}
  }
};