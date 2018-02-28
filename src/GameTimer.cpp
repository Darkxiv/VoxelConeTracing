#include <GameTimer.h>
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GameTimer::GameTimer( ) : 
    mStopped(true),
    mDeltaTime(0),
    mCurrTime(0),
    mPrevTime(0),
    mPausedTime(0)
{
    __int64 countsPerSec = 0;
    QueryPerformanceFrequency( ( LARGE_INTEGER* )( &countsPerSec ) );
    mSecondsPerCount = 1.0f / countsPerSec;

    // starting point
    QueryPerformanceCounter( ( LARGE_INTEGER* )( &mBaseTime ) );
    mStopTime = mBaseTime;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GameTimer::Start()
{
    if ( mStopped )
    {
        __int64 currTime = 0;
        QueryPerformanceCounter( ( LARGE_INTEGER* )( &currTime ) );

        mPausedTime += currTime - mStopTime;
        mPrevTime = currTime;
        mCurrTime = mStopTime;
        mStopTime = 0;

        mStopped = false;
    }

    mDeltaTime = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GameTimer::Stop()
{
    if ( mStopped )
        return;

    QueryPerformanceCounter( ( LARGE_INTEGER* )( &mStopTime ) );
    mDeltaTime = 0;
    mPrevTime = mStopTime;
    mCurrTime = mStopTime;
    mStopped = true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GameTimer::Reset( )
{
    __int64 currTime = 0;
    QueryPerformanceCounter( ( LARGE_INTEGER* )( &currTime ) );

    mBaseTime = currTime;
    mCurrTime = currTime;
    mPrevTime = currTime;

    mDeltaTime = 0;
    mPausedTime = 0;
    mStopTime = 0;

    mStopped = false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GameTimer::Tick()
{
    if ( mStopped )
    {
        mDeltaTime = 0;
        return;
    }

    QueryPerformanceCounter( ( LARGE_INTEGER* )( &mCurrTime ) );
    mDeltaTime = max( 0, ( mCurrTime - mPrevTime ) * mSecondsPerCount );
    mPrevTime = mCurrTime;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float GameTimer::GetLiveTime()
{
    QueryPerformanceCounter( ( LARGE_INTEGER* )( &mCurrTime ) );
    return max( 0, ( mCurrTime - mBaseTime - mPausedTime ) * mSecondsPerCount );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float GameTimer::GetDeltaTime()
{
    return mDeltaTime;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool GameTimer::IsPaused()
{
    return mStopped;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GameTimer& GameTimer::GetAppTimer( )
{
    static GameTimer appTimer;
    return appTimer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////