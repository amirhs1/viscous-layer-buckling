#include <iostream>
#include <string>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>

// Absolute path to the marker templates (<MSC_DATA_DIR>/marker-templates), injected by
// CMake so the binary finds them regardless of the working directory it is run from.
#ifndef TEMPLATE_DIR
#error "TEMPLATE_DIR is not defined: build with CMake and -DMSC_DATA_DIR=/path/to/data"
#endif

using namespace std;
using namespace cv;

int main (int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: background-template-prototype <video-or-image-sequence>" << endl;
    return 1;
  }
  VideoCapture cap {argv[1]};
  const string template_dir {TEMPLATE_DIR};
  Mat frame, left_templ=imread (template_dir + "/left.jpg"), right_templ=imread (template_dir + "/right.jpg");
  cap >> frame;
  cout << frame.size() << endl;
  bool first{true};
  int initial_width, initial_height;
  Mat initial, previous, diff, diff_resized, fg;
  Ptr<BackgroundSubtractorMOG2> bgsubtractor=createBackgroundSubtractorMOG2();
  bgsubtractor->setVarThreshold(10);

  while (!frame.empty()) {
    Mat pattern;
    Point left, right;
    matchTemplate (frame, left_templ, pattern, TM_SQDIFF_NORMED);
    minMaxLoc (pattern, nullptr, nullptr, &left, nullptr);
    matchTemplate (frame, right_templ, pattern, TM_SQDIFF_NORMED);
    minMaxLoc (pattern, nullptr, nullptr, &right, nullptr);
    left.x += left_templ.cols;
    Mat resized, cropped = Mat {frame, Range::all(), Range{left.x, right.x}};
    if (first) {
      initial_width = cropped.cols;
      initial_height = cropped.rows;
      initial = cropped.clone();
      previous = cropped.clone();
      first = false;
    } else {
      Size current {cropped.size()};
      Size size {initial_width, initial_height};
      resize (cropped, resized, size);
      absdiff (resized, previous, diff);
      previous = resized;
      resize (diff, diff, current);
      cvtColor (diff, diff, COLOR_BGR2GRAY);
      threshold (diff, diff, 0, 255, THRESH_TRIANGLE);
      imshow ("diff", diff);
    }
    imshow ("", cropped);
    waitKey (30);
    cap >> frame;
  }
  return 0;
}
