#include "vivacity/detectors.hpp"
#include <thread>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "opencv2/opencv.hpp"

//General constants
const int ch_max = 8;
const std::vector<std::string> ch_val = { "background",        "cyclists", "cars",        "pedestrians",
                                          "trucks/vans/buses", "buses",    "(6) ??? unknown", "(7) ??? unknown" };

//Contour processing constants
const int canny_thresh = 150;
const double contour_min_area_filter = 50.0;
const double max_contour_dist = 5.0;

//Contour self-correction constants
const int buffer_length = 300;
const int contour_avg_threshold = 8;
const double contour_avg_scalefactor = 1.5;

//Complex contour self-correction constants
const double min_contour_area_bgd = 200.0;
const double max_area_threshold = 0.9;
const double min_area_threshold = 0.1;
const double min_point_threshold = 0.2;
const double min_contour_area_fgd = 10.0;
const double area_confidence_threshold_min = 0.1;
const double area_confidence_threshold_max = 0.4;

//Threshold self-correction constants
const double threshold_min_area = 50;
const double threshold_min_val = 0.25;
const double threshold_mean_buffer = 0.025;
const int repeat_avoid_buffer = 10;

//Required Global Variables
cv::Mat image;
cv::Mat segnet_output;
std::vector<int> contour_count_buffer;
bool pause_check = false;
bool blob_detect = false;
bool contour_detect = false;
bool contour_overlay = false;
bool threshold_overlay = false;
bool activate_complex_detection = false;
int ch_des = 2;
int contour_avg_count = 0;
int cur_iteration = 0;
int prev_iteration_find = 0;

int kbhit(void) {
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

void key_check() {
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
				threshold_overlay = false;
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
				threshold_overlay = false;
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
				threshold_overlay = false;
				contour_count_buffer.clear();
			}
			contour_overlay = !contour_overlay;
	    } else if (cur_char == 't' || cur_char == 'T') {
			if (threshold_overlay) {
				std::cout << "de-activating threshold/histogram detection!" << std::endl;
			} else {
				std::cout << "activating threshold/histogram detection!" << std::endl;
				blob_detect = false;
				contour_detect = false;
				contour_overlay = false;
				contour_count_buffer.clear();
			}
			threshold_overlay = !threshold_overlay;
		}
	}
    }
}

std::tuple<std::vector<std::vector<cv::Point> >, std::vector<cv::Vec4i> > output_contours(cv::Mat &val_in) {
	cv::Mat val_out;
	cv::Canny(val_in, val_out, canny_thresh, canny_thresh * 2, 3);
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(val_out, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_TC89_KCOS, cv::Point(0, 0));
	for (int i = 0; i < contours.size(); i++) {
		std::vector<cv::Point> cur_contour;
		cv::approxPolyDP(contours[i], cur_contour, max_contour_dist, true);
		contours[i] = cur_contour;
	}
	//std::cout << "detected: " << contours.size() << " contours in background" << std::endl;
	return std::make_tuple(contours, hierarchy);
}

double find_mean(cv::Mat &mat_in){
	cv::Scalar mean_val = cv::mean(mat_in);
	auto ret_val = (mean_val.val[0])/255.0;
	return ret_val;
}

cv::Mat image_from_rect(std::vector<cv::Point> cur_contour, int channel = ch_des) {
	cv::Mat src =  segnet_output.rowRange(channel * (segnet_output.rows / ch_max), (channel + 1) * (segnet_output.rows / ch_max));
	cv::RotatedRect rect = cv::minAreaRect(cur_contour);
	cv::Mat m, rotated, cropped;
	float angle = rect.angle;
	cv::Size rect_size = rect.size;
	if (rect.angle < -45.0) {
		angle += 90.0;
		std::swap(rect_size.width, rect_size.height);
	}
	m = cv::getRotationMatrix2D(rect.center, angle, 1.0);
	cv::warpAffine(src, rotated, m, src.size(), cv::INTER_CUBIC);
	cv::getRectSubPix(rotated, rect_size, rect.center, cropped);
	return cropped;
}

cv::Ptr<cv::SimpleBlobDetector> generate_detector() {
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

void error_detect_simple(int val_in) {
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

std::vector<std::vector<cv::Point> > error_detect_complex(std::vector<std::vector<cv::Point> > fgnd, std::vector<std::vector<cv::Point> > bgnd, std::vector<cv::Vec4i> fgnd_hierarchy, std::vector<cv::Vec4i> bgnd_hierarchy) { // Approximate contours
	std::vector<std::vector<cv::Point> > op;;
    for (int i = 0; i < bgnd.size(); i++) {
    	if (bgnd_hierarchy[i][3] == -1) {//i.e. only use topmost contours
			std::vector<cv::Point> bgnd_contour = bgnd[i];
			if (cv::contourArea(bgnd_contour) > min_contour_area_bgd) {
			    for (int j = 0; j < fgnd.size(); j++) {
			    	std::vector<cv::Point> fgnd_contour = fgnd[j];
					bool inside = false;
					for (int l = 0; l < fgnd_contour.size(); l++) {
						if (!inside) {
							if (cv::pointPolygonTest(bgnd_contour, (fgnd[j])[l], false) > -1) {
								inside = true;
							}
						}
					}
					if (inside) {
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
								double area = double(cv::contourArea(fgnd_contour) / cv::contourArea(bgnd_contour)) ;
								if ( area < max_area_threshold && area > min_area_threshold) {
									cv::Mat im_crop = image_from_rect(bgnd_contour);
									double cur_mean = find_mean(im_crop);
									if (cur_mean > area_confidence_threshold_min && cur_mean < area_confidence_threshold_max) {
									op.push_back(bgnd_contour);
									/*
									cv::namedWindow( "OP-test" );
									cv::imshow( "OP-test", im_crop );
									*/
									if (!pause_check) { 
										std::cout << "erroneous classification in class: " << int(ch_des) << " (" << ch_val[ch_des] << ") detected ... pausing" << std::endl;
										std::cout << "Image printed with area " << cv::contourArea(bgnd_contour) << " and strength "<<mean(im_crop)<<std::endl;
									}
									pause_check = true;
									}			
								}
							}
						}
					}
			    }
			}
		}
	}
	return op;
}

std::vector<std::vector<int> > vector_verify (std::vector<double> val_in) {
	std::vector<double> val_non_zero;
	std::vector<int> index_non_zero;
	std::vector<std::vector<int> > index_similar;
	for (int i = 0; i < val_in.size(); i++) {
		if (!(val_in[i]==0)) { 
			index_non_zero.push_back(i); //Remember still offset by 1 to real class (ignores background)
			val_non_zero.push_back(val_in[i]);
		}
	}
	for (int i = 0; i < val_non_zero.size(); i++) {
		for (int j = 0; j < val_non_zero.size(); j++) {
			if (abs(val_non_zero[j] - val_non_zero[i]) < threshold_mean_buffer && i != j) {
				std::vector<int> opposite;
				opposite.push_back(index_non_zero[j]);
				opposite.push_back(index_non_zero[i]);
				if (std::find(index_similar.begin(), index_similar.end(), opposite) == index_similar.end()) {
					std::vector<int> insert;
					insert.push_back(index_non_zero[i]);
					insert.push_back(index_non_zero[j]);
					index_similar.push_back(insert);
				}
			}
		}
	}
	return index_similar;
}

std::vector<std::vector<cv::Point> > histogram_error_detect(std::vector<std::vector<cv::Point> > contours_in) {
	std::vector<std::vector<cv::Point> > error_contours;
	std::vector<std::vector<double> > op;
	for (int i = 0; i < contours_in.size(); i++) {
		std::vector<double> entry;
		if (cv::contourArea(contours_in[i]) > threshold_min_area) {
			for (int j = 1; j < ch_max; j++) {
				cv::Mat img = image_from_rect(contours_in[i],j);
				double entry_val = find_mean(img);
				if (entry_val > threshold_min_val) {
					entry.push_back(find_mean(img));
				} else {
					entry.push_back(0);
				}
			}
			std::vector<std::vector<int> > index_similar = vector_verify(entry);
			if (index_similar.size() > 0) {
				if ((cur_iteration - prev_iteration_find) > repeat_avoid_buffer) {
					prev_iteration_find = cur_iteration;
					error_contours.push_back(contours_in[i]);
					if (!pause_check) {
					std::cout<<"----------------------"<<std::endl;
					for (int i = 0 ; i < index_similar.size(); i++){
						std::cout<<"Error detected -> confusion between: " << ch_val[(index_similar[i])[0] + 1] << " and: " << ch_val[(index_similar[i])[1] + 1] << std::endl;
					}
					std::cout<<"----------------------"<<std::endl;
					}
					pause_check = true;
				}
			}
		}
		op.push_back(entry);
	}
	return error_contours;
	/* Print values
	std::cout<<"----------------------"<<std::endl;
	for (int i = 0; i < op.size(); i++) {
		std::cout<<"Area: " << cv::contourArea(contours_in[i]) << std::endl;
		for (int j = 0; j < op[i].size(); j++) {
			std::cout << op[i][j] << " ";
		}
		std::cout<<std::endl;
	}
	std::cout<<"----------------------"<<std::endl;
	*/
}

int main()
{
    std::thread first(key_check);
    std::shared_ptr<vivacity::Debug> debug(new vivacity::Debug());
    std::shared_ptr<vivacity::ConfigManager> cfg_mgr(new vivacity::ConfigManager(debug, "config.xml"));
    vivacity::Segnet segnet(debug, cfg_mgr, "cars_lorries_trucks_model");
    cv::Mat op_box;
	cur_iteration = 0;
	while (debug->procinput(image, pause_check)) {
	    if (pause_check == false) {
			cur_iteration++;
			segnet_output = segnet.label(image, ch_des);
	    }
	    // post processing
	    if (blob_detect) {
			cv::Mat val_out = segnet_output.rowRange(ch_des * (segnet_output.rows / ch_max), (ch_des + 1) * (segnet_output.rows / ch_max));
			std::vector<cv::KeyPoint> keypoints;
			cv::Ptr<cv::SimpleBlobDetector> detector = generate_detector();
			detector->detect(val_out, keypoints);
			cv::drawKeypoints(val_out, keypoints, op_box, cv::Scalar(0, 0, 255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
			debug->showFrame(op_box);
	    } else if (contour_detect) {
			cv::Mat val_in = segnet_output.rowRange(ch_des * (segnet_output.rows / ch_max), (ch_des + 1) * (segnet_output.rows / ch_max));
			std::tuple<std::vector<std::vector<cv::Point> >, std::vector<cv::Vec4i> >  raw_contour_out = output_contours(val_in);
			std::vector<std::vector<cv::Point> > contours_out = std::get<0>(raw_contour_out);
			std::vector<cv::Vec4i> cur_hierarchy = std::get<1>(raw_contour_out);
			cv::Mat out;
			cv::Mat in[] = { val_in, val_in, val_in };
			cv::merge(in, 3, out);
			int val_count = 0;
			for (size_t i = 0; i < contours_out.size(); i++) {
				if (cv::contourArea(contours_out[i]) > contour_min_area_filter && cur_hierarchy[i][3] == -1) {
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
	    } else if (contour_overlay) {
			cv::Mat val_bgd = segnet_output.rowRange(ch_des * (segnet_output.rows / ch_max), (ch_des + 1) * (segnet_output.rows / ch_max));
			if (contour_overlay) {
				cv::Mat val_in = segnet_output.rowRange(0, (segnet_output.rows / ch_max));
				std::tuple<std::vector<std::vector<cv::Point> >, std::vector<cv::Vec4i> >  raw_contour_out = output_contours(val_in);
				std::vector<std::vector<cv::Point> > contours_out =  std::get<0>(raw_contour_out);
				std::vector<cv::Vec4i> cur_hierarchy = std::get<1>(raw_contour_out);
				cv::Mat out;
				cv::Mat in[] = { val_bgd, val_bgd, val_bgd };
				cv::merge(in, 3, out);
				int val_count = 0;
				for (size_t i = 0; i < contours_out.size(); i++) {
					if (cv::contourArea(contours_out[i]) > contour_min_area_filter && cur_hierarchy[i][3] == -1) {
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
				if (activate_complex_detection) {
					// now process contours for background image
					std::tuple<std::vector<std::vector<cv::Point> >, std::vector<cv::Vec4i> >  raw_contour_out_fgnd = output_contours(val_in);
					std::vector<std::vector<cv::Point> > contours_out_fgnd = std::get<0>(raw_contour_out_fgnd);
					std::vector<cv::Vec4i> cur_hierarchy_fgnd = std::get<1>(raw_contour_out_fgnd);	
					std::vector<std::vector<cv::Point> > proc_out = error_detect_complex(contours_out_fgnd, contours_out, cur_hierarchy_fgnd, cur_hierarchy);
					for (size_t i = 0; i < proc_out.size(); i++) {
							cv::drawContours(out,
											 proc_out,
											 (int)i,
											 cv::Scalar(0, 0, 255),
											 1,
											 cv::LINE_8,
											 cv::noArray(),
											 0,
											 cv::Point(0, 0));
					}
				}
				debug->showFrame(out);
			} else {
				debug->showFrame(val_bgd);
			}
	    } else if (threshold_overlay) {
			cv::Mat val_bgd = segnet_output.rowRange(ch_des * (segnet_output.rows / ch_max), (ch_des + 1) * (segnet_output.rows / ch_max));
			cv::Mat val_in = segnet_output.rowRange(0, (segnet_output.rows / ch_max));
			std::tuple<std::vector<std::vector<cv::Point> >, std::vector<cv::Vec4i> >  raw_contour_out = output_contours(val_in);
			std::vector<std::vector<cv::Point> > contours_out =  std::get<0>(raw_contour_out);
			std::vector<cv::Vec4i> cur_hierarchy = std::get<1>(raw_contour_out);
			cv::Mat out;
			cv::Mat in[] = { val_bgd, val_bgd, val_bgd };
			cv::merge(in, 3, out);
			std::vector< std::vector<cv::Point> > valid_contours;
			int val_count = 0;
			for (size_t i = 0; i < contours_out.size(); i++) {
				if (cv::contourArea(contours_out[i]) > contour_min_area_filter && cur_hierarchy[i][3] == -1) {
					/* cv::drawContours(out,
								 contours_out,
								 (int)i,
								 cv::Scalar(0, 0, 255),
								 1,
								 cv::LINE_8,
								 cv::noArray(),
								 0,
								 cv::Point(0, 0)); */
					valid_contours.push_back(contours_out[i]);
				}
			}
			std::vector<std::vector<cv::Point> > contours_errors = histogram_error_detect(valid_contours);
			for (size_t i = 0; i < contours_errors.size(); i++) {
					cv::drawContours(out,
								 contours_errors,
								 (int)i,
								 cv::Scalar(0, 0, 255),
								 1,
								 cv::LINE_8,
								 cv::noArray(),
								 0,
								 cv::Point(0, 0));
					val_count++;
			}
			debug->showFrame(out);
		} else {
			debug->showFrame(segnet_output.rowRange(ch_des * (segnet_output.rows / ch_max), (ch_des + 1) * (segnet_output.rows / ch_max)));
		}
	}
    first.join();
}
