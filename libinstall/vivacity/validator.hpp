#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "trackers.hpp"

namespace vivacity
{

class Validator
{
    public:
        Validator(std::shared_ptr<Debug> &debugger,
            std::shared_ptr<ConfigManager> &config_manager,
            const std::string &tracker_name );

        virtual void validate_objects( const std::list<
            TrackedObject> &objects ) = 0;

        bool is_valid( int object_id );

    protected:

        virtual void updateFromConfig( const std::string
            &tracker_name ) = 0;

        std::shared_ptr<Debug> debug;
        std::shared_ptr<ConfigManager> cfg_mgr;
        int validation_thresh;
        std::unordered_map<int, int> validation_counts;
        std::list<int> valid_objects;
};

class PedestrianValidator : public Validator
{
    public:

        PedestrianValidator( std::shared_ptr<Debug> &debugger,
            std::shared_ptr<ConfigManager> &config_manager,
            const std::string &tracker_name );

        virtual void updateFromConfig( const std::string &tracker_name );

        virtual void validate_objects( const std::list<TrackedObject>
            &objects );

    private:

        double  min_ratio;
        double  max_ratio;
        double  min_area;
        double  max_area;
};

}

#endif