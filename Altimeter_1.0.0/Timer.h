// Timer.h
#ifndef TIMER_H
#define TIMER_H

class Timer {
  public:
    Timer();
    Timer(int mSec);
    void setTimer(int mSec);
    unsigned long getStartTime();
    unsigned long getEndTime();
    unsigned long getElapsedTime();
    unsigned long getTimeLeft();
    bool isFinished();
  private:
    int duration;
    unsigned long startTime;
};

#endif // TIMER_H
