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

// contour self-correction specifics
const int canny_thresh = 200;
const int contour_thresh = 5.0;
std::vector<int> contour_count_buffer;
const int buffer_length = 300;
const int contour_avg_threshold = 15;
const double contour_avg_scalefactor = 2;
int contour_avg_count = 0;

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(stdin_fileno, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(icanon | echo);
    tcsetattr(stdin_fileno, tcsanow, &newt);
    oldf = fcntl(stdin_fileno, f_getfl, 0);
    fcntl(stdin_fileno, f_setfl, oldf | o_nonblock);
    ch = getchar();
    tcsetattr(stdin_fileno, tcsanow, &oldt);
    fcntl(stdin_fileno, f_setfl, oldf);
    if (ch != eof) {
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
	    } else if (cur_char == 'b') {
		if (blob_detect) {
		    std::cout << "de-activating blob detection!" << std::endl;
		} else {
		    std::cout << "activating blob detection!" << std::endl;
		    contour_detect = false;
		    contour_overlay = false;
		    contour_count_buffer.clear();
		}
		blob_detect = !blob_detect; // toggle blob detection
	    } else if (cur_char == 'c') {
		if (contour_detect) {
		    std::cout << "de-activating contour detection!" << std::endl;
		} else {
		    std::cout << "activating contour detection!" << std::endl;
		    blob_detect = false;
		    contour_overlay = false;
		    contour_count_buffer.clear();
		}
		contour_detect = !contour_detect;
	    } else if (cur_char == 'o') {
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

cv::ptr<cv::simpleblobdetector> generate_detector()
{
    cv::simpleblobdetector::params params;
    params.maxthreshold = 256;
    params.minthreshold = 40;
    params.filterbycolor = true;
    if (ch_des == 0) {
	params.blobcolor = 0;
    } else {
	params.blobcolor = 255;
    };
    params.filterbyconvexity = true;
    params.minconvexity = 0.3;
    params.maxconvexity = 1;
    return cv::simpleblobdetector::create(params);
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

void error_detect_complex(std::vector<std::vector<cv::point> > fgnd, std::vector<std::vector<cv::point> > bgnd)
{
}

int main()
{
    std::thread first(key_check);
    std::shared_ptr<vivacity::debug> debug(new vivacity::debug());
    std::shared_ptr<vivacity::configmanager> cfg_mgr(new vivacity::configmanager(debug, "config.xml"));
    vivacity::segnet segnet(debug, cfg_mgr, "cars_lorries_trucks_model");
    cv::mat image;
    cv::mat op;
    cv::mat op_box;
    while (1) {
		while (debug->procinput(image, pause_check)) {
			if (pause_check == false) {
			op = segnet.label(image, ch_des);
			}
			// post processing
			if (blob_detect == true && contour_detect == false) {
			cv::mat val_out = op.rowrange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
			std::vector<cv::keypoint> keypoints;
			cv::ptr<cv::simpleblobdetector> detector = generate_detector();
			detector->detect(val_out, keypoints);
			cv::drawkeypoints(
				val_out, keypoints, op_box, cv::scalar(0, 0, 255), cv::drawmatchesflags::draw_rich_keypoints);
			debug->showframe(op_box);
			} else if (contour_detect == true && blob_detect == false) {
			cv::mat val_out;
			cv::mat val_in = op.rowrange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
			cv::canny(val_in, val_out, canny_thresh, canny_thresh * 2, 3);
			std::vector<std::vector<cv::point> > contours_out;
			cv::findcontours(val_out, contours_out, cv::retr_external, cv::chain_approx_none, cv::point(0, 0));
			cv::mat out;
			cv::mat in[] = { val_in, val_in, val_in };
			cv::merge(in, 3, out);
			int val_count = 0;
			for (size_t i = 0; i < contours_out.size(); i++) {
				if (cv::contourarea(contours_out[i]) > contour_thresh) {
				cv::drawcontours(out,
								 contours_out,
								 (int)i,
								 cv::scalar(0, 255, 0),
								 1,
								 cv::line_8,
								 cv::noarray(),
								 0,
								 cv::point(0, 0));
				val_count++;
				}
			}
			error_detect_simple(val_count);
			debug->showframe(out);
			} else {
			cv::mat val_bgd = op.rowrange(ch_des * (op.rows / ch_max), (ch_des + 1) * (op.rows / ch_max));
			if (contour_overlay) {
				cv::mat val_out;
				cv::mat val_in = op.rowrange(0, (op.rows / ch_max));
				cv::canny(val_in, val_out, canny_thresh, canny_thresh * 2, 3);
				std::vector<std::vector<cv::point> > contours_out;
				cv::findcontours(val_out, contours_out, cv::retr_external, cv::chain_approx_none, cv::point(0, 0));
				std::cout << "detected: " << int(contours_out.size()) << " contours in background" << std::endl;
				cv::mat out;
				cv::mat in[] = { val_bgd, val_bgd, val_bgd };
				cv::merge(in, 3, out);
				int val_count = 0;
				for (size_t i = 0; i < contours_out.size(); i++) {
				if (cv::contourarea(contours_out[i]) > contour_thresh) {
					cv::drawcontours(out,
									 contours_out,
									 (int)i,
									 cv::scalar(255, 0, 0),
									 1,
									 cv::line_8,
									 cv::noarray(),
									 0,
									 cv::point(0, 0));
					val_count++;
				}
				}
				if (true) {
				// now process contours for background image
				cv::mat val_out;
				cv::canny(val_bgd, val_out, canny_thresh, canny_thresh * 2, 3);
				std::vector<std::vector<cv::point> > contours_out_fgnd;
				cv::findcontours(
					val_out, contours_out_fgnd, cv::retr_external, cv::chain_approx_none, cv::point(0, 0));
				error_detect_complex(contours_out_fgnd, contours_out);
				}
				debug->showframe(out);
			} else {
				debug->showframe(val_bgd);
			}
			}
		}
	}
	first.join();
}