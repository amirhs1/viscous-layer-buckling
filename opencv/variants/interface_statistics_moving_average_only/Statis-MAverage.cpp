#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#include <boost/filesystem.hpp>
//OpenCV Libraries:
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
//GSL Libraries:
#include <gsl/gsl_math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_errno.h>//GSL error handling functions header
#include <gsl/gsl_vector.h>

using namespace std;
using namespace cv;

template <typename T>
T min (T a, T b) {return a < b ? a : b;}

template <typename T>
T max (T a, T b) {return a > b ? a : b;}

double FrameMeanCurvature(double *, int, double *, double *,double *);
double MovingAverage(double *, int, int);

int main (int argc, char* argv[]){
    if (argc == 1) return 1;
    string experiment_folder = argv[1];
    cout << "Enter Experiment Folder: " << endl << experiment_folder<< endl;
    string experiment_name = argv[2];
    cout << "Enter Experiment Name: " << endl << experiment_name << endl;
    
    string input_address = experiment_folder;
    input_address.append("/");
    input_address.append(experiment_name);
    input_address.append("-4-Thershold/");
    input_address.append(experiment_name);
    input_address.append("-Thershold-%04d.tif");
    cout << "Enter Input Images Path: " << endl << input_address << endl;
    
    string output_address = experiment_folder;
    output_address.append("/");
    output_address.append(experiment_name);
    output_address.append("-5-MinContour/");
    output_address.append(experiment_name);
    output_address.append("-MinCountor-%04d.tif");
    cout << "Enter Output Images Path: " << endl << output_address << endl;
    
    const char * path = output_address.c_str();//Path for writing output frames and data:
    char filename[500];
    
   
    
    string original_address = experiment_folder;
    original_address.append("/");
    original_address.append(experiment_name);
    original_address.append("-3-Resized/");
    original_address.append(experiment_name);
    original_address.append("-Resized-%04d.tif");
    cout << "Enter Original Images Path: " << endl << original_address << endl;
    

    VideoCapture cap {input_address};
    if (!cap.isOpened())
    {
        cerr << "Failed to open Image Sequence!\n" << endl;
        return 1;
    }
    Mat frame;
    cap >> frame;
    
    // Loading original images:
    VideoCapture cap_o {original_address};
    if (!cap_o.isOpened())
    {
        cerr << "Failed to open Image Sequence!\n" << endl;
        return 1;
    }
    Mat original;
    cap_o >> original;
    
    //Create txt files for writing data:
    string input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-FrameStatistics.txt");
    ofstream write_statis (input_dummy);
    write_statis << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number\t" << "(3):Frame Amplitude (mm)\t" << "(4):Frame Mean Curvature(1/mm)"  << endl;
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SmoothedData.txt");
    ofstream write_smooth_curv (input_dummy);
    write_smooth_curv << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number\t"  << "(3):Frame Smouthed Amplitude (mm)\t" << "(4):Frame Smouthed Curvature (1/mm)" << endl;
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-TransitionPoint.txt");
    ofstream write_transition (input_dummy);
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-Second_Derivative_of_Curvature.txt");
    ofstream write_sec_deriv (input_dummy);
    write_sec_deriv << "# Numbers of second derivative points are total frame - 2." << endl;
    write_sec_deriv << "#(1):Frame number\t" << "(2):Normalized frame number\t" << "(3):Second Derivative of Mean Curvature(1/mm^3)" << endl;

    // Inputs:
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    cout << "Total frames:" << total_frames << endl;
    double mm_pixel;
    cout << "Inter mm-to-pixel ratio:" << endl;
    cin >> mm_pixel;
    
    //Variables:
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    int frameindex = 0;
    int stride = 1;
    double *frame_amplitude = new double[total_frames];
    double *frame_curvature = new double[total_frames];
    
    // Main Part:
    while (!frame.empty()) {
        ++frameindex;
        //Find Contour with maximum area in each frame:
        findContours(frame, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//OpenCV functions destroy input frame.
        int max_idx = 0, max_size = 0;
        for(int idx = 0; idx < contours.size(); ++idx ){
            if (contours[idx].size() > max_size) {
                max_size = contours[idx].size();
                max_idx = idx;
            }
        }
        
        //Find lowest boundary of maximum-area contour:
        vector<int> curve (frame.cols, -1);
        vector<int> multiplicity (frame.cols, 0);
        for (auto point : contours[max_idx])    {
            ++multiplicity[point.x];
            if ( curve[point.x]< point.y) curve[point.x] = point.y;
        }
        vector<vector<Point>> curves (1);
        for (auto i = 0; i < curve.size();++i)
            if (multiplicity[i] != 0) curves[0].push_back(Point(i, curve[i]));//for writing curves on original frames.
        
        //Drawing lowest boundary of maximum-area contour in choosen frame:
        polylines (original, curves, false, Scalar::all(255));//
        imshow (experiment_name +" "+to_string(frameindex)+"/"+to_string(total_frames), original);
        waitKey (150);  //Wait time in miliseconds
        
        //Calculating curvature and variance:
        int data_size = curves[0].size();
        double *z_data = new double[data_size];
        for (auto k = 0; k < data_size; ++k) z_data[k] = curves[0][k].y;
        
        double y_min, y_max;
        gsl_stats_minmax (&y_min, &y_max, z_data,stride,data_size);
        frame_amplitude[frameindex-1] = abs(y_max - y_min);
        
        double *second_derivative = new double[data_size-2];// For the first and the last points, Could not define second derivative.
        double *first_derivative = new double[data_size-2];// For the first and the last points, Could not define first derivative.
        double *local_curvature = new double[data_size-2];
        frame_curvature[frameindex-1] =  FrameMeanCurvature(z_data,data_size,second_derivative,first_derivative,local_curvature);
        
        write_statis << frameindex << "\t" << (float)frameindex/total_frames << "\t" << frame_amplitude[frameindex-1] * mm_pixel << "\t" << frame_curvature[frameindex-1] / mm_pixel << endl;
        
        //Writing images:
        sprintf (filename, path, frameindex);
        imwrite (filename, original);
        delete []z_data;
        
        cap >> frame;
        cap_o >> original;
    }
    // Smoothing by means of moving average method:
    
    int lower,upper;
    double  *smouth_curvature = new double[total_frames];//Smoothed Curvature
    double *smooth_amplitude = new double[total_frames];//Smoothed Amplitude
    double dummy;
    
    for (int j = 0; j < total_frames; ++j){
        dummy = MovingAverage(frame_curvature,total_frames,j);
        smouth_curvature[j] = dummy;
    }
    
    for (int j = 0; j < total_frames; ++j){
        dummy = MovingAverage(frame_amplitude,total_frames,j);
        smooth_amplitude[j] = dummy;
    }
    
        //Writing smoouthed curavure and amplitude in txt file:
    for(int i = 0; i < total_frames; ++i)
        write_smooth_curv << i+1 << "\t" <<  (double) i / total_frames << "\t" << smooth_amplitude[i] * mm_pixel << "\t" << smouth_curvature[i] / mm_pixel<< endl;
    
    //Finding transition point:
    double * curv_sec_deri = new double[total_frames-2];
    for (int j = 1; j < total_frames-1; ++j){
        dummy = smouth_curvature[j-1] - 2 * smouth_curvature[j] + smouth_curvature[j+1];
        curv_sec_deri[j-1] = dummy;
        write_sec_deriv << j << "\t" << (double) j/(total_frames-1) << "\t" << curv_sec_deri[j-1] / pow(mm_pixel,3.0) << endl;
    }
    
    size_t max_index;
    max_index = gsl_stats_max_index (curv_sec_deri, 1,total_frames-2);
    cout << "Index of Maximum value of second derivative (Based on array index rule):(transition frame number in curvature data = this number + 1)\t" << max_index << endl;
    write_transition << "(1):Index of Maximum value of second derivative (Based on array index rule)\t" << "(2):Coresponding frame number" << endl;
    write_transition << max_index << "\t" << max_index+1;//
    
    delete []frame_amplitude;
    delete []frame_curvature;
    delete []smouth_curvature;
    delete []smooth_amplitude;
    delete []curv_sec_deri;
    
    write_statis.close();
    write_smooth_curv.close();
    write_sec_deriv.close();
    write_transition.close();
    
    return 0;
}

//****************************************************
//Calculating Mean curvature of each frame: locaal_curvature = <|second_derivative|>/sqrt[{(first_derivative)^2+1}^3]
double FrameMeanCurvature(double *z_data,int data_size,double * second_derivative, double * first_derivative, double * local_curvature){//Simple definition of numerical second derivative.
    double dummy;
    double curvature;
    for (int j = 1; j < data_size-1; ++j){
        dummy = z_data[j-1] - 2 * z_data[j] + z_data[j+1];// mm_pixel for z_data removes one of  mm_pixel^2 in the definition of second derivative.
        second_derivative[j-1] = abs(dummy);
        dummy = (z_data[j-1] - z_data[j+1]) / 2.0;//mm_pixel of z_data removes mm_pixel in th definition of first derivative.
        first_derivative[j-1] = abs(dummy);
        dummy = sqrt(pow(((first_derivative[j-1]*first_derivative[j-1])+1),3.0));//1/((((dz/dx)^2+1)^3)^{1/2})
        local_curvature[j-1] = second_derivative[j-1] / dummy;
    }
    
    curvature = gsl_stats_mean(local_curvature,1,(data_size-2));
    return curvature;
    delete []second_derivative;
    delete []first_derivative;
    delete []local_curvature;
}

// Moving average method over 10 Neighbours for smoothing curvature data:
double MovingAverage(double *data, int size, int index){
    int i;
    double sum = 0;
    auto lower = ::max (0, index-5), upper = ::min (size, index+6);//::max is something like cmath::sqrt but nothing before :: means that this is a user-defined function or anything else that is  used.
    for (i = lower; i < upper; i++) sum += data[i];
    sum /= (upper-lower);
    return sum;
}


