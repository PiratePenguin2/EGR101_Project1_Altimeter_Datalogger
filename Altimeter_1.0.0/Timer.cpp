// PT_Util::Timer implementation
#include <Arduino.h>
#include "Timer.h"

Timer::Timer() {
}

Timer::Timer(int mSec) {
  setTimer(mSec);
}

void Timer::setTimer(int mSec) {
  duration = mSec;
  startTime = millis();
}

unsigned long Timer::getStartTime() {
  return startTime;
}

unsigned long Timer::getEndTime() {
  return startTime + duration;
}

unsigned long Timer::getElapsedTime() {
  unsigned long currentTime = millis();
  if (currentTime >= startTime) {
    return currentTime - startTime;
  } else {
    // Handle rollover
    return (4294967295UL - 1 - startTime) + currentTime + 1;
  }
}

unsigned long Timer::getTimeLeft() {
  unsigned long elapsedTime = getElapsedTime();
  if (elapsedTime >= duration) {
    return 0; // Timer finished
  } else {
    return duration - elapsedTime;
  }
}

bool Timer::isFinished() {
  unsigned long currentTime = millis();
  if (currentTime >= startTime) {
    return (currentTime - startTime >= duration);
  } else {
    // Handle rollover
    return (4294967295UL - startTime + currentTime >= duration);
  }
}
