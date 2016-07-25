#include "vivacity/detectors.hpp"
#include <thread>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "opencv2/opencv.hpp"

const int ch_max = 8;
bool pause_check = false;
bool blob_detect = false;
bool contour_detect = false;
bool contour_overlay = false;
int ch_des = 2;
const std::string ch_val[] = {"Background", "Cyclists", "Cars", "Pedestrians", "Trucks/Vans/Buses", "Buses", "??? UNKNOWN", "??? UNKNOWN" };
const int canny_thresh = 200;
const int contour_thresh = 30.0;

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
            if (cur_char == 32) {
				if (!pause_check) {
					std::cout<<"Pausing..."<<std::endl;
				} else {
					std::cout<<"Continuing..."<<std::endl;
				}
                pause_check = !pause_check; //Toggle Pause
            } else if (cur_char > 47 && cur_char <58) {
                if ((cur_char - 48) < ch_max) {
                    //Swap to new class channel
                    ch_des = cur_char - 48;
                    std::cout<<"Switching to channel: " << int(ch_des)<< " (" << ch_val[ch_des] << ")" << std::endl;
                }
            } else if (cur_char == 'b') {
				if (blob_detect) {
					std::cout<<"De-activating blob detection!"<<std::endl;
				} else {
					std::cout<<"Activating blob detection!"<<std::endl;
					contour_detect = false;
					contour_overlay = false;
				}
                blob_detect = !blob_detect; //Toggle Blob detection
			} else if (cur_char == 'c') {
				if (contour_detect) {
					std::cout<<"De-activating contour detection!"<<std::endl;
				} else {
					std::cout<<"Activating contour detection!"<<std::endl;
					blob_detect = false;
					contour_overlay = false;
				}
				contour_detect = !contour_detect;
			} else if (cur_char == 'o') {
				if (contour_overlay) {
					std::cout<<"De-activating overlay contour detection!"<<std::endl;
				} else {
					std::cout<<"Activating overlay contour detection!"<<std::endl;
					blob_detect = false;
					contour_detect = false;
				}
				contour_overlay = !contour_overlay;
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
	if (ch_des == 0) {params.blobColor = 0; } else {params.blobColor = 255;};
    params.filterByConvexity = true;
    params.minConvexity = 0.3;
    params.maxConvexity = 1;
    return cv::SimpleBlobDetector::create(params);
}

int main()
{
    std::thread first (key_check);
    std::shared_ptr<vivacity::Debug> debug( new vivacity::Debug() );
    std::shared_ptr<vivacity::ConfigManager> cfg_mgr(new vivacity::ConfigManager( debug, "config.xml") );
    vivacity::Segnet segnet( debug, cfg_mgr, "cars_lorries_trucks_model");
    cv::Mat image;
    cv::Mat op;
    cv::Mat op_box;
    while (1) {
        while ( debug->procinput(image, pause_check))
        {
			if (pause_check == false) {
				op = segnet.label(image, ch_des);
			}
			//Post Processing
            if (blob_detect == true && contour_detect == false) {
				cv::Mat val_out = op.rowRange(ch_des*(op.rows/ch_max), (ch_des + 1)*(op.rows/ch_max));
                std::vector<cv::KeyPoint> keypoints;
				cv::Ptr<cv::SimpleBlobDetector> detector = generate_detector();
                detector->detect(val_out, keypoints);
                cv::drawKeypoints(val_out, keypoints, op_box, cv::Scalar(0,0,255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                debug->showFrame(op_box);
            } else if (contour_detect == true && blob_detect == false) {
				cv::Mat val_out;
				cv::Mat val_in = op.rowRange(ch_des*(op.rows/ch_max), (ch_des + 1)*(op.rows/ch_max));
				cv::Canny( val_in, val_out, canny_thresh, canny_thresh*2, 3 );
				std::vector<std::vector<cv::Point> > contours_out;
				cv::findContours(val_out,contours_out, cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
				std::cout<<"Detected: " << int(contours_out.size()) << " contours" <<std::endl;
				cv::Mat out;
				cv::Mat in[] =  {val_in, val_in, val_in};
				cv::merge(in, 3, out);
				for( size_t i = 0; i< contours_out.size(); i++ )
				{
					if (cv::contourArea(contours_out[i]) > contour_thresh) {
						cv::drawContours(out, contours_out, (int)i,  cv::Scalar(0,255,0), 1, cv::LINE_8, cv::noArray(),0,cv::Point(0,0));
					}
				}
				debug->showFrame(out);
			}
			else {
				cv::Mat val_bgd = op.rowRange(ch_des*(op.rows/ch_max), (ch_des + 1)*(op.rows/ch_max));
				if (contour_overlay) {
					cv::Mat val_out;
					cv::Mat val_in = op.rowRange(0, (op.rows/ch_max));
					cv::Canny( val_in, val_out, canny_thresh, canny_thresh*2, 3 );
					std::vector<std::vector<cv::Point> > contours_out;
					cv::findContours(val_out,contours_out, cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
					std::cout<<"Detected: " << int(contours_out.size()) << " contours in background" <<std::endl;
					cv::Mat out;
					cv::Mat in[] =  {val_bgd, val_bgd, val_bgd};
					cv::merge(in, 3, out);
					for( size_t i = 0; i< contours_out.size(); i++ )
					{
						if (cv::contourArea(contours_out[i]) > contour_thresh) {
							cv::drawContours(out, contours_out, (int)i,  cv::Scalar(255,0,0), 1, cv::LINE_8, cv::noArray(),0,cv::Point(0,0));
						}
					}
					debug->showFrame(out);
				} else {
                debug->showFrame(val_bgd); 
				}
            }
        } 
    }
    first.join(); 
}
