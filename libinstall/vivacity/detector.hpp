#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include "debugger.hpp"
#include "io.hpp"

namespace vivacity
{

class Detector
{
    public:
        virtual std::string getName()
        {
            return model_name;
        }
        virtual std::vector<cv::Rect> detect( const cv::Mat &image ) = 0;

    protected:
        //virtual void updateFromConfig() = 0;
        virtual ~Detector() {};

        std::shared_ptr<Debug> debug;
        std::shared_ptr<ConfigManager> cfg_manager;
        std::string model_name;
};

}

#endif
