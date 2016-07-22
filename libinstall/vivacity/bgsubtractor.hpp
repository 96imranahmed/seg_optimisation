#ifndef BGSUBTRACTOR_HPP
#define BGSUBTRACTOR_HPP

#include "detectors.hpp"

#include "opencv2/video/background_segm.hpp"
#ifdef WITH_CUDA
    #include "opencv2/cudabgsegm.hpp"
#endif

namespace vivacity
{

class BGSubtractor : public Detector
{
    public:
        BGSubtractor( const std::shared_ptr<Debug> &debug_ptr,
            const std::shared_ptr<ConfigManager> &cfg_ptr, bool gpu,
            const std::string &name = "" );

        cv::Mat getForeground();
        cv::Mat getForeground( cv::Mat image );
        virtual std::vector<cv::Rect> detect( const cv::Mat &image );
        std::vector<cv::Rect> detect( const cv::Mat &image, cv::Mat &mask );
        std::vector<std::vector<cv::Point> > detectContours( cv::Mat image,
            cv::Mat &output_mask );

    private:
        void updateFromConfig();
        void initialiseCPUModel();
        void initialiseGPUModel();

        bool use_gpu;
        cv::Ptr<cv::BackgroundSubtractor> cpu_subtractor;
#ifdef WITH_CUDA
        cv::Ptr<cv::cuda::BackgroundSubtractorMOG2> gpu_subtractor;
#endif
        cv::Mat mask;
        int history;
        double threshold;
        bool remove_shadows;
        double min_perim_length;
        double max_perim_length;
        double erosion_level;
        double dilation_level;
        bool cpu_initialised;
        bool gpu_initialised;
};

}

#endif
