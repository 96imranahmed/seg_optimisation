#include "vivacity/detectors.hpp"
#include <thread>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "opencv2/opencv.hpp"

const int ch_max = 8;
bool pause_check;
bool blob_detect;
int ch_des = 2;

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

void key_check()
{
    while (1) {
        if (kbhit()) {
            char cur_char = getchar();
            if (cur_char == 'p') {
                pause_check = !pause_check; //Toggle Pause
            } else if (cur_char > 47 && cur_char <58) {
                if ((cur_char - 48) < ch_max) {
                    //Swap to new class channel
                    ch_des = cur_char - 48;
                    std::cout<<int(ch_des)<<std::endl;
                }
            } else if (cur_char == 'b') {
                blob_detect = !blob_detect; //Toggle Blob detection
            }
        }
    }
}

cv::Ptr<cv::SimpleBlobDetector> generate_detector()
{
    cv::SimpleBlobDetector::Params params;
    params.maxThreshold = 256;
    params.minThreshold = 40;
    params.filterByColor = true;
    params.blobColor = 255;
    params.filterByConvexity = true;
    params.minConvexity = 0.3;
    params.maxConvexity = 1;
    cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
    return detector;
}

int main()
{
    pause_check = false;
    blob_detect = false;
    std::thread first (key_check);
    std::shared_ptr<vivacity::Debug> debug( new vivacity::Debug() );
    std::shared_ptr<vivacity::ConfigManager> cfg_mgr(new vivacity::ConfigManager( debug, "config.xml") );
    vivacity::Segnet segnet( debug, cfg_mgr, "cars_lorries_trucks_model");
    cv::Mat image;
    cv::Mat op;
    cv::Mat op_box;
    cv::Ptr<cv::SimpleBlobDetector> detector = generate_detector();
    while (1) {
        while ( debug->procinput(image, pause_check))
        {
            if (blob_detect) {
                if (pause_check == false) {
                    op = segnet.label(image, ch_des);
                }
                std::vector<cv::KeyPoint> keypoints;
                detector->detect(op, keypoints);
                cv::drawKeypoints(op, keypoints, op_box, cv::Scalar(0,0,255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                for (int i = 0; i < keypoints.count(); i++) {
                 
                }
                debug->showFrame(op_box);
            } else {
                if (pause_check == false) {
                    op = segnet.label(image, ch_des);
                }
                debug->showFrame(op); 
            }
        } 
    }
    first.join(); 
}
