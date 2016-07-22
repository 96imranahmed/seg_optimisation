#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "debugger.hpp"

#ifndef _WIN32
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif
#include "boost/filesystem.hpp"
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>


namespace vivacity
{

enum class TimeUnit { HOURS, MINUTES, SECONDS, MILLISECONDS };

class Timer
{
    public:
        Timer();

        long long int getLifetime( TimeUnit unit = TimeUnit::SECONDS ) const;
        int getTimeSinceReset( TimeUnit unit = TimeUnit::SECONDS ) const;
        bool checkElapsed( int time_limit, bool do_reset = true,
            TimeUnit unit = TimeUnit::SECONDS );
        double getAverageFPS();
        double getFPS( bool do_reset = true );
        void fixFPS( int target );
        void reset();

    private:
        long long int birth;
        long long int last_reset_time;
        long long int fps_birth;
        long long int fps_reset_time;
};

class Debug
{
    public:

        explicit Debug( const std::string &file_path = "",
            bool display_images = false, 
            bool logging = false, const std::string &logging_path = "./logs/",  
            bool record_video = false,
            const std::string &video_path = "./recordings/",
            bool raw_video_output = false, int verbosity = 2,
            bool display_fps = false );

        static void print( const std::string &message );

        operator bool() const;
        Debug& operator>>( cv::Mat &frame_container );
        void print( const std::string &message, int importance );
        void print( std::stringstream &stream, int importance );
        void showFrame();
        void jumpToFrame( int frame_number );
        void showFrame( cv::Mat alt_output );
        void clearFrame();
        void drawRectangles( std::vector<cv::Rect> rects );
        void drawPaths( std::vector<std::vector<cv::Point> > paths,
            int thickness = 1 );

        // Configuration updates
        void switchToCamera( int camera_id, int res_x, int res_y );
        void switchToVideo( const std::string &video_path );
        void switchToImageList( const std::string &dir_path );
        void startLogging( const std::string &video_path="./logs/" );
        void stopLogging();
        void startRecording( const std::string &video_path="./recordings/",
            int framerate = 10 );
        void startScreengrabbing( const std::string
            &screenshots_path="./screenshots/", int delay=30,
            TimeUnit=TimeUnit::SECONDS );
        void stopRecording();
        void setImageDisplay( bool setting );
        void saveImage();

        int getFrameCount() const
        {
            return frame_count;
        } 
        cv::Size getFrameSize() const
        {
            return frame_size;
        }
        double getFPS() const
        {
            return current_fps;
        }
        void setFPSDisplay( bool setting )
        {
            show_fps = setting;
        }
        void setVerbosity( int level )
        {
            verbosity_level = level;
        }
        void setTargetFPS( int target )
        {
            target_fps = target;
        }
        void setRotation( int orientation )
        {
            rotation = orientation;
        }
        void setCropEdges( bool crop )
        {
            crop_edges = crop;
        }
        void setScalingFactor( double scale )
        {
            scaling_factor = scale;
        }
        void setEqualize( bool use_equalization )
        {
            equalize = use_equalization;
        }
        
    private:
        void log( std::string );

        cv::Mat image;
        cv::Mat raw_image;
        cv::VideoCapture video;
        bool is_image;
        bool is_video;
        bool is_image_list;
        bool show_images;
        std::stringstream output_stream;
        bool write_log;
        std::string log_path;
        std::string log_file;
        bool screengrabbing;
        std::string screenshots_path;
        int screenshots_delay;
        Timer screenshot_timer;
        int frame_count;
        bool show_fps;
        Timer fps_timer;
        double current_fps;
        double target_fps;
        int verbosity_level;
        bool record_vid;
        int recording_framerate;
        std::string recording_path;
        std::string vid_file;
        bool unedited;
        cv::VideoWriter video_writer;
        cv::Size frame_size;
        int rotation;
        bool crop_edges;
        double scaling_factor;
        bool equalize;
        std::vector<std::string> image_list;
        std::vector<std::string>::iterator image_it;
};

// Mutex for console output
static std::mutex console_mutex;

// Utility functions
void cvVersion();
const std::string getCurrentDateTime();
char* stringToCharArray( const std::string &str );
std::string getImageType( cv::Mat image );
cv::Point getRectCenter( const cv::Rect &rect );
double getRectRatio( const cv::Rect &rect );
double euclidDist( const cv::Point &p1, const cv::Point &p2 );
cv::Mat getPerspectiveTransform( const cv::Mat &camera_points,
    const cv::Mat &projected_points );
cv::Point applyPerspectiveTransform( const cv::Point pt,
    const cv::Mat &transform );
std::vector<cv::Point> applyPerspectiveTransform( const
    std::vector<cv::Point> &pts, const cv::Mat &transform );
std::vector<std::pair<cv::Point, long long int> > applyPerspectiveTransform(
    const std::vector<std::pair<cv::Point, long long int> > &pts,
    const cv::Mat &transform );
const long long int getDetailedTimeStamp();

// Image pre-processing
cv::Mat cropToNarrow( const cv::Mat &image );
void equalizeHistogram( cv::Mat &image, double clipLimit = 3.0,
    cv::Size win_size = cv::Size( 8, 8 ) );
void adjustBrightness( cv::Mat &image, int b_change, int g_change,
    int r_change );
void adjustContrast( cv::Mat &image, int gain );
}

#endif
