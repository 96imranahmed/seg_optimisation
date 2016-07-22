#ifndef LABELER_HPP
#define LABELER_HPP

#include "detectors.hpp"

#include <random>

namespace vivacity
{

struct Box
{
    Box( const cv::Rect &r, bool pos ) :
        rect( r ),
        is_positive(pos)
    {}

    cv::Rect rect;
    bool is_positive;
};

class EventHandler
{
    public:

        EventHandler( std::shared_ptr<Debug> &debugger, int width,
            int height )
            : debug( debugger )
            , width_ratio( width )
            , height_ratio( height )
            , tl_set( false )
        {}

        void onLeftClick( cv::Point pt, int flags );
        void onRightClick();
        void onMouseMove( cv::Point pt, int flags );
        void deleteLastBox();
        std::vector<cv::Mat> getSamples( bool isPositive,
            int neg_sample_count = -1 );

        void setFrame( cv::Mat &image, const std::vector<cv::Rect> &rects =
            std::vector<cv::Rect>() )
        {
            frame = image;
            frame.copyTo( original_frame );
            detections = rects;
            drawBoxes();
        }

    private:

        void drawBoxes();
        void generateNegativeSamples( int count );

        std::shared_ptr<Debug> debug;
        std::list<Box> boxes;
        cv::Mat frame;
        cv::Mat original_frame;
        cv::Point current_tl;
        cv::Point current_br;
        bool tl_set;
        int width_ratio;
        int height_ratio;
        std::vector<cv::Rect> detections;
};

class Labeler
{
    public:

        Labeler( std::shared_ptr<Debug> &debugger, 
            std::shared_ptr<ConfigManager> &cfg_manager,
            std::shared_ptr<Detector> detect_ptr = nullptr,
            const std::string &name = "");

        void run();

    private:

        void updateFromConfig();
        void saveImagesFromBoxes();
        void processKey( int key, EventHandler &handler );
        void saveSamples( EventHandler &handler, bool skip );

        std::shared_ptr<Debug> debug;
        std::shared_ptr<ConfigManager> cfg_mgr;
        int skip_frames;
        int window_ratio_width;
        int window_ratio_height;
        int negative_sample_count;
        std::string positive_dir;
        std::string negative_dir;
        std::string model_name;
        std::string window_name;
        bool labeling;
        bool end;
        std::shared_ptr<Detector> detector;
};

}

#endif