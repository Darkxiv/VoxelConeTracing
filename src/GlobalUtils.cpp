#include <GlobalUtils.h>

static std::ofstream errorFile;
static std::ofstream infoFile;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::ofstream* GetOfstream( LoggerLevel level )
{
    std::ofstream *file = &infoFile;
    const char *fn = "logInfo.txt";
    switch ( level )
    {
    case LOG_INFO:
        file = &infoFile;
        fn = "logInfo.txt";
        break;
    case LOG_ERROR:
        file = &errorFile;
        fn = "logError.txt";
        break;
    default:
        assert( false );
        return nullptr;
    }

    bool success = CheckLogFile( *file, fn );
    assert( success );
    if ( !success )
        return nullptr;

    return file;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WriteToFile( std::ofstream &file )
{
    // close up recursion
    file << std::endl;
    file.flush();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CheckLogFile( std::ofstream &file, const char *fn )
{
    if ( !file.is_open( ) )
    {
        file.open( fn, std::fstream::out );
        if ( file.fail( ) )
            return false;
    }
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SplitFilename( const std::string &str, std::string &path, std::string &file )
{
    std::size_t found = str.find_last_of( "/\\" );
    if ( found != std::string::npos )
    {
        path = str.substr( 0, found + 1 );
        file = str.substr( found + 1 );
    }
    else
    {
        file = str;
        path = "";
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
