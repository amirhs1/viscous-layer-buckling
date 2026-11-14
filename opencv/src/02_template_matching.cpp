#include <cstdlib>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>
#include <unistd.h>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_errno.h>

// Folder that stage 2 appends its per-run regression row in: the CMake build
// folder, injected by CMake. It is never the input data folder.
#ifndef OUTPUT_DIR
#define OUTPUT_DIR "."
#endif

using namespace std;
using namespace cv;

int main () {//Program takes original image sequences path as input (argv[])
    
    //freopen("expriment-inputs.txt","r",stdin);
    
    //Taking input datas like:
    string experiment_folder;
    cout << "Enter Experiment Folder: ";
    cin >> experiment_folder;
    cout << experiment_folder << endl;
    
    string experiment_name;
    cout << "Enter Experiment Name: ";
    cin >> experiment_name;
    cout << experiment_name << endl;
    
    
    string template_address;
    string save_address;
    string input_address;
    template_address = experiment_folder;
    save_address = experiment_folder;
    input_address = experiment_folder;
    input_address.append("/");
    input_address.append(experiment_name);
    input_address.append("-1-Renumbered/");
    input_address.append(experiment_name);
    input_address.append("-Renumbered-%04d.tif");
    double length_initial;
    cout << "Enter Initial Length: ";
    cin >> length_initial;
    cout << length_initial << endl;
    
    double frequency;
    cout << "Enter Frequency of Motor: ";
    cin >> frequency;
    cout << frequency << endl;
    
    double frame_second;
    cout << "Enter FPS: ";
    cin >> frame_second;
    cout << frame_second << endl;
    double time_interval = 1/frame_second;
    
    double pixel_mm;
    cout << "Enter Pixels per mm ratio: ";
    cin >> pixel_mm;
    cout << pixel_mm << endl;
    double mm_pixel = 1/pixel_mm;
  

    
    experiment_folder.append("/");
    experiment_folder.append(experiment_name);
    experiment_folder.append("-2-TemMat");
    
    boost::filesystem::create_directory(experiment_folder);//Create directory (folder) for saving tamplate matchted image sequences:
    
    //Reading image sequences:
//    if (argc == 1) return 1;
//    string arg = argv[1];
    VideoCapture cap {input_address};
    if (!cap.isOpened())
    {
        cerr << "Failed to open Image Sequence!\n" << endl;
        return 1;
    }
    Mat frame;
    cap >> frame;
    
    //Reading Templates:
    string left_address;//Right template
    left_address = template_address;
    left_address.append("/green.tif");
    Mat left_templ = imread (left_address);
    string right_address;//Left template
    right_address = template_address;
    right_address.append("/red.tif");
    Mat right_templ = imread (right_address);
    
    
    
    string input_dummy;
    input_dummy.append("/");
    input_dummy.append(experiment_name);
    input_dummy.append("-TemMat-%04d.tif");
    experiment_folder.append(input_dummy);
    const char * path = experiment_folder.c_str();//Path for writing output frames and data:
    char filename[250];
    string template_address2 = template_address;
    input_dummy = experiment_name;
    input_dummy.append("-TemplateMatchingData.txt");
    template_address.append("/");
    template_address.append(input_dummy);
    ofstream write_data (template_address);
    write_data << "Frame\t Time(s)\t Left side(Pixel)\t Left Position(mm)\t Right side(pixel)\t Right Position(mm)\t Length(pixel)\t Layer Length(mm)" << endl;
    input_dummy = experiment_name;
    input_dummy.append("-Velovity-Log.txt");
    template_address2.append("/");
    template_address2.append(input_dummy);
    ofstream write_fit (template_address2);
    write_fit << "#Linear Regression: a*x + b" << endl;
    write_fit << "Velocity(mm/s)\t\t"  << "Velocity error(mm/s)\t\t" << "Sum Square \t\t" << "Estimated initial length \t\t"  << " Estimated initial length error(mm/s)\t\t" << endl;
    ofstream write_experiment (string(OUTPUT_DIR) + "/linear_regression_data.txt", ios::app);
    if (write_experiment.seekp(0, ios_base::beg).tellp() == write_experiment.seekp(0, ios_base::end).tellp())
    {
    write_experiment << "#1:Experiment Name\t\t" << "2:Experiment Path\t\t" << "3:Initial Length\t\t" <<"4:Frame per seconds\t\t" << "5:Milimeters per pixel\t\t" << "6:Frequency (Hz)\t\t" << "7:Velocity(mm/s)\t\t" << "8:Velocity Error(mm/s)\t\t" << endl;
    }
//    ofstream write_wavenumber ("wavenumber.txt");
//    write_wavenumber << "Frame Number\t" << "Normalized frame number\t" << "Frame maximum wavenumbr (1/mm)" << endl;
    
    
    //Defining Linear Regression Parameters: y = a*x + b
    vector<double> t_data;//Frame Data
    vector<double> l_data;//Length Data
    const int stride = 1;
    int data_size;
    double velocity,l_initial;//Stimated compression velocity and intial length of layer.
    //Named after gsl_fit_linear's own outputs: cov00 is the variance of the
    //intercept (l_initial), cov11 the variance of the slope (velocity), and
    //cov01 their covariance. Standard errors are the square roots of the two
    //variances; cov01 is not an error and is negative whenever time is
    //non-negative, as it is here.
    double cov00 = 0, cov01 = 0, cov11 = 0, sum_square = 0;
    int frame_index=0;
    
    //Template Matching:
    while (!frame.empty()) {
        frame_index++;
        Mat pattern;
        Point left, right;
        //Left side
        matchTemplate (frame, left_templ, pattern, TM_SQDIFF_NORMED);//Compares a template against overlapped image regions.
        minMaxLoc (pattern, nullptr, nullptr, &left, nullptr);//The best matches can be found as global minimums (when TM_SQDIFF was used).
        
        //Right side
        matchTemplate (frame, right_templ, pattern, TM_SQDIFF_NORMED);
        minMaxLoc (pattern, nullptr, nullptr, &right, nullptr);
        
        //Removing extra regions by making them black.
        left.x += left_templ.cols;
        rectangle (frame, Point(0,0), Point(left.x, frame.rows), Scalar::all(0), -1);//Making left side black by means of a black colored rectangle
        rectangle (frame, Point(right.x, 0), Point(frame.cols, frame.rows), Scalar::all(0), -1);//Right side ...
        imshow (experiment_name, frame);
        waitKey (30);//Wait in miliseconds for next frame.
        
        // Writing Data for Linear Regression:
        t_data.push_back (((frame_index-1)*time_interval));//Frame number
        l_data.push_back (((right.x-left.x)*mm_pixel));//Length of layer
        
        //Writing Length Data and template matched images:
        write_data << frame_index << "\t " << (frame_index-1) * time_interval << "\t " << left.x << "\t " << left.x * mm_pixel << "\t " << right.x ;
        write_data << "\t " << right.x * mm_pixel << "\t " << (right.x-left.x) << "\t " << (right.x-left.x) * mm_pixel << endl;
        sprintf (filename, path, frame_index);
        imwrite (filename, frame);
        
        cap >> frame;
    }
    
    
    //Calculating Linear Regression:
    data_size = t_data.size();
    int status_func;
    double * length_data = new double[data_size];
    double * time_data = new double[data_size];
    
    for (int i = 0; i < data_size; i++) {
        time_data[i] = t_data[i];
        length_data[i] = l_data[i];
    }
    status_func = gsl_fit_linear (time_data, stride, length_data, stride, data_size, &l_initial, &velocity, &cov00, &cov01, &cov11, &sum_square);

    if(status_func){/* an error occurred */
        printf ("error: %s\n", gsl_strerror (status_func));
    }
    double velocity_error = sqrt(cov11);
    double l_initial_error = sqrt(cov00);
    write_experiment << experiment_name << "\t" << save_address << "\t" << length_initial << "\t" << frame_second << "\t" << mm_pixel << "\t" << frequency << "\t" << velocity << "\t" << velocity_error << endl;
    write_fit << velocity << "\t" << velocity_error << "\t" << sum_square << "\t" << l_initial << "\t" << l_initial_error << "\t" <<endl;
    
    // Theoretical most unstable wavenumber for wrinkling (undulating) state:
//    double heigth_0 = 1.99; //in milimeters.
//    double tau = 139;// Viscous-Hydrodynamical (Gravity) charateristic time (s).
//    double tau_1 = 5759988;// Viscous-Tentional charateristic tim (s).
//    double bond_number = tau_1/tau;
//    double * strain_rate = new double[data_size];// in s^-1.
//    double * control_para = new double[data_size];//Control parameter.
//    double * k_wrinkling = new double[data_size];//Maximum wavenmuber.
//    for (int i = 0; i < data_size; i++) {
//        strain_rate[i] = velocity / length_data[i];
//        control_para[i] = -2 * tau * strain_rate[i] * (1 / (2 * bond_number));
//        k_wrinkling[i] = 1/(heigth_0 * sqrt(control_para[i]));
//        write_wavenumber << i+1 << "\t" << (double) (i+1) / data_size << "\t" << k_wrinkling[i] << endl;
//    }
    
    // Theoretical maximum wavenumber for coupled state:
//    double *height_t = new double[data_size];
    
    
//    delete []time_data;
//    delete []length_data;
//    delete []strain_rate;
//    delete []control_para;
//    delete []k_wrinkling;
//    delete []k_coupled;
//    write_wavenumber.close();
    write_data.close();
    write_fit.close();
    return 0;
}
