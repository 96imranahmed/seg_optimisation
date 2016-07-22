#ifndef COLOR_HIST_HPP
#define COLOR_HIST_HPP

#include "trackers.hpp"

namespace vivacity
{

class ColorHistogramTracker : public Tracker
{
    public:

        ColorHistogramTracker( std::shared_ptr<Debug> &debug,
            std::shared_ptr<ConfigManager> &config_manager,
            const std::string &name = "" );

        virtual void track( const cv::Mat &frame,
            const std::vector<cv::Rect> &detections );

        void track( const cv::Mat &frame,
            const std::vector<cv::Rect> &detections, const cv::Mat
            &mask );

        void initialise( const cv::Mat &first_frame, const
            std::vector<cv::Rect> &first_detections, const cv::Mat
            &first_mask = cv::Mat() );

    private:

        void updateFromConfig();

        void matchObjectsToDetections( const std::vector<cv::Rect>
            &detections, std::unordered_map<int, std::list<std::pair<
            int, double> > > &match_candidates, const cv::Mat &frame );

        void selectMatchCandidates( std::unordered_map<int,
            std::list<std::pair<int, double> > > &match_candidates,
            const cv::Mat &frame, const std::vector<cv::Rect>
            &detections, const cv::Mat &mask = cv::Mat() );

        double getColorHistogramDifference( cv::Rect object1, cv::Rect object2,
            const cv::Mat &old_frame, const cv::Mat &frame, const 
            cv::Mat &old_mask = cv::Mat(), const cv::Mat &mask = 
            cv::Mat() );

    public:
        cv::Mat computeColorHistogram( const cv::Mat &image, const
            cv::Mat &mask = cv::Mat() );

        double color_hist_diff_thresh;
        double max_dist_thresh;
        boost::circular_buffer<cv::Mat> past_masks;
        bool use_mask;
};

}

#endif