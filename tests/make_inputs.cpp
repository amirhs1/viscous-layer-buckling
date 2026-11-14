// Generate the synthetic inputs of the test suite with OpenCV; no real data are needed.
// The folders must exist (run_tests.sh makes them); this program only writes the files.
//
//   make-inputs markers <dir>
//       Two 32x32 marker templates, <dir>/left.jpg (a cross) and <dir>/right.jpg (a ring).
//   make-inputs stage12 <experiment_dir> <marker_dir>
//       Stage 2's templates green.tif and red.tif, and 40 frames in
//       <experiment_dir>/<name>-1-Renumbered/ in which the markers move 2 px per frame toward
//       each other (<name> is the last part of <experiment_dir>).
//   make-inputs stage34 <experiment_dir> growing|oscillating|blank|colour
//       A binary interface y = 100 + A sin(2 pi x / 100), rounded to whole pixels, in
//       <name>-4-Thershold/ (white layer above, black below), the matching originals in
//       <name>-3-Resized/, and expected.txt: per frame, the amplitude (max - min of y) and the
//       sample variance of y, in pixels.
//         growing:     A = 2 + i in frame i, 30 frames
//         oscillating: A = 6 + 3 sin(2 pi (i + 0.3) / 24), a period of 24 frames, 96 frames
//                      (the 0.3 phase keeps the smoothed variance from having a zero derivative)
//         blank:       as growing, 3 frames, but frame 2 is all black (no contour)
//         colour:      as growing, but the thresholded frames are stored as 3-channel images,
//                      which is how OpenCV 4.6 returns them whatever the stage asks for
#include <opencv2/opencv.hpp>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using std::string;

static const double PI = 3.14159265358979323846;

static string basename_of (const string &path) {
    string p = path;
    while (!p.empty() && p.back() == '/') p.pop_back();
    return p.substr(p.find_last_of('/') + 1);
}

static bool write (const string &file, const Mat &image) {
    if (imwrite(file, image)) return true;
    std::cerr << "cannot write " << file << " (does its folder exist?)" << std::endl;
    return false;
}

static int markers (const string &dir) {
    Mat left(32, 32, CV_8UC3, Scalar(255, 255, 255)), right = left.clone();
    rectangle(left, Point(13, 4), Point(18, 27), Scalar(0, 0, 0), FILLED);
    rectangle(left, Point(4, 13), Point(27, 18), Scalar(0, 0, 0), FILLED);
    circle(right, Point(16, 16), 11, Scalar(0, 0, 0), 5);
    return write(dir + "/left.jpg", left) && write(dir + "/right.jpg", right) ? 0 : 1;
}

static int stage12 (const string &exp, const string &marker_dir) {
    const string name = basename_of(exp);
    const int frames = 40, speed = 2, width = 640, height = 240, top = 100;
    Mat left = imread(marker_dir + "/left.jpg"), right = imread(marker_dir + "/right.jpg");
    if (left.empty() || right.empty()) { std::cerr << "no markers in " << marker_dir << std::endl; return 1; }
    if (!write(exp + "/green.tif", left) || !write(exp + "/red.tif", right)) return 1;
    const int left0 = 40, right0 = width - 40 - right.cols;
    char file[4096];
    for (int i = 0; i < frames; ++i) {
        Mat frame(height, width, CV_8UC3, Scalar(200, 200, 200));
        left.copyTo(frame(Rect(left0 + speed * i, top, left.cols, left.rows)));
        right.copyTo(frame(Rect(right0 - speed * i, top, right.cols, right.rows)));
        std::snprintf(file, sizeof file, "%s/%s-1-Renumbered/%s-Renumbered-%04d.tif", exp.c_str(), name.c_str(), name.c_str(), i + 1);
        if (!write(file, frame)) return 1;
    }
    return 0;
}

static int stage34 (const string &exp, const string &mode) {
    const string name = basename_of(exp);
    const int width = 400, height = 200, y0 = 100, wavelength = 100;
    int frames;
    if (mode == "growing" || mode == "colour") frames = 30;
    else if (mode == "oscillating") frames = 96;
    else if (mode == "blank") frames = 3;
    else { std::cerr << "unknown mode " << mode << std::endl; return 1; }
    FILE *expected = std::fopen((exp + "/expected.txt").c_str(), "w");
    if (!expected) { std::cerr << "cannot write " << exp << "/expected.txt" << std::endl; return 1; }
    std::fprintf(expected, "frame\tamplitude_px\tvariance_px2\n");
    char file[4096];
    for (int i = 0; i < frames; ++i) {
        const double a = mode == "oscillating" ? 6.0 + 3.0 * std::sin(2 * PI * (i + 0.3) / 24) : 2.0 + i;
        std::vector<long> y(width);
        for (int x = 0; x < width; ++x)
            y[x] = std::lrint(y0 + a * std::sin(2 * PI * x / wavelength));   // round half to even
        Mat binary(height, width, CV_8UC1, Scalar(0));
        if (!(mode == "blank" && i == 1))
            for (int x = 0; x < width; ++x)
                binary(Rect(x, 0, 1, y[x] + 1)).setTo(255);                // the layer, down to the interface
        Mat grey(height, width, CV_8UC1, Scalar(60)), original;           // 60 below the interface,
        grey.setTo(187, binary);                                           // 187 (= 255 / 2 + 60) above
        cvtColor(grey, original, COLOR_GRAY2BGR);
        std::snprintf(file, sizeof file, "%s/%s-4-Thershold/%s-Thershold-%04d.tif", exp.c_str(), name.c_str(), name.c_str(), i + 1);
        Mat stored = binary;
        if (mode == "colour") cvtColor(binary, stored, COLOR_GRAY2BGR);
        if (!write(file, stored)) return 1;
        std::snprintf(file, sizeof file, "%s/%s-3-Resized/%s-Resized-%04d.tif", exp.c_str(), name.c_str(), name.c_str(), i + 1);
        if (!write(file, original)) return 1;
        long lo = y[0], hi = y[0];
        double mean = 0;
        for (long v : y) { lo = std::min(lo, v); hi = std::max(hi, v); mean += v; }
        mean /= width;
        double ss = 0;
        for (long v : y) ss += (v - mean) * (v - mean);
        std::fprintf(expected, "%d\t%ld\t%.6f\n", i + 1, hi - lo, ss / (width - 1));
    }
    std::fclose(expected);
    return 0;
}

int main (int argc, char *argv[]) {
    const string cmd = argc > 1 ? argv[1] : "";
    if (cmd == "markers" && argc == 3) return markers(argv[2]);
    if (cmd == "stage12" && argc == 4) return stage12(argv[2], argv[3]);
    if (cmd == "stage34" && argc == 4) return stage34(argv[2], argv[3]);
    std::cerr << "usage: make-inputs markers <dir> | stage12 <experiment_dir> <marker_dir> | "
                 "stage34 <experiment_dir> growing|oscillating|blank|colour" << std::endl;
    return 2;
}
