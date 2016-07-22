#ifndef SVM_HOG_HPP
#define SVM_HOG_HPP

#include "detectors.hpp"

#   ifdef WITH_CUDA
#   include "opencv2/cudaobjdetect.hpp"
#   endif

#include "opencv2/objdetect.hpp"
#include "opencv2/ml.hpp"

namespace vivacity
{

class SVM_HOG : public Detector
{
    public:
        SVM_HOG( const std::shared_ptr<Debug> &debug_ptr,
            const std::shared_ptr<ConfigManager> &cfg_ptr, bool gpu,
            const std::string &name = "" );

        void train();

        std::vector<cv::Rect> detect( const cv::Mat &image );

        bool isTrained();

    private:
        void getSamples( const std::string &dir_name,
            std::vector<std::string> &files );

        void generateFeatures( std::vector<std::string> &samples,
            std::vector<cv::Mat> &features );

        void convertDescriptorsToTrainingData(
            const std::vector<cv::Mat> &descriptors,
            cv::Mat &training_data );

        void getDescriptorsFromSVM( const cv::Ptr<cv::ml::SVM> &svm,
            std::vector<float> &descriptor_vector );

        void saveSVMModel( std::vector<float> &descriptor_vector,
            std::vector<unsigned int> &vector_indices,
            const std::string &file_name );

        void loadSVMModel();
        void updateFromConfig();
        void initialiseCPUHOG();
        void initialiseGPUHOG();
        void initialiseSVM();

        std::string svm_data_path;
        std::string svm_model_file;
        std::string positive_dir;
        std::string negative_dir;
        bool use_gpu;
        bool cpu_initialised;
        bool gpu_initialised;
        bool model_loaded;
        std::string model_name;
        cv::Ptr<cv::ml::SVM> svm;
        cv::Ptr<cv::HOGDescriptor> cpu_hog;
#   ifdef WITH_CUDA
        cv::Ptr<cv::cuda::HOG> gpu_hog;
#   endif
        cv::Size win_size;
        cv::Size win_stride;
        cv::Size block_size;
        cv::Size block_stride;
        cv::Size cell_size;
        cv::Size padding;
        int nlevels;
        int nbins;
        int group_threshold;
        bool trained;
        double hit_threshold;
        double scaling_factor;

        // SVM paramters
        int svm_type;
        int kernel_type;
        int max_iterations;
        double iteration_epsilon;
        double coef0;
        double degree;
        double gamma;
        double nu;
        double c;
        double p;
};

}

#endif
