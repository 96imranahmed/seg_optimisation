#ifndef HUNGARIAN_HPP
#define HUNGARIAN_HPP

#include "trackers.hpp"

namespace vivacity
{
    class HungarianTracker : public Tracker 
    {
        public:

            HungarianTracker( std::shared_ptr<Debug> &debugger,
                std::shared_ptr<ConfigManager> &config_manager,
                const std::string &name = "" );

            void updateFromConfig();

            virtual void track( const cv::Mat &frame, 
                const std::vector<cv::Rect> &detections );

        private:

            void hungarianMatch( const cv::Mat &frame,
                const std::vector<cv::Rect> &detections, 
                std::vector<Match> &matches );
            double getCostOfMatching( const cv::Mat &frame1, const cv::Mat &frame2,
                const cv::Rect &r1, const cv::Rect &r2 ) const;
            double getDetectionSimilarity( const cv::Mat &detection_subwindow1,
                const cv::Mat &detection_subwindow2 ) const;
    };
}

#endif