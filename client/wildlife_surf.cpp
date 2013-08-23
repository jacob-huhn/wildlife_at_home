#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
//
//#include <opencv2/core/types_c.h>
//#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <stats.hpp>

#ifdef _BOINC_APP_
#ifdef _WIN32
#include "boinc_win.h"
#include "str_util.h"
#endif

#include "diagnostics.h"
#include "util.h"
#include "filesys.h"
#include "boinc_api.h"
#include "mfile.h"
#endif

using namespace std;
using namespace cv;

//#define GUI

void write_checkpoint();
bool read_checkpoint();
int skipNFrames(CvCapture* capture, int n);
void printUsage();

int minHessian = 300;
Scalar color;
int currentFrame = 0;
vector<double> percentages;
string checkpoint_filename;

string vidFileName;
string checkpointVidFileName;
string featFileName;
string checkpointFeatFileName;

int main(int argc, char **argv) {
    if (argc != 3) {
        printUsage();
        return -1;
    }

    Rect finalRect;
    vector<Rect> boundingRects;
    vector<Point2f> tlPoints;
    vector<Point2f> brPoints;

    vidFileName = string(argv[1]);
    featFileName = string(argv[2]);

    // Get BOINC resolved file paths.
#ifdef _BOINC_APP_
    string resolved_vid_path;
    string resolved_feat_path;
    int retval = boinc_resolve_filename_s(vidFileName.c_str(), resolved_vid_path);
    if (retval) {
        cerr << "Error, could not open file: '" << vidFileName << "'" << endl;
        cerr << "Resolved to: '" << resolved_vid_path << "'" << endl;
        return false;
    }
    vidFileName = resolved_vid_path;

    retval = boinc_resolve_filename_s(featFileName.c_str(), resolved_feat_path);
    if (retval) {
        cerr << "Error, could not open file: '" << featFileName << "'" << endl;
        cerr << "Resolved to : '" << resolved_feat_path << "'" << endl;
        return false;
    }
    featFileName = resolved_feat_path;
#endif

    CvCapture *capture = cvCaptureFromFile(vidFileName.c_str());

    Mat descriptors_file;
    FileStorage infile(featFileName, FileStorage::READ);
    if (infile.isOpened()) {
        read(infile["Descriptors"], descriptors_file);
        infile.release();
    } else {
        cout << "Feature file " << featFileName << " does not exist." << endl;
        exit(-1);
    }
    cout << "Descriptors: " << descriptors_file.size() << endl;

    int framePos = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);
    int total = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);

    double fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
    int framesInThreeMin = (int)fps*180;

#ifdef _BOINC_APP_  
    boinc_init();
#endif

    cerr << "Video File Name: " << vidFileName << endl;
    cerr << "Feature File Name: " << featFileName << endl; 
    cerr << "Frames Per Second: " << fps << endl;
    cerr << "Frame Count: " << total << endl;
    cerr << "Number of Frames in Three Minutes: " << framesInThreeMin << endl;

    checkpoint_filename = "checkpoint.txt";

    if(read_checkpoint()) {
        if(checkpointVidFileName.compare(vidFileName)!=0 || checkpointFeatFileName.compare(featFileName)!=0) {
            cerr << "Checkpointed video or feature filename was not the same as given video or feature filename... Restarting" << endl;
        } else {
            cerr << "Continuing from checkpoint..." << endl;
        }
    } else {
        cerr << "Unsuccessful checkpoint read" << endl << "Starting from beginning of video" << endl;
    }

    skipNFrames(capture, percentages.size() * framesInThreeMin);
    framePos = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);
    cerr << "Starting at Frame: " << framePos << endl;

    while ((double)framePos/total < 1.0) {
        //cout << framePos/total << endl;
        Mat frame(cvarrToMat(cvQueryFrame(capture)));
        framePos = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES);

        SurfFeatureDetector detector(minHessian);

        vector<KeyPoint> keypoints_frame;

        detector.detect(frame, keypoints_frame);

        SurfDescriptorExtractor extractor;

        Mat descriptors_frame;

        extractor.compute(frame, keypoints_frame, descriptors_frame);

        // Find Matches
        FlannBasedMatcher matcher;
        vector<DMatch> matches;
        matcher.match(descriptors_frame, descriptors_file, matches);

        double max_dist = 0;
        double min_dist = 100;
        double avg_dist = 0;

        for (int i=0; i<matches.size(); i++) {
            double dist = matches[i].distance;
            if(dist < min_dist) min_dist = dist;
            if(dist > max_dist) max_dist = dist;
        }

        //cout << "Max dist: " << max_dist << endl;
        cout << "Min dist: " << min_dist << endl;

        vector<DMatch> good_matches;

        for (int i=0; i<matches.size(); i++) {
            if (matches[i].distance <= 0.18 && matches[i].distance <= 2.0*min_dist) {
                good_matches.push_back(matches[i]);
                avg_dist += matches[i].distance;
            }
        }
        if (good_matches.size() > 0) {
        	avg_dist = avg_dist/good_matches.size();
        	cout << "Avg dist: " << avg_dist << endl;
        }

		// Localize object.
		vector<Point2f> matching_points;
		vector<KeyPoint> keypoints_matches;

		for (int i=0; i<good_matches.size(); i++) {
			keypoints_matches.push_back(keypoints_frame[good_matches[i].queryIdx]);
			matching_points.push_back(keypoints_frame[good_matches[i].queryIdx].pt);
		}

		// Code to draw the points.
		Mat frame_points;
#ifdef GUI
        drawKeypoints(frame, keypoints_matches, frame_points, Scalar::all(-1), DrawMatchesFlags::DEFAULT);
#endif

		//Get bounding rectangle.
		if (matching_points.size() == 0) {
			Point2f tlFrame(0, 0);
			Point2f brFrame(frame.cols, frame.rows);
			tlPoints.push_back(tlFrame);
			brPoints.push_back(brFrame);
		} else {
			Rect boundRect = boundingRect(matching_points);
			
			//Calculate mean.
			Mat mean;
			reduce(matching_points, mean, CV_REDUCE_AVG, 1);
			double xMean = mean.at<float>(0,0);
			double yMean = mean.at<float>(0,1);
			
			//Calculate standard deviation.
			vector<int> xVals;
			vector<int> yVals;
			for (int i=0; i<matching_points.size(); i++) {
				xVals.push_back(matching_points[i].x);
				yVals.push_back(matching_points[i].y);
			}
			double xStdDev = standardDeviation(xVals, xMean);
			double yStdDev = standardDeviation(yVals, yMean);
			
			Point2f tlStdPoint(xMean-xStdDev/2, yMean+yStdDev/2);
			Point2f brStdPoint(xMean+xStdDev/2, yMean-yStdDev/2);
			
			Rect stdDevRect(tlStdPoint, brStdPoint);

#ifdef GUI
			color = Scalar(0, 0, 255); // Blue, Green, Red
			rectangle(frame_points, boundRect.tl(), boundRect.br(), color, 2, 8, 0);
			color = Scalar(0, 255, 255); // Blue, Green, Red
			rectangle(frame_points, stdDevRect.tl(), stdDevRect.br(), color, 2, 8, 0);
#endif
			boundingRects.push_back(boundRect);
			tlPoints.push_back(boundRect.tl());
			brPoints.push_back(boundRect.br());
		}

		if (tlPoints.size() != 0) {
			
			// Calculate mean rectangle.
			Mat tlMean;
			Mat brMean;
			reduce(tlPoints, tlMean, CV_REDUCE_AVG, 1);
			reduce(brPoints, brMean, CV_REDUCE_AVG, 1);
			Point2f tlPoint(tlMean.at<float>(0,0), tlMean.at<float>(0,1));
			Point2f brPoint(brMean.at<float>(0,0), brMean.at<float>(0,1));
			
			Rect averageRect(tlPoint, brPoint);
		
			// Calculate median rectangle.
			vector<int> tlxVals;
			vector<int> tlyVals;
			vector<int> brxVals;
			vector<int> bryVals;
			for (int i=0; i<tlPoints.size(); i++) {
				tlxVals.push_back(tlPoints[i].x);
				tlyVals.push_back(tlPoints[i].y);
				brxVals.push_back(brPoints[i].x);
				bryVals.push_back(brPoints[i].y);
			}
			int tlxMedian;
			int tlyMedian;
			int brxMedian;
			int bryMedian;
			tlxMedian = quickMedian(tlxVals);
			tlyMedian = quickMedian(tlyVals);
			brxMedian = quickMedian(brxVals);
			bryMedian = quickMedian(bryVals);
			//cout << "lt Median: " << tlxMedian << "," << tlyMedian << endl;
			//cout << "br Median: " << brxMedian << "," << bryMedian << endl;
			Point2i tlMedianPoint(tlxMedian, tlyMedian);
			Point2i brMedianPoint(brxMedian, bryMedian);

			Rect medianRect(tlMedianPoint, brMedianPoint);			

#ifdef GUI
			color = Scalar(255, 0, 0); // Blue, Green, Red
			rectangle(frame_points, averageRect.tl(), averageRect.br(), color, 2, 8, 0);
			color = Scalar(0, 255, 0);
			rectangle(frame_points, medianRect.tl(), medianRect.br(), color, 2, 8, 0);
#endif

<<<<<<< HEAD
                finalRect = averageRect;
            }

            // Check for 1800 frame mark.
            if (framePos != 0 && framePos % 1800 == 0.0) {
                double frameDiameter = sqrt(pow((double)frame.cols, 2) * pow((double)frame.rows, 2));
                double roiDiameter = sqrt(pow((double)finalRect.width, 2) * pow((double)finalRect.height, 2));
                double probability = 1-(roiDiameter/frameDiameter);

                percentages.push_back(probability);
#ifndef _BOINC_APP_
                cerr << framePos << " -- " << probability << endl;
=======
			finalRect = averageRect;
		}

		// Check for frames in three minutes mark.
		framesInThreeMin = 20;
		if (framePos != 0 && framePos % framesInThreeMin == 0.0) {
			double probability;
			if (tlPoints.empty() && brPoints.empty()) {
				probability = 0.0;
				
			} else {
				double frameDiameter = sqrt(pow((double)frame.cols, 2) * pow((double)frame.rows, 2));
				double roiDiameter = sqrt(pow((double)finalRect.width, 2) * pow((double)finalRect.height, 2));
				probability = 1-(roiDiameter/(frameDiameter*0.6));
			}
			if (probability < 0) probability = 0.0;
			percentages.push_back(probability);
#ifndef _BOINC_APP_
			cout << "Min Dist: " << min_dist << endl;
            cout << probability << endl;
>>>>>>> 0cdb87747e90d68f12c0956b3be43790f442d002
#endif
			boundingRects.clear();
			tlPoints.clear();
			brPoints.clear();
		}

		// Update percent completion and look for checkpointing request.
#ifdef _BOINC_APP_
		boinc_fraction_done((double)framePos/total);

<<<<<<< HEAD
            if(boinc_time_to_checkpoint()) {
                cerr << "checkpointing" << endl;
                write_checkpoint();
                boinc_checkpoint_completed();
            }
=======
		if(boinc_time_to_checkpoint() || key == 's') {
			cerr << "checkpointing" << endl;
			write_checkpoint();
			boinc_checkpoint_completed();
		}
>>>>>>> 0cdb87747e90d68f12c0956b3be43790f442d002
#endif

#ifdef GUI
		imshow("SURF", frame_points);
		if(cvWaitKey(15)==27) break;
#endif
	}

	cerr << "<slice_probabilities>" << endl;
	for (int i=0; i<percentages.size(); i++) cerr << percentages[i] << endl;
	cerr << "</slice_probabilities>" << endl;

#ifdef GUI
    cvDestroyWindow("SURF");
#endif

    cvReleaseCapture(&capture);

#ifdef _BOINC_APP_
    boinc_finish(0);
#endif
    return 0;
}

void write_checkpoint() {
	#ifdef _BOINC_APP_
    string resolved_path;
    int retval = boinc_resolve_filename_s(checkpoint_filename.c_str(), resolved_path);
    if (retval) {
        cerr << "Couldn't resolve file name..." << endl;
        return;
    }
    checkpoint_filename = resolved_path;
    #endif

    ofstream checkpoint_file(checkpoint_filename.c_str());

    if (!checkpoint_file.is_open()) {
        cerr << "Checkpoint file not open..." << endl;
        return;
    }

    checkpoint_file << "VIDEO_FILE_NAME: " << vidFileName << endl;
    checkpoint_file << "FEAT_FILE_NAME: " << featFileName << endl;
    checkpoint_file << "PROBABILITIES: " << percentages.size() << endl;

    for (int i=0; i<percentages.size(); i++) {
        checkpoint_file << percentages[i] << endl;
    }
    checkpoint_file << endl;

    checkpoint_file.close();
}

bool read_checkpoint() {
	#ifdef _BOINC_APP_
    string resolved_path;
    int retval = boinc_resolve_filename_s(checkpoint_filename.c_str(), resolved_path);
    if (retval) {
        return false;
    }
    checkpoint_filename = resolved_path;
    #endif

    ifstream checkpoint_file(checkpoint_filename.c_str());
    if (!checkpoint_file.is_open()) return false;

    string s;
    checkpoint_file >> s >> checkpointVidFileName;
    if (s.compare("VIDEO_FILE_NAME:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'VIDEO_FILE_NAME'" << endl;

		#ifdef _BOINC_APP_
        boinc_finish(1);
		#endif
        exit(1);
    }
    
    checkpoint_file >> s >> checkpointFeatFileName;
    if (s.compare("FEAT_FILE_NAME:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'FEAT_FILE_NAME'" << endl;

		#ifdef _BOINC_APP_
        boinc_finish(1);
		#endif
        exit(1);
    }

    int intervals;
    checkpoint_file >> s >> intervals;
    if (s.compare("PROBABILITIES:") != 0) {
        cerr << "ERROR: malformed checkpoint! could not read 'INTERVALS'" << endl;

		#ifdef _BOINC_APP_
        boinc_finish(1);
		#endif
        exit(1);
    }

    float current;
    for(int i=0; i<intervals; i++) {
        checkpoint_file >> current;
        percentages.push_back(current);
        if (!checkpoint_file.good()) {
            cerr << "ERROR: malformed checkpoint! not enough probabilities present" << endl;

			#ifdef _BOINC_APP_
            boinc_finish(1);
			#endif
            exit(1);
        }
    }

    return true;
}

int skipNFrames(CvCapture* capture, int n) {
    for(int i = 0; i < n; ++i) {
        if(cvQueryFrame(capture) == NULL) {
        	return i+1;
        }
    }
	return n;
}

void printUsage() {
	cout << "Usage: cv_surf <vid> <feats>" << endl;
}
