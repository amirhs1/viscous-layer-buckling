#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
#include <gsl/gsl_math.h>
#include <gsl/gsl_statistics_double.h>
#include <cmath>
//GSL Libraries:
#include <gsl/gsl_errno.h>//GSL error handling functions header
#include <gsl/gsl_fft_real.h> //Real data FFT
#include <gsl/gsl_fft_halfcomplex.h>//
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_vector.h>

using namespace std;
using namespace cv;

template <typename T>
T min (T a, T b) {return a < b ? a : b;}

template <typename T>
T max (T a, T b) {return a > b ? a : b;}

double FrameMeanCurvature(double *, int, double *);
double MovingAverage(gsl_vector *, int, int);
void FFT (double *, int, int);
void SmoothingCurvature(size_t, gsl_vector *, gsl_vector *, ofstream&, ofstream& , ofstream&, int, gsl_vector *);
void SmoothingAmplitude(size_t, gsl_vector *, gsl_vector *, ofstream&, int, gsl_vector *);

// Preview windows are shown unless MSC_NO_DISPLAY is set to a non-empty value.
static bool show_windows () {
    const char *value = getenv ("MSC_NO_DISPLAY");
    return !(value && *value);
}

int main (int argc, char* argv[]){
    if (argc < 3) {
        cerr << "Usage: interface-statistics <experiment_folder> <experiment_name>" << endl;
        return 1;
    }
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
    write_statis << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number" << "\t" << "(3):Maximum y (mm)" << "\t";
    write_statis << "(4):Minimum y (mm)" << "\t" << "(5):Abs(Max-Min) (mm)" << "\t" << "(6): Frame Variance(mm^2)\t"  << "(7):Frame Mean Curvature(1/mm)\t"  << "(8):Frame Max FFT frequency"<< '\n';
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SmoothedData.txt");
    ofstream write_smooth_curv (input_dummy);
    write_smooth_curv << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number\t"  << "(3):Frame Mean Curvature(1/mm) - Moving average method\t";
    write_smooth_curv << "(4):Frame Mean Curvature(1/mm) - Spline method\t" << "(5):Smooth Amplitude (mm) - Moving average method\t" << "(6):Smooth Amplitude (mm) - Spline method" << '\n';
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-FFT.txt");
    ofstream write_fft (input_dummy);
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SplineCurvatureFitParameters.txt");
    ofstream write_spline_fitpara (input_dummy);
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SplineAmplitudeFitParameters.txt");
    ofstream write_amplitude_fitpara (input_dummy);
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-TransitionPoint.txt");
    ofstream write_transition (input_dummy);
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-Splined_Data_Second_Derivative.txt");
    ofstream write_sec_deriv (input_dummy);
    write_sec_deriv << "# Numbers of second derivative points are total frame - 2." << '\n';
    write_sec_deriv << "#(1):Frame number\t" << "(2):Normalized frame number\t" << "(3):Second Derivative of Mean Curvature(1/mm^3)" << '\n';

    // Inputs:
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    cout << "Total frames:" << total_frames << endl;
    double mm_pixel;
    cout << "Inter mm-to-pixel ratio:" << endl;
    cin >> mm_pixel;
    
    //B-Spline Variables:
    const size_t spline_size = total_frames;
    gsl_vector *spline_x, *spline_y;
    spline_x = gsl_vector_alloc(spline_size);
    spline_y = gsl_vector_alloc(spline_size);
    
    // Main Part:
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    int frameindex = 0;
    int stride = 1;
    gsl_vector *frame_amplitude;
    frame_amplitude = gsl_vector_alloc(spline_size);
    
    const bool show = show_windows ();
    while (!frame.empty()) {
        ++frameindex;
        if (original.empty()) {
            cerr << "error: fewer original (-3-Resized) frames than thresholded ones, at frame " << frameindex << endl;
            return 1;
        }
        //Find the contour with the most points in each frame:
        // Image sequences are read as 3-channel images, and findContours needs one channel.
        if (frame.channels () != 1) cvtColor (frame, frame, COLOR_BGR2GRAY);
        findContours(frame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);
        if (contours.empty()) {
            cerr << "error: no contour in thresholded frame " << frameindex << endl;
            return 1;
        }
        int max_idx = 0, max_size = 0;
        for(int idx = 0; idx < contours.size(); ++idx )
        {
            if (contours[idx].size() > max_size) {
                max_size = contours[idx].size();
                max_idx = idx;
            }
        }
        
        //Find the lowest boundary of that contour:
        vector<int> curve (frame.cols, -1);
        vector<int> multiplicity (frame.cols, 0);
        for (auto point : contours[max_idx])    {
            ++multiplicity[point.x];
            if ( curve[point.x]< point.y) curve[point.x] = point.y;
        }
        vector<vector<Point>> curves (1);
        for (auto i = 0; i < curve.size();++i) {
            if (multiplicity[i] != 0) curves[0].push_back(Point(i, curve[i]));//for writing curves on original frames
        }
        
        
        
        
        //Draw that boundary on the original frame:
        polylines (original, curves, false, Scalar::all(255));//
        if (show) {
            imshow (experiment_name +" "+to_string(frameindex)+"/"+to_string(total_frames), original);
            waitKey (150);  //Wait time in milliseconds
        }
        
        //Calculating curvature and variance:
        int data_size = curves[0].size();
        double *y_data = new double[data_size];
        for (auto k = 0; k < data_size; ++k) y_data[k] = curves[0][k].y * mm_pixel;
        
        double y_mean = 0;
        y_mean = gsl_stats_mean(y_data,1,data_size);// Using y_mean for substracting ground state fourier modes (Shifting modes to origin)
        
        double y_min, y_max;
        gsl_stats_minmax (&y_min, &y_max, y_data,stride,data_size);
        gsl_vector_set(frame_amplitude, frameindex-1, abs(y_max - y_min));
        
        double *second_derivative = new double[data_size-2];// For the first and the last points, Could not define second derivative.
        double frame_variance,frame_curvature;
        
        frame_variance = gsl_stats_variance(y_data,1,data_size);
        frame_curvature =  FrameMeanCurvature(y_data,data_size,second_derivative);
        delete []second_derivative;
        
        write_statis << frameindex << "\t" << (float)frameindex/spline_size << "\t" << y_max << "\t" << y_min << "\t" << gsl_vector_get(frame_amplitude,frameindex-1) << "\t" << frame_variance << "\t" << frame_curvature << "\t";
        
        //Smoothing curvature:
        gsl_vector_set(spline_x, frameindex-1, frameindex-1);
        gsl_vector_set(spline_y, frameindex-1, frame_curvature);
        
        //FFT of y_data:
        double *fft_data = new double[data_size];// gsl FTT functions destroy input data.
        for (int j = 0; j < data_size; ++j) fft_data[j] = y_data[j]- y_mean;
        FFT(fft_data,data_size,stride);
        write_fft << "#frame number: " << frameindex << '\n';
        write_fft << "#(1):Pixel Number" << "\t" <<"(2):x component (mm)" << "\t" << "(3):y component (mm)" << "\t" << "(4):FFT of y (1/mm)" << '\n';
        for (int j=0; j< data_size; j++) write_fft << j+1 << "\t" << curves[0][j].x*mm_pixel << "\t" << y_data[j]*mm_pixel << "\t" << fft_data[j] << endl;
        write_fft << '\n' << '\n';
        double frame_fft_max;
        frame_fft_max = gsl_stats_max (fft_data, stride, data_size);
        write_statis << frame_fft_max << '\n';
        delete []fft_data;
        
        
        //Writing images:
        snprintf (filename, sizeof filename, path, frameindex);
        imwrite (filename, original);
        delete []y_data;
        
        cap >> frame;
        cap_o >> original;
    }
    // Smoothing by means of moving average method:
    //Smoothing Curvature:
    int lower,upper;
    gsl_vector *mov_average;
    mov_average = gsl_vector_alloc(spline_size);
    double mov_average_dummy;
    for (int j = 0; j < spline_size; ++j){
        mov_average_dummy = MovingAverage(spline_y,spline_size,j);
        gsl_vector_set(mov_average, j, mov_average_dummy);
    }
    //Smoothing Amplitude:
    gsl_vector *smooth_amplitude;
    smooth_amplitude = gsl_vector_alloc(spline_size);
    for (int j = 0; j < spline_size; ++j){
        mov_average_dummy = MovingAverage(frame_amplitude,spline_size,j);
        gsl_vector_set(smooth_amplitude, j, mov_average_dummy);
    }
    
    // Smoothing by means of B-Spline Method:
    //Smoothing Curvature:
    gsl_vector *spline_smooth;
    spline_smooth = gsl_vector_alloc(spline_size);
    
    SmoothingCurvature(spline_size, spline_x, spline_y, write_spline_fitpara, write_sec_deriv, write_transition, total_frames, spline_smooth);
    
    //Smoothing Amplitude:
    gsl_vector *spline_amplitude;
    spline_amplitude = gsl_vector_alloc(spline_size);
    SmoothingAmplitude(spline_size, spline_x, frame_amplitude, write_amplitude_fitpara, total_frames, spline_amplitude);
    
    for(int i = 0; i < spline_size; ++i){
        write_smooth_curv << i+1 << "\t" <<  (double) i / spline_size << "\t" << gsl_vector_get (mov_average, i) << "\t" << gsl_vector_get (spline_smooth, i) << "\t";
        write_smooth_curv << gsl_vector_get (smooth_amplitude, i) << "\t" << gsl_vector_get (spline_amplitude, i) << '\n';
    }
    
    gsl_vector_free(spline_x);
    gsl_vector_free(mov_average);
    gsl_vector_free(spline_smooth);
    gsl_vector_free(smooth_amplitude);
    gsl_vector_free(spline_amplitude);
    
    write_statis.close();
    write_fft.close();
    write_smooth_curv.close();
    return 0;
}

//Calculating Mean curvature of each frame: curvature = 1 / <|second_derivative|>
double FrameMeanCurvature(double *y_data,int data_size,double * second_derivative){//Simple definition of numerical second derivative.
    float dummy;
    double mean_curvature;
    for (int j = 1; j < data_size-1; ++j){
        dummy = y_data[j-1] - 2 * y_data[j] + y_data[j+1];
        second_derivative[j-1] = abs(dummy);
    }
    mean_curvature = gsl_stats_mean(second_derivative,1,(data_size-2));
    return mean_curvature;
}


// Moving average method over 10 Neighbours for smoothing curvature data:
double MovingAverage(gsl_vector *data, int size, int index){
    int i;
    double sum = 0;
    auto lower = ::max (0, index-5), upper = ::min (size, index+6);//::max is something like cmath::sqrt but nothing before :: means that this is a user-defined function or anything else that is  used.
    for (i = lower; i < upper; i++) sum += gsl_vector_get(data, i);
    sum /= (upper-lower);
    return sum;
}

// FFT of max curve
void FFT (double *fft_data, int data_size, int fft_stride){
    gsl_fft_real_wavetable * real;
    gsl_fft_real_workspace * work;
    
    work = gsl_fft_real_workspace_alloc (data_size);
    real = gsl_fft_real_wavetable_alloc (data_size);
    
    gsl_fft_real_transform (fft_data, fft_stride, data_size, real, work);
    
    gsl_fft_real_wavetable_free (real);
    gsl_fft_real_workspace_free (work);
}

//Smoothing by means of B-Spline:
void SmoothingCurvature(size_t spline_size, gsl_vector *spline_x, gsl_vector *spline_y, ofstream& write_spline_fitpara, ofstream& write_sec_deriv, ofstream& write_transition, int total_frames, gsl_vector *spline_smooth){
    //B-Spline variables:
    double spline_xdummy, spline_ydummy;
    size_t spline_i, spline_j;
    const int spline_order = 4;
    const size_t spline_breaks = 5;
    const size_t spline_coeffs = spline_breaks + spline_order -2;
    gsl_bspline_workspace *spline_workspace;
    gsl_vector *spline_basis;
    gsl_vector *multifit_c;
    gsl_matrix *multifit_X, *multifit_cov;
    gsl_multifit_linear_workspace *multifit_workspace;
    double multifit_chisq, multifit_Rsq, multifit_dof, multifit_tss;
    
    
    /* allocate a k order bspline workspace (k = spline_order) */
    spline_workspace = gsl_bspline_alloc(spline_order, spline_breaks);
    
    spline_basis = gsl_vector_alloc(spline_coeffs);
    multifit_X = gsl_matrix_alloc(spline_size, spline_coeffs);
    multifit_c = gsl_vector_alloc(spline_coeffs);
    multifit_cov = gsl_matrix_alloc(spline_coeffs, spline_coeffs);
    multifit_workspace = gsl_multifit_linear_alloc(spline_size, spline_coeffs);
    
    
    /* use uniform breakpoints based on total frames */
    gsl_bspline_knots_uniform(0,spline_size-1, spline_workspace);
    
    /* construct the fit matrix X */
    double basis_dummy;
    
    for (spline_i = 0; spline_i < spline_size; ++spline_i)
    {
        spline_xdummy = gsl_vector_get(spline_x, spline_i);
        
        /* compute B_j(xi) for all j */
        gsl_bspline_eval(spline_xdummy, spline_basis, spline_workspace);
        
        /* fill in row i of X */
        for (spline_j = 0; spline_j < spline_coeffs; ++spline_j)
        {
            basis_dummy = gsl_vector_get(spline_basis, spline_j);
            gsl_matrix_set(multifit_X, spline_i, spline_j,basis_dummy);
        }
    }
    /* do the fit */
    gsl_multifit_linear(multifit_X, spline_y, multifit_c, multifit_cov, & multifit_chisq, multifit_workspace);
    multifit_dof = spline_size - spline_coeffs;
    multifit_tss = gsl_stats_tss(spline_y->data, 1, spline_y->size);
    multifit_Rsq = 1.0 - multifit_chisq / multifit_tss;
    write_spline_fitpara << "Standard error = " << stderr << '\n' << "chisq/dof = " <<  multifit_chisq / multifit_dof << '\n';//stderr is a variable from Error Handling header file.
    write_spline_fitpara << "Rsq = "  << multifit_Rsq << '\n';
    write_spline_fitpara.close();
    
    
    // output the smoothed curve
    double multifit_yi,multifit_yerr;
    
    gsl_vector *smoothed_curvature;
    smoothed_curvature = gsl_vector_alloc(spline_size);
    for (int frame_number = 0 ; frame_number < spline_size; frame_number++)
    {
        gsl_bspline_eval(frame_number, spline_basis, spline_workspace);
        gsl_multifit_linear_est(spline_basis, multifit_c, multifit_cov, &multifit_yi, &multifit_yerr);
        
        
    //Smoothed data for calculating second derivative of curvature:
    gsl_vector_set(smoothed_curvature,frame_number, multifit_yi);
    gsl_vector_set(spline_smooth, frame_number, multifit_yi);
    }
    //Second Derivative of smouthed data:
    float dummy;
    double * curv_sec_deri = new double[spline_size-2];
    for (int j = 1; j < spline_size-1; ++j){
        dummy = gsl_vector_get(smoothed_curvature,j-1) - 2 * gsl_vector_get(smoothed_curvature,j) + gsl_vector_get(smoothed_curvature,j+1);
        curv_sec_deri[j-1] = dummy;
        write_sec_deriv << j << (double) j/(spline_size-1) << curv_sec_deri[j-1] << '\n';
    }
    write_sec_deriv.close();
    
    size_t max_index;
    max_index = gsl_stats_max_index (curv_sec_deri, 1,spline_size-2);
    cout << "Index of Maximum value of second derivative (Based on array index rule):(transition frame number in curvature data = this number + 2)\t" << max_index << endl;
    write_transition << "(1):Index of Maximum value of second derivative (Based on array index rule)\t" << "(2):Coresponding frame number" << '\n';
    write_transition << max_index << max_index+2;//
    write_transition.close();
    
    //Free memory:
    gsl_bspline_free(spline_workspace);
    gsl_vector_free(spline_basis);
    gsl_vector_free(spline_y);
    
    gsl_matrix_free(multifit_X);
    gsl_vector_free(multifit_c);
    gsl_matrix_free(multifit_cov);
    gsl_multifit_linear_free(multifit_workspace);
    
    
    gsl_vector_free(smoothed_curvature);
    delete []curv_sec_deri;
}
void SmoothingAmplitude(size_t spline_size, gsl_vector *spline_x, gsl_vector *frame_amplitude, ofstream& write_amplitude_fitpara, int total_frames, gsl_vector *spline_amplitude){
    //B-Spline variables:
    double spline_xdummy, spline_ydummy;
    size_t spline_i, spline_j;
    const int spline_order = 4;
    const size_t spline_breaks = 5;
    const size_t spline_coeffs = spline_breaks + spline_order -2;
    gsl_bspline_workspace *spline_workspace;
    gsl_vector *spline_basis;
    gsl_vector *multifit_c;
    gsl_matrix *multifit_X, *multifit_cov;
    gsl_multifit_linear_workspace *multifit_workspace;
    double multifit_chisq, multifit_Rsq, multifit_dof, multifit_tss;

    /* allocate a k order bspline workspace (k = spline_order) */
    spline_workspace = gsl_bspline_alloc(spline_order, spline_breaks);
    
    spline_basis = gsl_vector_alloc(spline_coeffs);
    multifit_X = gsl_matrix_alloc(spline_size, spline_coeffs);
    multifit_c = gsl_vector_alloc(spline_coeffs);
    multifit_cov = gsl_matrix_alloc(spline_coeffs, spline_coeffs);
    multifit_workspace = gsl_multifit_linear_alloc(spline_size, spline_coeffs);
    /* use uniform breakpoints based on total frames */
    gsl_bspline_knots_uniform(0,spline_size-1, spline_workspace);
    
    /* construct the fit matrix X */
    double basis_dummy;
    for (spline_i = 0; spline_i < spline_size; ++spline_i)
    {
        spline_xdummy = gsl_vector_get(spline_x, spline_i);
        /* compute B_j(xi) for all j */
        gsl_bspline_eval(spline_xdummy, spline_basis, spline_workspace);
        /* fill in row i of X */
        for (spline_j = 0; spline_j < spline_coeffs; ++spline_j){
            basis_dummy = gsl_vector_get(spline_basis, spline_j);
            gsl_matrix_set(multifit_X, spline_i, spline_j,basis_dummy);
        }
    }
    /* do the fit */
    gsl_multifit_linear(multifit_X, frame_amplitude, multifit_c, multifit_cov, & multifit_chisq, multifit_workspace);
    multifit_dof = spline_size - spline_coeffs;
    multifit_tss = gsl_stats_tss(frame_amplitude->data, 1, frame_amplitude->size);
    multifit_Rsq = 1.0 - multifit_chisq / multifit_tss;
    write_amplitude_fitpara << "Standard error = " << stderr << '\n' << "chisq/dof = " <<  multifit_chisq / multifit_dof << '\n';//stderr is a variable from Error Handling header file.
    write_amplitude_fitpara << "Rsq = "  << multifit_Rsq << '\n';
    write_amplitude_fitpara.close();
    
    // output the smoothed curve
    double multifit_yi,multifit_yerr;
    for (int frame_number = 0 ; frame_number < spline_size; frame_number++)
    {
        gsl_bspline_eval(frame_number, spline_basis, spline_workspace);
        gsl_multifit_linear_est(spline_basis, multifit_c, multifit_cov, &multifit_yi, &multifit_yerr);
        //Smoothed data for calculating second derivative of curvature:
        gsl_vector_set(spline_amplitude, frame_number, multifit_yi);
    }
    //Free memory:
    gsl_bspline_free(spline_workspace);
    gsl_vector_free(spline_basis);
    gsl_vector_free(frame_amplitude);
    gsl_matrix_free(multifit_X);
    gsl_vector_free(multifit_c);
    gsl_matrix_free(multifit_cov);
    gsl_multifit_linear_free(multifit_workspace);
}

