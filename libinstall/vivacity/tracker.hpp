#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "io.hpp"
#include "trackers.hpp"

#include "boost/circular_buffer.hpp"
#include "boost/filesystem.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <vector>

namespace vivacity
{

class TrackedObject
{
    public:

        TrackedObject( std::shared_ptr<Debug> &debugger, int id_number,
            const cv::Mat &frame = cv::Mat(), const cv::Rect &roi = cv::Rect() );
        void addInstance( const cv::Mat &frame, const cv::Rect &roi );

        cv::Rect getLastRect() const;

        cv::Point getLastPosition() const;

        double getSpeedPPS( const cv::Mat &persepective_transform
            = cv::Mat() ) const;
        double getSpeedMPH( const cv::Mat &perspective_transform ) const;

        int getLastSeen() const
        {
            return last_seen;
        }
        void resetLastSeen()
        {
            last_seen = 0;
        }
        void incrementLastSeen()
        {
            ++last_seen;
        }
        int getId() const
        {
            return id;
        }
        std::vector<cv::Point> getPath( const int &compact_size = 0 ) const;
        std::vector<std::pair<cv::Point, long long int> > getPathTimes(
            const int &compact_size = 0 ) const;

    protected:

        std::shared_ptr<Debug> debug;
        int id;
        int last_seen;
        std::vector<cv::Rect> detection_locations;
        std::vector<cv::Point> path;
        std::vector < std::pair<cv::Point, long long int> > path_times;
};

struct Match
{
    Match( const int &object_id, const int &new_detection_index,
        const double matching_score )
        : old_object( object_id )
        , new_object( new_detection_index )
    {}

    int old_object;
    int new_object;
    double match_score;
};

class Tracker
{
    public:
        Tracker( std::shared_ptr<Debug> &debugger, std::shared_ptr<ConfigManager>
            &config_manager, const std::string &name = "" );

        virtual void initialise( const cv::Mat &first_frame, const
            std::vector<cv::Rect> &first_detections );

        virtual void track( const cv::Mat &frame,
            const std::vector<cv::Rect> &detections ) = 0;

        bool isInitialised()
        {
            return initialised;
        }
        std::list<TrackedObject> getObjects()
        {
            return objects;
        }
        std::string getModelName()
        {
            return model_name;
        }
        int getLookback()
        {
            return lookback;
        }

    protected:

        virtual void updateFromConfig() = 0;

        void incrementObjectCount()
        {
            ++object_count;
        }
        int getObjectCount()
        {
            return object_count;
        }

        bool initialised;
        std::list<TrackedObject> objects;
        std::shared_ptr<Debug> debug;
        std::shared_ptr<ConfigManager> cfg_mgr;
        std::string model_name;
        int lookback;
        boost::circular_buffer<cv::Mat> past_images;

    private:

        int object_count;
};

}

#endif
