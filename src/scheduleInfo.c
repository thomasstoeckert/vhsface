//#pragma once
#include <pebble.h>

static const char *sched_p_label[] = {
  "1st",
  "2nd",
  "3rd",
  "4th",
  "5th",
  "6th",
  "7th"
};
  
static const char *sched_reg_p_start[] = {
  "8:45",
  "9:38",
  "10:31",
  "11:24",
  "11:24",
  "1:47",
  "2:40"
};
static const char *sched_reg_p_end[] = {
  "9:34",
  "10:27",
  "11:20",
  "12:13",
  "1:43",
  "2:36",
  "3:30"
};

static const int sched_reg_times[] = {
  845,
  934,
  1027,
  1120,
  1213,
  1343,
  1446,
  1530
};

static const char *sched_shr_p_start[] = {
  "8:45",
  "9:26",
  "10:06",
  "10:46",
  "11:26",
  "12:56",
  "1:37"
};
static const char *sched_shr_p_end[] = {
  "9:22",
  "10:02",
  "10:42",
  "11:22",
  "12:52",
  "13:33",
  "14:15"
};
static const int sched_shr_times[] = {
  845,
  922,
  1002,
  1042,
  1122,
  1252,
  1333,
  1415
};
