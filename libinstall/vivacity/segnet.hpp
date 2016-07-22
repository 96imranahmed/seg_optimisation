#ifndef SEGNET_HPP
#define SEGNET_HPP

#include "detectors.hpp"

namespace vivacity
{

class Segnet : public Detector
{
    public: 

        enum class Class { ALL, PEDESTRIAN, CYCLIST, CAR };

        Segnet( std::shared_ptr<Debug> &debug_ptr,
            std::shared_ptr<ConfigManager> &cfg_ptr,
            const std::string &name = "" );

        cv::Mat processImage( const cv::Mat &image );
        virtual std::vector<cv::Rect> detect( const cv::Mat &image );
        std::vector<cv::Rect> detect( const cv::Mat &image, cv::Mat &mask,
            Class type = Class::ALL );
        std::vector<std::vector<cv::Point> > detectContours( const cv::Mat 
            &image, cv::Mat &mask, Class type = Class::ALL );
        cv::Mat getMask( Class type = Class::ALL );

    private:

        cv::Mat frame;
        cv::Mat processed_frame;
};

}

#endif