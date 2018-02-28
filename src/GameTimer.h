#ifndef __GAME_TIMER_H
#define __GAME_TIMER_H

class GameTimer
{
public:
    GameTimer();

    void Start();
    void Stop();
    void Reset();
    void Tick( );

    float GetLiveTime( );
    float GetDeltaTime( );

    bool IsPaused();

    // TODO we don't have any engine class yet, so we put main timer here
    static GameTimer &GetAppTimer();
private:
    float mSecondsPerCount;
    float mDeltaTime;

    __int64 mBaseTime;
    __int64 mPausedTime;
    __int64 mStopTime;
    __int64 mPrevTime;
    __int64 mCurrTime;

    bool mStopped;
};

#endif