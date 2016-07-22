#ifndef HAARCASCADE_HPP
#define HAARCASCADE_HPP

/*
 * Revisit this detector when required and migrate to OpenCV 3

#include "detectors.hpp"

#include "opencv2/video/background_segm.hpp"
#include "opencv2/gpu/gpu.hpp"

namespace vivacity
{
    class BGSubtractor : public Detector
    {
        public:
            BGSubtractor( const std::shared_ptr<Debug> &debug_ptr,
                const std::shared_ptr<ConfigManager> &cfg_ptr, bool gpu,
                const std::string &name = "" );

            cv::Mat getForeground();
            cv::Mat getForeground( cv::Mat image);
            std::vector<cv::Rect> detect( cv::Mat image);
            std::vector<std::vector<cv::Point> > detectContours( cv::Mat image, cv::Mat &output_mask );

        private:
            void updateFromConfig();
            void initialiseCPUModel();
            void initialiseGPUModel();

            bool use_gpu;
            cv::BackgroundSubtractorMOG2 cpu_subtractor;
            cv::gpu::MOG2_GPU gpu_subtractor;
            cv::Mat mask;
            int history;
            double threshold;
            bool detect_shadows;
            double min_perim_length;
            double max_perim_length;
            double erosion_level;
            double dilation_level;
            bool cpu_initialised;
            bool gpu_initialised;
    };
}

 */

#endif