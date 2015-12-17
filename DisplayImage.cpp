#include <stdio.h>
#include <iostream>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"

using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc,
         char **argv) {

    Mat img_scene;
    Mat img_object = imread("frida.jpg", CV_LOAD_IMAGE_COLOR);
    VideoCapture cap(0); // Open the default camera.
    if(!cap.isOpened())  /* check if we succeeded. */ {
        return -1;
    }

    while(true) {

        Mat frame;
        cap >> img_scene; // Get a new frame from camera.
        //cvtColor(frame, img_scene, CV_BGR2GRAY);
        if(!img_object.data || !img_scene.data) {
            std::cout<< " --(!) Error reading images " << std::endl;

            return -1;
        }

        // -- Step 1 : Detect the keypoints using SURF Detector.
        int minHessian = 400;

        Ptr<SURF> detector = SURF::create( minHessian );

        std::vector<KeyPoint> keypoints_object, keypoints_scene;

        detector->detect(img_object, keypoints_object);
        detector->detect(img_scene, keypoints_scene);

        //-- Step 2 : Calculate descriptors (feature vectors).
        //SurfDescriptorExtractor extractor;

        Mat descriptors_object, descriptors_scene;

        detector->compute(img_object, keypoints_object, descriptors_object);
        detector->compute(img_scene, keypoints_scene, descriptors_scene);

        bool isEmpty = descriptors_scene.empty();

        FlannBasedMatcher matcher;
        std::vector<std::vector< DMatch > > matches;
        std::vector< DMatch > good_matches;
        if (!isEmpty) {

            //-- Step 3 : Matching descriptor vectors using FLANN matcher.
            matcher.knnMatch(descriptors_object, descriptors_scene, matches, 2);

            for(int i = 0; i < descriptors_object.rows; i++) {
                if(matches[i][0].distance <0.6* matches[i][1].distance) {
                    good_matches.push_back(matches[i][0]); }
            }
        }

        Mat img_matches;
        drawMatches(img_object, keypoints_object, img_scene, keypoints_scene,
                    good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
                    std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

        //-- Localize the object from img_1 in img_2
        std::vector<Point2f> obj;
        std::vector<Point2f> scene;

        for(size_t i = 0; i < good_matches.size(); i++) {

            //-- Get the keypoints from the good matches
            obj.push_back(keypoints_object[ good_matches[i].queryIdx ].pt);
            scene.push_back(keypoints_scene[ good_matches[i].trainIdx ].pt);
        }

        if(obj.size() > 12) {

          Mat H = findHomography( obj, scene, CV_RANSAC );

          //-- Get the corners from the image_1 ( the object to be "detected" )
          std::vector<Point2f> obj_corners(4);
          obj_corners[0] = cvPoint(0,0); obj_corners[1] = cvPoint( img_object.cols, 0 );
          obj_corners[2] = cvPoint( img_object.cols, img_object.rows ); obj_corners[3] = cvPoint( 0, img_object.rows );
          std::vector<Point2f> scene_corners(4);

          perspectiveTransform( obj_corners, scene_corners, H);

            //-- Draw lines between the corners (the mapped object in the scene - image_2)
            Point2f offset((float)img_object.cols, 0);
            line(img_matches, scene_corners[0] + offset, scene_corners[1] + offset, Scalar(0, 255, 0), 4);
            line(img_matches, scene_corners[1] + offset, scene_corners[2] + offset, Scalar(0, 255, 0), 4);
            line(img_matches, scene_corners[2] + offset, scene_corners[3] + offset, Scalar(0, 255, 0), 4);
            line(img_matches, scene_corners[3] + offset, scene_corners[0] + offset, Scalar(0, 255, 0), 4);


        }
        //-- Show detected matches
        imshow("Frida Kahlo Tracker", img_matches);
        waitKey(1);
    }

    return 0;
}
