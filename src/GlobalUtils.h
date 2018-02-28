#ifndef __GLOBAL_UTILS_H
#define __GLOBAL_UTILS_H

#include <assert.h>
#include <fstream>
#include <ctime>
#include <string>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline void COMSafeRelease( T &p )
{
    if ( p != nullptr )
    {
        p->Release( );
        p = nullptr;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// note: this logger isn't optimal but simple
// better to take external logger
enum LoggerLevel
{
    LOG_INFO,
    LOG_ERROR
};

void WriteToFile( std::ofstream &file );
bool CheckLogFile( std::ofstream &file, const char *fn );
std::ofstream* GetOfstream( LoggerLevel level );
void SplitFilename( const std::string &str, std::string &path, std::string &file );
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename... Types>
void WriteToFile( std::ofstream &file, const T& firstArg, const Types&... args )
{
    file << firstArg;
    WriteToFile( file, args... );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename... Types>
void WriteLog( LoggerLevel level, const char *moduleName, int line, const Types&... args )
{
    std::ofstream *file = GetOfstream( level );
    if ( !file )
        return;

    /* note: can save localtime
    time_t t = time( 0 );   // get time now
    struct tm *now = localtime( &t );

    *file << "Day " << now->tm_mday << " " << now->tm_hour << "." << now->tm_min << " " << line << ": ";
    */
    *file << moduleName << " (" << line << "): ";
    WriteToFile( *file, args... );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LOG( level, ... ) WriteLog(level, __FILE__, __LINE__, __VA_ARGS__);
#define LOG_INFO( ... ) LOG( LOG_INFO, __VA_ARGS__ )
#define LOG_ERROR( ... ) LOG( LOG_ERROR, __VA_ARGS__ )
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ASSERT_( condition, ... ) if ( !(condition) ) { LOG_ERROR( "\tAssert! Condition: ", #condition, "\t", __VA_ARGS__); assert(false); }
#define ASSERT( condition, ... ) ASSERT_( condition, "", __VA_ARGS__ )
#define WARNING( condition, ... ) if ( condition ) LOG_INFO( "\tWarning! Condition: ", #condition, "\t", __VA_ARGS__ )
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename T >
T Clamp( T x, T min, T max )
{
    if ( min > max )
    {
        T sw = min;
        min = max;
        max = sw;
    }
    return x > max ? max : x < min ? min : x;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float PI = 3.1415926f;
const float PI_2 = 1.5707963f;
const float EPS = 0.00001f;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define GET_FX_VAR( loadingCheck, var, fxValue ) { var = fxValue; loadingCheck &= var->IsValid(); ASSERT( loadingCheck ); }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif