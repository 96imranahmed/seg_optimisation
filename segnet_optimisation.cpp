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
const std::vector<std::string> ch_val = { "background",        "cyclists", "cars",        "pedestrians",
                                          "trucks/vans/buses", "buses",    "??? unknown", "??? unknown" };

// Contour self-correction vars and constants
const int canny_thresh = 200;
const double contour_thresh = 5.0;
std::vector<int> contour_count_buffer;
const int buffer_length = 300;
const int contour_avg_threshold = 10;
const double contour_avg_scalefactor = 2;
int contour_avg_count = 0;
// Complex contour self-correction vars and constants

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
    if (ch != EOF) {
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
		    std::cout << "pausing..." << std::endl;
		} else {
		    std::cout << "continuing..." << std::endl;
		}
		pause_check = !pause_check; // toggle pause
	    } else if (cur_char > 47 && cur_char < 58) {
		if ((cur_char - 48) < ch_max) {
		    // swap to new class channel
		    ch_des = cur_char - 48;
		    std::cout << "switching to channel: " << int(ch_des) << " (" << ch_val[ch_des] << ")" << std::endl;
		}
	    } else if (cur_char == 'b' || cur_char == 'B') {
		if (blob_detect) {
		    std::cout << "de-activating blob detection!" << std::endl;
		} else {
		    std::cout << "activating blob detection!" << std::endl;
		    contour_detect = false;
		    contour_overlay = false;
		    contour_count_buffer.clear();
		}
		blob_detect = !blob_detect; // toggle blob detection
	    } else if (cur_char == 'c'|| cur_char == 'C') {
		if (contour_detect) {
		    std::cout << "de-activating contour detection!" << std::endl;
		} else {
		    std::cout << "activating contour detection!" << std::endl;
		    blob_detect = false;
		    contour_overlay = false;
		    contour_count_buffer.clear();
		}
		contour_detect = !contour_detect;
	    } else if (cur_char == 'o' || cur_char == 'O') {
		if (contour_overlay) {
		    std::cout << "de-activating overlay contour detection!" << std::endl;
		} else {
		    std::cout << "activating overlay contour detection!" << std::endl;
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
    if (ch_des == 0) {
	params.blobColor = 0;
    } else {
	params.blobColor = 255;
    };
    params.filterByConvexity = true;
    params.minConvexity = 0.3;
    params.maxConvexity = 1;
    return cv::SimpleBlobDetector::create(params);
}

void error_detect_simple(int val_in)
{
    if (!pause_check) {
	if (contour_count_buffer.size() > buffer_length) {
	    contour_count_buffer.erase(contour_count_buffer.begin());
	    contour_count_buffer.push_back(val_in);
	} else {
	    contour_count_buffer.push_back(val_in);
	}
	/*
	for (int i = 0; i < contour_count_buffer.size(); i++) {
	                for (int j = 0; j < contour_count_buffer[i]; j++) {
	                                std::cout << "*";
	                }
	                std::cout << std::endl;
	}
	std::cout << "----------------------------------------------" << std::endl;
	*/
	if (contour_count_buffer.size() > 2) {
	    double m = std::accumulate(std::begin(contour_count_buffer), (std::end(contour_count_buffer) - 1), 0.0) /
	               (contour_count_buffer.size() - 1);
	    if ((double)contour_count_buffer.back() > (m * contour_avg_scalefactor)) {
		contour_avg_count++;
	    } else if (contour_avg_count < 2) {
		contour_avg_count = 0;
	    } else {
		contour_avg_count--;
	    }
	    // std::cout<<contour_avg_count<<std::endl;
	}
	if (contour_avg_count > contour_avg_threshold) {
	    contour_count_buffer.clear();
	    contour_avg_count = 0;
	    pause_check = true;
	    std::cout << "erroneous classification in class: " << int(ch_des) << " (" << ch_val[ch_des]
	              << ") detected ... pausing" << std::endl;
	}
    }
}

void error_detect_complex(std::vector<std::vector<cv::Point> > fgnd, std::vector<std::vector<cv::Point> > bgnd)
{ // Approximate contours
	if (!pause_check) {
    const double max_contour_dist = 5.0; // 1 is original, 0 is most coarse approxmation
    const double min_contour_area_bgd = 30.0;
    const double max_area_threshold = 0.2;
    const double min_point_threshold = 0.95;
	const double min_contour_area_fgd = 10.0;
    for (int i = 0; i < bgnd.size(); i++) {
	std::vector<cv::Point> bgnd_contour;
	cv::approxPolyDP(bgnd[i], bgnd_contour, max_contour_dist, true);
	if (cv::contourArea(bgnd_contour) > min_contour_area_bgd) {
	    for (int j = 0; j < fgnd.size(); j++) {
			if (cv::pointPolygonTest(bgnd_contour, (fgnd[j])[0], false) > -1) {
				std::vector<cv::Point> fgnd_contour;
				cv::approxPolyDP(fgnd[j], fgnd_contour, max_contour_dist, true);
				if (cv::contourArea(fgnd_contour) > min_contour_area_fgd) {
					// Check if contour fully within
					int points_within = 0;
					for (int k = 0; k < fgnd_contour.size(); k++) {
						if (cv::pointPolygonTest(bgnd_contour, fgnd_contour[k], false) > -1) {
						points_within++;
						}
					}
					if (double(points_within / fgnd_contour.size()) > min_point_threshold) {
						// Now check area
						if (double(cv::contourArea(fgnd_contour) / cv::contourArea(bgnd_contour)) <
							max_area_threshold) {
							pause_check = true;
							std::cout << "erroneous classification in class: " << int(ch_des) << " ("
									  << ch_val[ch_des] << ") detected ... pausing" << std::endl;
						}
					}
				}
			}
	    }
	}
    }
	}
}

int main()
{
    if (false) {
    cv::namedWindow( "test" );
	cv::namedWindow( "test2" );
    cv::Mat mat( cv::imread("im.png", CV_LOAD_IMAGE_GRAYSCALE ) );
	cv::Mat mask( ( mat > 0) / 255 );
    cv::resize( mat, mat, cv::Size( 256, 256 ) );
	cv::resize( mask, mask, cv::Size( 256, 256 ) );
    cv::imshow( "test", mat );
	cv::imshow( "test2", mask );
    cv::waitKey(0);
	} else {
    std::thread first(key_check);
    std::shared_ptr<vivacity::Debug> debug(new vivacity::Debug());
    std::shared_ptr<vivacity::ConfigManager> cfg_mgr(new vivacity::ConfigManager(debug, "config.xml"));
    vivacity::Segnet segnet(debug, cfg_mgr, "cars_lorries_trucks_model");
    cv::Mat image;
    cv::Mat op;
    cv::Mat op_box;
    while (1) {
	while (debug->procinput(image, pause_check)) {
	    if (pause_check == false) {
		op = segnet.label(image, ch_des);
	    }
	    // post processing
	    if (blob_detect == true && contour_detect == false) {
		cv::Mat val_out = op.rowRange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
		std::vector<cv::KeyPoint> keypoints;
		cv::Ptr<cv::SimpleBlobDetector> detector = generate_detector();
		detector->detect(val_out, keypoints);
		cv::drawKeypoints(
		    val_out, keypoints, op_box, cv::Scalar(0, 0, 255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		debug->showFrame(op_box);
	    } else if (contour_detect == true && blob_detect == false) {
		cv::Mat val_out;
		cv::Mat val_in = op.rowRange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
		cv::Canny(val_in, val_out, canny_thresh, canny_thresh * 2, 3);
		std::vector<std::vector<cv::Point> > contours_out;
		cv::findContours(val_out, contours_out, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
		cv::Mat out;
		cv::Mat in[] = { val_in, val_in, val_in };
		cv::merge(in, 3, out);
		int val_count = 0;
		for (size_t i = 0; i < contours_out.size(); i++) {
		    if (cv::contourArea(contours_out[i]) > contour_thresh) {
			cv::drawContours(out,
			                 contours_out,
			                 (int)i,
			                 cv::Scalar(0, 255, 0),
			                 1,
			                 cv::LINE_8,
			                 cv::noArray(),
			                 0,
			                 cv::Point(0, 0));
			val_count++;
		    }
		}
		error_detect_simple(val_count);
		debug->showFrame(out);
	    } else {
		cv::Mat val_bgd = op.rowRange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
		if (contour_overlay) {
		    cv::Mat val_out;
		    cv::Mat val_in = op.rowRange(0, (op.rows / ch_max));
		    cv::Canny(val_in, val_out, canny_thresh, canny_thresh * 2, 3);
		    std::vector<std::vector<cv::Point> > contours_out;
		    cv::findContours(val_out, contours_out, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
		    //std::cout << "detected: " << contours_out.size() << " contours in background" << std::endl;
		    cv::Mat out;
		    cv::Mat in[] = { val_bgd, val_bgd, val_bgd };
		    cv::merge(in, 3, out);
		    int val_count = 0;
		    for (size_t i = 0; i < contours_out.size(); i++) {
			if (cv::contourArea(contours_out[i]) > contour_thresh) {
			    cv::drawContours(out,
			                     contours_out,
			                     (int)i,
			                     cv::Scalar(255, 0, 0),
			                     1,
			                     cv::LINE_8,
			                     cv::noArray(),
			                     0,
			                     cv::Point(0, 0));
			    val_count++;
			}
		    }
		    if (true) {
			// now process contours for background image
			cv::Mat val_out;
			cv::Canny(val_bgd, val_out, canny_thresh, canny_thresh * 2, 3);
			std::vector<std::vector<cv::Point> > contours_out_fgnd;
			cv::findContours(val_out, contours_out_fgnd, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
			error_detect_complex(contours_out_fgnd, contours_out);
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
}