#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "debugger.hpp"
#include "io.hpp"

#ifndef _WIN32
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif
#include "boost/filesystem.hpp"
#undef BOOST_NO_CXX11_SCOPED_ENUMS

#include <memory>
#include <unordered_map>

namespace vivacity
{

    // Config file types
enum class Model { MAIN, HOG, BGS, HUNGARIAN, COLOR_HIST, LABELER };

class ConfigManager
{
    public:
        explicit ConfigManager( const std::shared_ptr<Debug> &debug_ptr, 
            const std::string &config_name = std::string( "config.xml" ), 
            const std::string &config_location = std::string( "./" ),
            const std::string &models_location = std::string ( "./models/" ) );

        std::string addModel( Model model_type,
            const std::string &model_name = "" );

        bool hasEntry( const std::string &entry,
            const std::string &model_name = "" );

        // Read value from XML config file (must pass value by reference)
        template<class T>
        T read( const std::string &entry, const std::string &model_name = "" )
        {
            T value;
            std::string config_path = getModelPath( model_name );
            cv::FileStorage xml_reader( config_path, cv::FileStorage::READ );

            cv::FileNode node( xml_reader[entry] );
            if ( !node.empty() )
            {
                xml_reader[entry] >> value;
            }
            else
            // Append default value to the config
            {
                debug->print( "WARNING: No entry for \"" + entry + "\" found in "
                    + config_path + "! Will attempt to use default value instead.", 1 );

                xml_reader.release();
                T default_value;
                default_value = Defaults::getDefault<T>( entry );
                xml_reader.open( config_path, cv::FileStorage::APPEND );
                xml_reader << entry << default_value;
                value = default_value;
            }

            xml_reader.release();
            return value;
        }

        // Writes a value to a config file
        template<class T>
        void write( const std::string &entry, T value,
            const std::string &model_name = "" )
        {
            std::string config_path = getModelPath( model_name );
            cv::FileStorage xml_reader( config_path, cv::FileStorage::READ );
            cv::FileNode node( xml_reader[entry] );
            xml_reader.release();

            if ( node.empty() )
            {
                cv::FileStorage xml_writer( config_path, cv::FileStorage::APPEND );
                xml_writer << entry << value;
                xml_writer.release();
            }
            else
            {
                // Update the entry 
                deleteEntry( entry, model_name );
                cv::FileStorage xml_writer( config_path, cv::FileStorage::APPEND );
                xml_writer << entry << value;
                xml_writer.release();
            }
        }

    private: 
        void updateDebug();
        void deleteEntry( const std::string &entry,
            const std::string &model_name = "");

        std::string getModelPath( const std::string &model_name );
        void buildConfig( Model model_type, const std::string &config_location );

        std::string generateFileFromName( std::stringstream &config_stream,
            const std::string &model_name, const std::string &base_name );

        std::string config_file;
        std::string config_path;
        std::string models_path;
        std::unordered_map<std::string, std::string> model_configs;
        std::shared_ptr<Debug> debug;
};



}
#endif
