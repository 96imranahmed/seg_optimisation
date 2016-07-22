#ifndef SQL_HPP
#define SQL_HPP

#include "debugger.hpp"
#include "io.hpp"

#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/prepared_statement.h"
#include "mysql_driver.h"

#include <mutex>
#include <thread>

namespace vivacity
{

class FieldList
{
    public:
        FieldList( const std::vector<std::string> &field_list );
        std::string serialize();
        int getSize()
        {
            return fields.size();
        }

    private:
        std::vector<std::string> fields;
};

class SQLPackage
{
    public:
        SQLPackage( std::shared_ptr<FieldList> field_list,
            const std::vector<std::string> &value_list );
        std::string serialize();
        std::string getFieldData();

        std::shared_ptr<FieldList> getFields()
        {
            return fields;
        }

    private:
        std::shared_ptr<FieldList> fields;
        std::vector<std::string> values;
};

class SQLUploader
{
    public:
        SQLUploader( std::shared_ptr<Debug> &debugger,
            std::shared_ptr<ConfigManager> &config_manager,
            const std::vector<std::string> &fields,
            const std::string &url, const std::string &db_schema,
            const std::string &table_name, const std::string &user,
            const std::string &password );
        void work( std::exception_ptr &exc_ptr, std::mutex &exc_mutex );
        void queueRow( const std::vector<std::string> &values );
        void upload();

    private:
        char* serialize_batch();

        std::shared_ptr<Debug> debug;
        std::shared_ptr<ConfigManager> cfg_mgr;
        std::shared_ptr<FieldList> field_list;
        std::list<SQLPackage> packages;
        std::list<SQLPackage> backup;
        std::mutex data_mutex;
        const std::string url;
        const std::string db_name;
        const std::string schema;
        const std::string username;
        const std::string password;
        int buffer_limit;
        int cache_limit;
        int time_limit;
        Timer timer;
        Timer log_timer;
};

}

#endif
