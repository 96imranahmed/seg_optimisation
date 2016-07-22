#ifndef DEFAULTS_HPP
#define DEFAULTS_HPP

#include "debugger.hpp"
#include "io.hpp"

#include <map>

namespace vivacity
{        

// This struct helps to enable having type-specific as well
// as default behaviour for the getDefault template method.
template<class T>
struct identity { typedef T type; };

class Defaults
{   
    public:

        template <class T>
        static T getDefault( const std::string &entry )
        {
            return getDefault( entry, identity<T>() );
        }

    private:

        static void initialise();

        // This generic method should never be called.
        // Means the wrong type was invoked for fetching a default value.
        template<class T>
        static T getDefault( const std::string &entry, identity<T> )
        {
            Debug::print( "ERROR: No parameter of this type exists in the "
                " Vivacity library!" );
            throw std::exception();
            T t;
            return t;
        }

        static int getDefault( const std::string &entry, identity<int> )
        {
            if ( !initialised )
            {
                initialise();
            }
            // Could not find entry
            if ( int_map.find( entry ) == int_map.end() )
            {
                Debug::print( "ERROR: Cannot read non-existant config entry \""
                    + entry + "\". No matching default value!" );
                throw std::exception();
            }
            return int_map[entry];
        }

        static bool getDefault( const std::string &entry, identity<bool> )
        {
            if ( !initialised )
            {
                initialise();
            }
            // Could not find entry
            if ( bool_map.find( entry ) == bool_map.end() )
            {
                Debug::print( "ERROR: Cannot read non-existant config entry \""
                    + entry + "\". No matching default value!" );
                throw std::exception();
            }
            return bool_map[entry];
        }

        static double getDefault( const std::string &entry, identity<double> )
        {
            if ( !initialised )
            {
                initialise();
            }
            // Could not find entry
            if ( double_map.find( entry ) == double_map.end() )
            {
                Debug::print( "ERROR: Cannot read non-existant config entry \""
                    + entry + "\". No matching default value!" );
                throw std::exception();
            }
            return double_map[entry];
        }
        
        static std::string getDefault( const std::string &entry,
            identity<std::string> )
        {
            if ( !initialised )
            {
                initialise();
            }
            // Could not find entry
            if ( str_map.find( entry ) == str_map.end() )
            {
                Debug::print( "ERROR: Cannot read non-existant config entry \""
                    + entry + "\". No matching default value!" );
                throw std::exception();
            }
            return str_map[entry];
        }

        static cv::Mat getDefault( const std::string &entry, identity<cv::Mat> )
        {
            if ( !initialised )
            {
                initialise();
            }
            // Could not find entry
            if ( mat_map.find( entry ) == mat_map.end() )
            {
                Debug::print( "ERROR: Cannot read non-existant config entry \""
                    + entry + "\". No matching default value!" );
                throw std::exception();
            }
            return mat_map[entry];
        }

        static bool initialised;
        static std::map<std::string, int> int_map;
        static std::map<std::string, bool> bool_map;
        static std::map<std::string, double> double_map;
        static std::map<std::string, std::string> str_map;
        static std::map<std::string, cv::Mat> mat_map;
};
}

#endif
