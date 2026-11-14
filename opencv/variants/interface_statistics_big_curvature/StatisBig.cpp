#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
//Boost Library:
#include <boost/filesystem.hpp>
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

double FrameMeanCurvature(double *, int, double *, double *,double *);
void SmoothingCurvature(double, size_t, gsl_vector *, gsl_vector *, ofstream&, ofstream& , ofstream&, int, gsl_vector *);
void SmoothingAmplitude(size_t, gsl_vector *, gsl_vector *, ofstream&, int, gsl_vector *);

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
    
    // Loading thersholded frames:
    VideoCapture cap {input_address};
    if (!cap.isOpened())
    {
        cerr << "Failed to open Image Sequence!\n" << endl;
        return 1;
    }
    Mat frame;
    cap >> frame;
    
    // Loading original frames:
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
    write_statis << "#(1):Frame number" << "\t" << "(2):Normalized Frame Number\t" << "(3):Frame Amplitude (mm)\t" << "(4):Frame Mean Curvature(1/mm)" << endl;
    
    input_dummy = experiment_folder;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-SmoothedData.txt");
    ofstream write_smooth_curv (input_dummy);
    write_smooth_curv << "#(1):Frame number\t" << "(2):Normalized Frame Number\t" << "(3):Frame Mean Curvature(1/mm) - B-Spline\t" << "(4):Smooth Amplitude(mm) - B-Spline" << endl;
    
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
    write_sec_deriv << "# Numbers of second derivative points are total frame - 2." << endl;
    write_sec_deriv << "#(1):Frame number\t" << "(2):Normalized frame number\t" << "(3):Second Derivative of Mean Curvature(1/mm^3)" << endl;

    // Inputs:
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    cout << "Total frames:" << total_frames << endl;
    double mm_pixel;
    cout << "Inter mm-to-pixel ratio:" << endl;
    cin >> mm_pixel;
    
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
    gsl_vector *frame_amplitude;
    frame_amplitude = gsl_vector_alloc(spline_size);
    
    while (!frame.empty()) {
        ++frameindex;
        //Find Contour with maximum area in each frame:
        findContours(frame, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//OpenCV functions destroy input frame.
        int max_idx = 0, max_size = 0;
        for(int idx = 0; idx < contours.size(); ++idx )
        {
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
        for (auto i = 0; i < curve.size();++i) {
            //            if (multiplicity[i] != 0) curves[0].push_back(Point(i, curve[i]));
            if (multiplicity[i] != 0) curves[0].push_back(Point(i, curve[i]));//for writing curves on original frames
        }

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
        gsl_vector_set(frame_amplitude, frameindex-1, abs(y_max - y_min));
        
        double *second_derivative = new double[data_size-2];// For the first and the last points, Could not define second derivative.
        double *first_derivative = new double[data_size-2];// For the first and the last points, Could not define first derivative.
        double *local_curvature = new double[data_size-2];
        double frame_curvature;

        frame_curvature =  FrameMeanCurvature(z_data,data_size,second_derivative,first_derivative,local_curvature);
        
        write_statis << frameindex << "\t" << (float)frameindex/spline_size << "\t" << gsl_vector_get(frame_amplitude,frameindex-1) * mm_pixel << "\t" << frame_curvature / mm_pixel << endl;
        
        //Smoothing curvature:
        gsl_vector_set(spline_x, frameindex-1, frameindex-1);
        gsl_vector_set(spline_z, frameindex-1, frame_curvature);
        
        //Writing images:
        sprintf (filename, path, frameindex);
        imwrite (filename, original);
        delete []z_data;
        
        cap >> frame;
        cap_o >> original;
    }
    
    // Smoothing by means of B-Spline Method:
    //Smoothing Curvature:
    gsl_vector *spline_smooth;
    spline_smooth = gsl_vector_alloc(spline_size);
    
    SmoothingCurvature(mm_pixel, spline_size, spline_x, spline_z, write_spline_fitpara, write_sec_deriv, write_transition, total_frames, spline_smooth);
    
    //Smoothing Amplitude:
    gsl_vector *spline_amplitude;
    spline_amplitude = gsl_vector_alloc(spline_size);
    SmoothingAmplitude(spline_size, spline_x, frame_amplitude, write_amplitude_fitpara, total_frames, spline_amplitude);
    
    for(int i = 0; i < spline_size; ++i){
        write_smooth_curv << i+1 << "\t" <<  (double) i / spline_size  << "\t" << gsl_vector_get (spline_smooth, i) / mm_pixel << "\t";
        write_smooth_curv << gsl_vector_get (spline_amplitude, i) * mm_pixel << endl;
    }
    
    gsl_vector_free(spline_x);
    gsl_vector_free(spline_smooth);
    gsl_vector_free(spline_amplitude);
    
    write_statis.close();
    write_smooth_curv.close();
    return 0;
}

//****************************************************
//Calculating Mean curvature of each frame: curvature = 1 / <|second_derivative|>
double FrameMeanCurvature(double *z_data,int data_size,double * second_derivative, double * first_derivative, double * local_curvature){//Simple definition of numerical second derivative.
    double dummy;
    double mean_curvature;
    for (int j = 1; j < data_size-1; ++j){
        dummy = z_data[j-1] - 2 * z_data[j] + z_data[j+1];// mm_pixel for z_data removes one of  mm_pixel^2 in the definition of second derivative.
        second_derivative[j-1] = abs(dummy);
        dummy = (z_data[j-1] - z_data[j+1]) / 2.0;//mm_pixel of z_data removes mm_pixel in th definition of first derivative.
        first_derivative[j-1] = abs(dummy);
        dummy = sqrt(pow(((first_derivative[j-1]*first_derivative[j-1])+1),3.0));//1/((((dz/dx)^2+1)^3)^{1/2})
        local_curvature[j-1] = second_derivative[j-1] / dummy;
    }
    
    mean_curvature = gsl_stats_mean(local_curvature,1,(data_size-2));
    return mean_curvature;
    delete []second_derivative;
    delete []first_derivative;
    delete []local_curvature;
}

//Smoothing by means of B-Spline:
void SmoothingCurvature(double mm_pixel, size_t spline_size, gsl_vector *spline_x, gsl_vector *spline_z, ofstream& write_spline_fitpara, ofstream& write_sec_deriv, ofstream& write_transition, int total_frames, gsl_vector *spline_smooth){
    //B-Spline variables:
    double spline_xdummy, spline_zdummy;
    size_t spline_i, spline_j;
    const int spline_order = 4;
    const size_t spline_breaks = 2;
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
    gsl_multifit_linear(multifit_X, spline_z, multifit_c, multifit_cov, & multifit_chisq, multifit_workspace);
    multifit_dof = spline_size - spline_coeffs;//Degrees of freedom
    multifit_tss = gsl_stats_tss(spline_z->data, 1, spline_z->size);//Totsl sum of squares
    multifit_Rsq = 1.0 - multifit_chisq / multifit_tss;
    write_spline_fitpara << "Standard error = " << stderr << endl << "chisq/dof = " <<  multifit_chisq / multifit_dof << endl;//stderr is a variable from Error Handling header file.
    write_spline_fitpara << "Rsq = "  << multifit_Rsq << endl;
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
        write_sec_deriv << j << (double) j/(spline_size-1) << curv_sec_deri[j-1] / pow(mm_pixel,3.0) << endl;
    }
    write_sec_deriv.close();
    size_t max_index;
    max_index = gsl_stats_max_index (curv_sec_deri, 1,spline_size-2);
    cout << "Index of Maximum value of second derivative (Based on array index rule):(transition frame number in curvature data = this number + 2)\t" << max_index << endl;
    write_transition << "(1):Index of Maximum value of second derivative (Based on array index rule)\t" << "(2):Coresponding frame number" << endl;
    write_transition << max_index << max_index+2;//
    write_transition.close();
    
    //Free memory:
    gsl_bspline_free(spline_workspace);
    gsl_vector_free(spline_basis);
    gsl_vector_free(spline_z);
    gsl_matrix_free(multifit_X);
    gsl_vector_free(multifit_c);
    gsl_matrix_free(multifit_cov);
    gsl_multifit_linear_free(multifit_workspace);
    gsl_vector_free(smoothed_curvature);
    delete []curv_sec_deri;
}

void SmoothingAmplitude(size_t spline_size, gsl_vector *spline_x, gsl_vector *frame_amplitude, ofstream& write_amplitude_fitpara, int total_frames, gsl_vector *spline_amplitude){
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
    write_amplitude_fitpara << "Standard error = " << stderr << endl << "chisq/dof = " <<  multifit_chisq / multifit_dof << endl;//stderr is a variable from Error Handling header file.
    write_amplitude_fitpara << "Rsq = "  << multifit_Rsq << endl;
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

