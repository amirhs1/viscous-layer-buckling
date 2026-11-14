#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#include <boost/filesystem.hpp>//Boost Library
//OpenCV Libraries:
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
//GSL Libraries:
#include <gsl/gsl_math.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_errno.h>//GSL error handling functions header
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

double FrameMeanCurvature(double *, int, double *, double *, double *);
double MovingAverage(gsl_vector *, int, int);
void BSplineCurvature(double, size_t, gsl_vector *, gsl_vector *, ofstream&, ofstream& , ofstream&, int, gsl_vector *, gsl_vector *, double *, double *, double *);
void BSplineAmplitude(size_t, gsl_vector *, gsl_vector *, ofstream&, int, gsl_vector *, gsl_vector *);

// Preview windows are shown unless MSC_NO_DISPLAY is set to a non-empty value.
static bool show_windows () {
    const char *value = getenv ("MSC_NO_DISPLAY");
    return !(value && *value);
}

int main (int argc, char* argv[]){
    if (argc < 5) {
        cerr << "Usage: final-pipeline <experiment_folder> <experiment_name> <mm_per_pixel> <period_switch: 0 or 1>" << endl;
        return 1;
    }

    string experiment_folder = argv[1];
    cout << "Enter Experiment Folder: " << endl << experiment_folder<< endl;
    string experiment_name = argv[2];
    cout << "Enter Experiment Name: " << endl << experiment_name << endl;
    
    string input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-Inputs.txt");
    ofstream write_inputs (input_dummy);
    
    write_inputs << experiment_folder << '\n' << experiment_name << '\n';
    
    string input_address = experiment_folder;
    input_address.append("/");
    input_address.append(experiment_name);
    input_address.append("-4-Thershold/");
    input_address.append(experiment_name);
    input_address.append("-Thershold-%04d.tif");
    cout << "Enter Input Images Path: " << endl << input_address << endl;
    write_inputs << input_address << '\n';
    
    string output_address = experiment_folder;
    output_address.append("/");
    output_address.append(experiment_name);
    output_address.append("-5-MinContour/");
    output_address.append(experiment_name);
    output_address.append("-MinContour-%04d.tif");
    cout << "Enter Output Images Path: " << endl << output_address << endl;
    write_inputs << output_address << '\n';
    
    const char * path = output_address.c_str();//Path for writing output frames and data:
    char filename[500];
    
   
    
    string original_address = experiment_folder;
    original_address.append("/");
    original_address.append(experiment_name);
    original_address.append("-3-Resized/");
    original_address.append(experiment_name);
    original_address.append("-Resized-%04d.tif");
    cout << "Enter Original Images Path: " << endl << original_address << endl;
    write_inputs << original_address << '\n';
    

    VideoCapture cap {input_address};
    // Keep the thresholded frames 8-bit single-channel, as OpenCV 2/3 returned them;
    // since OpenCV 4 an image sequence is converted to BGR, which findContours rejects.
    cap.set (CAP_PROP_CONVERT_RGB, false);
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
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-FrameStatistics.txt");
    ofstream write_statis (input_dummy);
    write_statis << "#First frame is assumed zero frame beacuse of zero time so total frame of experiment is reduced by 1." << '\n';
    write_statis << "#" << experiment_name << '\n';
    write_statis << "#(1):Frame number\t" << "(2):Normalized Frame Number\t" << "(3):A(F) (mm)\t";
    write_statis << "(4):A(F) - error (mm)\t" << "(5):K(F) (1/mm)\t" << "(6):K(F) - error (1/mm)\t" << "(7):Var(F) (mm^2)" << '\n';
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SmoothedData.txt");
    ofstream write_smooth_curv (input_dummy);
    write_smooth_curv << "#First frame is assumed zero frame beacuse of zero time so total frame of experiment is reduced by 1." << '\n';
    write_smooth_curv << "#" << experiment_name << '\n';
    write_smooth_curv << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number\t"  << "(3):Smoothed A(F) (mm) - Mov.Ave.\t" << "(4):Smoothed K(F) (1/mm) - Mov.Ave.\t";
    write_smooth_curv << "(5):Smooth A(F) (mm) - B.Spline\t" << "(6):Smooth A(F) - error (mm) - B.Spline\t";
    write_smooth_curv << "(7):Smoothed K(F) (1/mm) - B.Spline\t" << "(8):Smoothed K(F) - error (1/mm) - B.Spline\t" << "(9):Var(F) (mm^2)- Mov.Ave" << '\n';
    
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
    write_transition << "#First frame is assumed zero frame beacuse of zero time so total frame of experiment is reduced by 1." << '\n';
    write_transition << "#" << experiment_name << '\n';
    write_transition << "#Index of Maximum value of second derivative (Based on array index rule):(transition frame number in curvature data = this number )" << '\n';
    write_transition << "(1) F_c - Mov. Ave. Max of var.\t";
    write_transition << "(2) F_c - Mov. Ave. Min of var. before Max.\t";
    write_transition << "(3) F_c - Mov. Ave. Max of cur.\t";
    write_transition << "(4) F_c - Mov. Ave. Min of cur. before Max.\t";
    write_transition << "(5):Index sec. deri. - Mov. Ave. \t" << "(6):Index sec. deri. - MovAve. Vari. Sec.\t";
    write_transition << "(7) F_c - B.Spline\t" << "(8):Index sec. deri. - B.Spline Derivative method\t"  << '\n';
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-Sec_Der_of_Curv.txt");
    ofstream write_sec_deriv (input_dummy);
    write_sec_deriv << "#" << experiment_name << '\n';
    write_sec_deriv << "#First frame is assumed zero frame beacuse of zero time so total frame of experiment is reduced by 1." << '\n';
    write_sec_deriv << "# Numbers of second derivative points are total frame - 2." << '\n';
    write_sec_deriv << "#(1):Frame number\t" << "(2):Normalized frame number\t" << "(3):K''(F) (1/mm^3) - Mov.Ave.\t" << "(4):K''(F) (1/mm^3) - B.Spline\t" << "(5):K''(F) (1/mm^3) - B.Spline with its internal derivation process" << "(6): Variance  Sec. Deri. (mm^0):\t" << "(7): Variance Fir. Deri. (mm^1):\t" << '\n';
    
    // Inputs:
    const double delta_t = 0.0166;
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    cout << "Total frames:" << total_frames << endl;
    write_inputs << total_frames << '\n';
    double mm_pixel = std::stod (argv[3]);
    cout << "Inter mm-to-pixel ratio: " << mm_pixel << endl;
    write_inputs << mm_pixel << '\n';
    
    //B-Spline Variables:
    const size_t spline_size = total_frames;
    gsl_vector *spline_x, *spline_z;
    spline_x = gsl_vector_alloc(spline_size);
    spline_z = gsl_vector_alloc(spline_size);
    
    // Main Part:
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    int frameindex = 0;
    int stride = 1;
    gsl_vector *frame_variance;
    frame_variance = gsl_vector_alloc(spline_size);
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
        for (auto i = 0; i < curve.size();++i)
            if (multiplicity[i] != 0) curves[0].push_back(Point(i, curve[i]));//for writing curves on original frames

        //Draw that boundary on the original frame:
        polylines (original, curves, false, Scalar::all(255));//
        if (show) {
            imshow (experiment_name +" "+to_string(frameindex)+"/"+to_string(total_frames), original);
            waitKey (50);  //Wait time in milliseconds
        }
        
        //Calculating curvature and variance:
        int data_size = curves[0].size();
        double *z_data = new double[data_size];
        for (auto k = 0; k < data_size; ++k) z_data[k] = curves[0][k].y;
        
        double y_min, y_max;
        gsl_stats_minmax (&y_min, &y_max, z_data,stride,data_size);
        gsl_vector_set(frame_amplitude, frameindex-1, abs(y_max - y_min));
        double var_dummy;
        var_dummy = gsl_stats_variance(z_data,1,data_size);
        gsl_vector_set(frame_variance, frameindex-1, var_dummy);
        
        double *second_derivative = new double[data_size - 2];// For the first and the last points, Could not define second derivative.
        double *first_derivative = new double[data_size - 2];// For the first and the last points, Could not define first derivative.
        double *local_curvature = new double[data_size - 2];
        double frame_curvature;
        frame_curvature =  FrameMeanCurvature(z_data,data_size,second_derivative,first_derivative,local_curvature);
        delete []second_derivative;
        delete []first_derivative;
        delete []local_curvature;
        
        write_statis << frameindex-1 << "\t" << (double)frameindex/(spline_size-1) << "\t" << gsl_vector_get(frame_amplitude,frameindex-1) * mm_pixel << "\t" <<  mm_pixel << "\t" << frame_curvature / mm_pixel << "\t" <<  "?" << "\t" << gsl_vector_get(frame_variance,frameindex-1) * mm_pixel * mm_pixel <<'\n';
        
        //Smoothing curvature:
        gsl_vector_set(spline_x, frameindex-1, frameindex-1);
        gsl_vector_set(spline_z, frameindex-1, frame_curvature);
        
        //Writing images:
        snprintf (filename, sizeof filename, path, frameindex);
        imwrite (filename, original);
        delete []z_data;
        
        cap >> frame;
        cap_o >> original;
    }
    write_statis << '\n' << '\n';
    // Smoothing by means of Moving Average method:
    int lower,upper;
    /*Smoothed Variance:*/
    double *movAve_variance = new double[spline_size];
    /*Smoothed Curvature:*/
    gsl_vector *movAve_curvature;
    movAve_curvature = gsl_vector_alloc(spline_size);
    /*Smoothed Amplitude:*/
    gsl_vector *movAve_amplitude;
    movAve_amplitude = gsl_vector_alloc(spline_size);
    double movAve_dummy;
    /*Smoothing Process:*/
    for (int j = 0; j < spline_size; ++j){
        movAve_dummy = MovingAverage(frame_variance,spline_size,j);
        movAve_variance[j] = movAve_dummy;
        
        movAve_dummy = MovingAverage(spline_z,spline_size,j);
        gsl_vector_set(movAve_curvature, j, movAve_dummy);
        
        movAve_dummy = MovingAverage(frame_amplitude,spline_size,j);
        gsl_vector_set(movAve_amplitude, j, movAve_dummy);
    }
        //Finding transition point:
    
    double * movAve_sec_deri = new double[total_frames-2];
    double * movAve_sec_deri_vari = new double[total_frames-2];
    double * movAve_first_deri = new double[total_frames-2];
    double * movAve_first_deri_vari = new double[total_frames-2];
    double * movAve_curvature_dummy = new double[total_frames-2];
    for (int j = 1; j < total_frames-1; ++j){
        movAve_dummy = movAve_variance[j-1] - 2.0 * movAve_variance[j] + movAve_variance[j+1];
        movAve_sec_deri_vari[j-1] = movAve_dummy;
        movAve_dummy = (movAve_variance[j+1] - movAve_variance [j-1])/2;
        movAve_first_deri_vari[j-1] = movAve_dummy;
        movAve_dummy = gsl_vector_get(movAve_curvature,j-1) - 2.0 * gsl_vector_get(movAve_curvature,j) + gsl_vector_get(movAve_curvature,j+1);
        movAve_sec_deri[j-1] = movAve_dummy;
        movAve_dummy = (gsl_vector_get(movAve_curvature,j+1) - gsl_vector_get(movAve_curvature,j-1))/2;
        movAve_first_deri[j-1] = movAve_dummy;
        movAve_dummy = gsl_vector_get(movAve_curvature,j-1);
        movAve_curvature_dummy[j-1] = movAve_dummy;
    }
    size_t max_index;
    max_index = gsl_stats_max_index (movAve_variance, 1,spline_size-2);
    size_t max_index_cur;
    max_index_cur  = gsl_stats_max_index (movAve_curvature_dummy, 1,spline_size-2);
    int min_index = 0;
    int min_index_cur = 0;
    if (max_index < 5)
        cout << "max_index is smaller than 5." << endl;
    else
        for (int i = 0; i < max_index - 5; ++i)
            if ( movAve_first_deri_vari[i] * movAve_first_deri_vari[i+1] < 0)
                min_index = i;
    
    if (max_index_cur < 5)
        cout << "max_index is smaller than 5." << endl;
    else
        for (int i = 0; i < max_index_cur - 5; ++i)
            if ( movAve_first_deri[i] * movAve_first_deri[i+1] < 0)
                min_index_cur = i;
    
    cout << "Mov. Ave F_c: (Max of variance) " << max_index << endl;
    cout << "Mov. Ave F_c: (Min before Max of variance) " << min_index << endl;
    cout << "Mov. Ave F_c: (Max of curvature) " << max_index_cur << endl;
    cout << "Mov. Ave F_c: (Min before Max of curvature) " << min_index_cur << endl;
    write_transition << max_index << "\t";
    write_transition << min_index << "\t";
    write_transition << max_index_cur << "\t";
    write_transition << min_index_cur << "\t";
    max_index = gsl_stats_max_index (movAve_sec_deri, 1,spline_size-2);
    cout << "Index of Maximum value of second derivative based on array index rule (transition frame number in curvature data = this number):" << endl;
    cout << "Mov. Ave F_c: " << max_index << endl;
    write_transition << max_index << "\t";
    max_index = gsl_stats_max_index (movAve_sec_deri_vari, 1,spline_size-2);
    cout << "Mov. Ave F_c (variance): " << max_index << endl;
    write_transition << max_index << "\t";
    
    // Smoothing by means of B-Spline Method:
    /*Smoothed Curvature:*/
    gsl_vector *spline_curvature;
    gsl_vector *spline_curvature_er;
    spline_curvature = gsl_vector_alloc(spline_size);
    spline_curvature_er = gsl_vector_alloc(spline_size);
    /*Smoothed Amplitude:*/
    gsl_vector *spline_amplitude;
    gsl_vector *spline_amplitude_er;
    spline_amplitude = gsl_vector_alloc(spline_size);
    spline_amplitude_er = gsl_vector_alloc(spline_size);
    
    /*Smoothing Process:*/
    BSplineCurvature(mm_pixel, spline_size, spline_x, spline_z, write_spline_fitpara, write_sec_deriv, write_transition, total_frames, spline_curvature, spline_curvature_er, movAve_sec_deri, movAve_sec_deri_vari, movAve_first_deri_vari);
    BSplineAmplitude(spline_size, spline_x, frame_amplitude, write_amplitude_fitpara, total_frames, spline_amplitude, spline_amplitude_er);
    
    // Writing Smoothing data:
    for(int i = 0; i < spline_size; ++i){
        write_smooth_curv << i << "\t" <<  (double) i / (spline_size-1) << "\t";
        write_smooth_curv << gsl_vector_get (movAve_amplitude, i) * mm_pixel << "\t" << gsl_vector_get (movAve_curvature, i) / mm_pixel  << "\t";
        write_smooth_curv << gsl_vector_get (spline_amplitude, i) * mm_pixel << "\t" << gsl_vector_get (spline_amplitude_er, i) * mm_pixel << "\t";
        write_smooth_curv << gsl_vector_get (spline_curvature, i) / mm_pixel << "\t" << gsl_vector_get (spline_curvature_er, i) / mm_pixel << "\t";
        write_smooth_curv << movAve_variance[i] * mm_pixel * mm_pixel << '\n';
    }
    write_smooth_curv << '\n' << '\n';
    // Frequency of No-Buckling State:
    vector<int> extremum_index;
    int extremum_counter = 0;
    int extremum_switch = std::stod (argv[4]);
    double extremum_sum = 0.0;
    double extremum_half_frequency;
    if(extremum_switch){
        input_dummy = experiment_folder;
        input_dummy.append("/");
        input_dummy.append(experiment_name);
        input_dummy.append("-FrequencyData.txt");
        ofstream write_frequency (input_dummy);
        write_frequency << "#" << experiment_name << '\n';
        write_frequency << "(1) Number of Index:\t" << "(1) Sum of difference:\t" << "(3) Period (number of frames):\t" << "(4) Period (seconds):\t" << "(5)  Time interval(Delta t) sec.:" << '\n';
        
        for (int i = 0; i < total_frames - 3; ++i)//Be careful total_frames - 3 not total_frames - 2 !
            if ( movAve_first_deri_vari[i] * movAve_first_deri_vari[i+1] < 0){
                ++extremum_counter;
                extremum_index.push_back(i);
            }
        if (extremum_counter < 2) {
            cerr << "Fewer than two extrema in the smoothed variance: no oscillation period to measure." << endl;
            write_frequency << extremum_counter << "\t" << "fewer than two extrema: no period" << '\n';
        } else {
        for(size_t i = 0; i + 1 < extremum_index.size(); ++i)
             extremum_sum += extremum_index[i+1] - extremum_index[i];
        extremum_half_frequency = extremum_sum / (extremum_counter - 1);//Number of extremum is one more than number of crests and picks.
        cout << "TIme interval (Delta t): " << delta_t << endl;
        cout << "Frequency in frames: " << 2 * extremum_half_frequency << endl;
        cout << "Frequency in seconds: " << (2 * extremum_half_frequency)* delta_t << endl;
        write_frequency << extremum_counter << "\t" << extremum_sum << "\t" << 2 * extremum_half_frequency << "\t";
        write_frequency << (2 * extremum_half_frequency) * delta_t << delta_t << '\n';
        }
        write_frequency << '\n' << '\n' << '\n';
        write_frequency.close();
    }
    delete []movAve_first_deri_vari;   // freed here, after its last use (it used to be freed in BSplineCurvature)
    gsl_vector_free(frame_variance);
    gsl_vector_free(frame_amplitude);
    gsl_vector_free(spline_x);
    
    delete []movAve_variance;
    gsl_vector_free(movAve_curvature);
    gsl_vector_free(movAve_amplitude);
    
    gsl_vector_free(spline_curvature);
    gsl_vector_free(spline_amplitude);
    
    write_statis.close();
    write_smooth_curv.close();
    write_inputs.close();
    return 0;
}

//*******************************************************************************

//Calculating Mean curvature of each frame: locaal_curvature = <|second_derivative|>/sqrt[{(first_derivative)^2+1}^3]
double FrameMeanCurvature(double *z_data,int data_size, double * second_derivative, double * first_derivative, double * local_curvature){//Simple definition of numerical second derivative.
    double dummy;
    double curvature;
    for (int j = 1; j <= data_size - 2; ++j){
        dummy = z_data[j + 1] - 2 * z_data[j] + z_data[j - 1];// mm_pixel for z_data removes one of  mm_pixel^2 in the definition of second derivative.
        second_derivative[j-1] = abs(dummy);
        dummy = (z_data[j + 1] - z_data[j - 1]) / 2;//mm_pixel of z_data removes mm_pixel in th definition of first derivative.
        first_derivative[j-1] = abs(dummy);
        dummy = sqrt(pow(((first_derivative[j-1]*first_derivative[j-1])+1),3.0));//1/((((dz/dx)^2+1)^3)^{1/2})
        local_curvature[j-1] = second_derivative[j-1] / dummy;
    }
    
    curvature = gsl_stats_mean(local_curvature,1,data_size - 2 );
    return curvature;
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

//Smoothing by means of B-Spline:
void BSplineCurvature(double mm_pixel, size_t spline_size, gsl_vector *spline_x, gsl_vector *spline_z, ofstream& write_spline_fitpara, ofstream& write_sec_deriv, ofstream& write_transition, int total_frames, gsl_vector *spline_curvature, gsl_vector *spline_curvature_er, double *movAve_sec_deri, double *movAve_sec_deri_vari, double *movAve_first_deri_vari){
    //B-Spline variables:
    double spline_xdummy, spline_zdummy;
    size_t spline_i, spline_j;
    const int spline_order = 4;
    const size_t spline_breaks = 5;
    const size_t spline_coeffs = spline_breaks + spline_order -2;
    const size_t spline_nderiv = 2;
    
    gsl_bspline_workspace *spline_workspace;
    gsl_vector *multifit_c;
    gsl_matrix *multifit_X, *multifit_cov;
    double multifit_chisq, multifit_Rsq, multifit_dof, multifit_tss;
    
    gsl_matrix *spline_derivative;
    gsl_vector *multifit_der_c;
    gsl_matrix *multifit_der_X, *multifit_der_cov;
    gsl_multifit_linear_workspace *multifit_der_workspace;
    double multifit_der_chisq, multifit_der_Rsq, multifit_der_dof, multifit_der_tss;
    
    /* allocate a k order bspline workspace (k = spline_order) */
    spline_workspace = gsl_bspline_alloc(spline_order, spline_breaks);
    
    multifit_X = gsl_matrix_alloc(spline_size, spline_coeffs);
    multifit_c = gsl_vector_alloc(spline_coeffs);
    multifit_cov = gsl_matrix_alloc(spline_coeffs, spline_coeffs);
    
    spline_derivative = gsl_matrix_alloc(spline_coeffs, spline_nderiv+1);
    multifit_der_X = gsl_matrix_alloc(spline_size, spline_coeffs);
    multifit_der_c = gsl_vector_alloc(spline_coeffs);
    multifit_der_cov = gsl_matrix_alloc(spline_coeffs, spline_coeffs);
    multifit_der_workspace = gsl_multifit_linear_alloc(spline_size, spline_coeffs);
    
    /* use uniform breakpoints based on total frames */
    gsl_bspline_knots_uniform(0,spline_size-1, spline_workspace);
    
    /* construct the fit matrix X */
    double dummy;
    gsl_vector *spline_sec_deri_basis;
    spline_sec_deri_basis = gsl_vector_alloc(spline_coeffs);
    
    for (spline_i = 0; spline_i < spline_size; ++spline_i){
        spline_xdummy = gsl_vector_get(spline_x, spline_i);
        
        /* compute B_j(xi) and its derivative for all j */
        gsl_bspline_deriv_eval(spline_xdummy, spline_nderiv, spline_derivative, spline_workspace);
        
        /* fill in row i of X */
        for (spline_j = 0; spline_j < spline_coeffs; ++spline_j){
            dummy = gsl_matrix_get(spline_derivative, spline_j, 0);//d^0B_j(x)/dx^0
            gsl_matrix_set(multifit_X, spline_i, spline_j,dummy);//evaluate multifit_x at i,j
            
            dummy = gsl_matrix_get(spline_derivative, spline_j, 2);//d^2B_j(x)/dx^2
            gsl_matrix_set(multifit_der_X, spline_i, spline_j,dummy);//evaluate multifit_der_x at i,j
        }
    }
    /* do the fit */
    gsl_multifit_linear(multifit_X, spline_z, multifit_c, multifit_cov, & multifit_chisq, multifit_der_workspace);
    multifit_dof = spline_size - spline_coeffs;
    multifit_tss = gsl_stats_tss(spline_z->data, 1, spline_z->size);
    multifit_Rsq = 1.0 - multifit_chisq / multifit_tss;
    write_spline_fitpara << "Standard error = " << stderr << '\n' << "chisq/dof = " <<  multifit_chisq / multifit_dof << '\n';//stderr is a variable from Error Handling header file.
    write_spline_fitpara << "Rsq = "  << multifit_Rsq << '\n';
    
    // do the fit for B-Spline with derivative:
    gsl_multifit_linear(multifit_der_X, spline_z, multifit_der_c, multifit_der_cov, & multifit_der_chisq, multifit_der_workspace);
    multifit_der_dof = spline_size - spline_coeffs;
    multifit_der_tss = gsl_stats_tss(spline_z->data, 1, spline_z->size);
    multifit_der_Rsq = 1.0 - multifit_der_chisq / multifit_der_tss;
    write_spline_fitpara << "Second derivative data:"  << '\n';
    write_spline_fitpara << "Standard error = " << stderr << '\n' << "chisq/dof = " <<  multifit_der_chisq / multifit_der_dof << '\n';//stderr is a variable from Error Handling header file.
    write_spline_fitpara << "Rsq = "  << multifit_der_Rsq << '\n';
    write_spline_fitpara.close();
    
    // Evaluate smouthed curvatue and its second derivative by means of X and der_X matrices:
    double multifit_yi,multifit_yerr;
    double multifit_der_yi,multifit_der_yerr;
    double * spline_sec_deri = new double[spline_size];
    
    for (int frame_number = 0 ; frame_number < spline_size; frame_number++){
        
    // B-Spline with derivative
        gsl_bspline_deriv_eval(frame_number, spline_nderiv, spline_derivative, spline_workspace);
   
        gsl_matrix_get_col (spline_sec_deri_basis, spline_derivative, 0);
        gsl_multifit_linear_est(spline_sec_deri_basis, multifit_c, multifit_cov, &multifit_yi, &multifit_yerr);
        gsl_matrix_get_col (spline_sec_deri_basis, spline_derivative, 2);
        gsl_multifit_linear_est(spline_sec_deri_basis, multifit_der_c, multifit_der_cov, &multifit_der_yi, &multifit_der_yerr);
        
    //Smoothed data for calculating second derivative of curvature:
        gsl_vector_set(spline_curvature,frame_number, multifit_yi);
        gsl_vector_set(spline_curvature_er,frame_number, multifit_yerr);
        spline_sec_deri[frame_number] = multifit_der_yi;
    }
    
    //Second Derivative of smoothed data:
    double * spline_curv_sec_deri = new double[spline_size-2];
    for (int j = 1; j < spline_size-1; ++j){
        dummy = gsl_vector_get(spline_curvature,j-1) - 2 * gsl_vector_get(spline_curvature,j) + gsl_vector_get(spline_curvature,j+1);
        spline_curv_sec_deri[j-1] = dummy;
        write_sec_deriv << j-1 << "\t" << (double) (j-1)/(spline_size-2) << "\t" << movAve_sec_deri[j-1] / pow(mm_pixel,3.0)  << "\t" << spline_curv_sec_deri[j-1] / pow(mm_pixel,3.0) << "\t" << spline_sec_deri[j-1] / pow(mm_pixel,3.0) << "\t" << movAve_sec_deri_vari[j-1] << "\t" << movAve_first_deri_vari[j-1] * mm_pixel << '\n';
    }
    write_sec_deriv << '\n' << '\n';
    write_sec_deriv.close();
    
    size_t max_index;
    max_index = gsl_stats_max_index (spline_curv_sec_deri, 1,spline_size-2);
    cout << "BSpline. F_c: " << max_index  << endl;
    write_transition << max_index << "\t";
    max_index = gsl_stats_max_index (spline_sec_deri, 1,spline_size-2);
    cout << "BSpline. internal derivation F_c: " << max_index << endl;
    write_transition << max_index << '\n' << '\n' << '\n';
    write_transition.close();
    
    //Free memory:
    gsl_bspline_free(spline_workspace);
    gsl_vector_free(spline_z);
    
    gsl_matrix_free(multifit_X);
    gsl_vector_free(multifit_c);
    gsl_matrix_free(multifit_cov);
    
    gsl_matrix_free(multifit_der_X);
    gsl_vector_free(multifit_der_c);
    gsl_matrix_free(multifit_der_cov);
    gsl_matrix_free(spline_derivative);
    gsl_vector_free(spline_sec_deri_basis);
    gsl_multifit_linear_free(multifit_der_workspace);
    
    delete []spline_curv_sec_deri;
    delete []movAve_sec_deri;
    delete []movAve_sec_deri_vari;
    delete []spline_sec_deri;
}
void BSplineAmplitude(size_t spline_size, gsl_vector *spline_x, gsl_vector *frame_amplitude, ofstream& write_amplitude_fitpara, int total_frames, gsl_vector *spline_amplitude, gsl_vector *spline_amplitude_er){
    //B-Spline variables:
    double spline_xdummy, spline_zdummy;
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
        gsl_vector_set(spline_amplitude_er, frame_number, multifit_yerr);
    }
    //Free memory:
    gsl_bspline_free(spline_workspace);
    gsl_vector_free(spline_basis);
    gsl_matrix_free(multifit_X);
    gsl_vector_free(multifit_c);
    gsl_matrix_free(multifit_cov);
    gsl_multifit_linear_free(multifit_workspace);
}

