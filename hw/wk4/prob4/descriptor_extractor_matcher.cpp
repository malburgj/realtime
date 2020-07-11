#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include <opencv2/xfeatures2d.hpp>

#include <iostream>

using namespace cv;
using namespace std;

void help(char** argv)
{
    cout << "\nThis program demonstrats keypoint finding and matching between 2 images using features2d framework.\n"
     << "If ransacReprojThreshold>=0 then homography matrix is calculated and used to filter matches\n"
     << "\n"
     << "Example of usage:\n"
     << "./descriptor_extractor_matcher SURF SURF BruteForce CrossCheckFilter cola1.jpg cola2.jpg 3\n"
     << "\n"
     << "Possible detectorType values: SIFT.\n"
     << "Possible descriptorType values: SIFT.\n"
     << "Possible matcherType values: see in documentation on createDescriptorMatcher().\n"
     << "Possible matcherFilterType values: NoneFilter, CrossCheckFilter." << endl;
}

#define DRAW_RICH_KEYPOINTS_MODE  (0)
#define DRAW_OUTLIERS_MODE        (0)

const string winName = "correspondences";

enum { NONE_FILTER = 0, CROSS_CHECK_FILTER = 1 };

int getMatcherFilterType( const string& str )
{
    if( str == "NoneFilter" )
        return NONE_FILTER;
    if( str == "CrossCheckFilter" )
        return CROSS_CHECK_FILTER;
    CV_Error(CV_StsBadArg, "Invalid filter name");
    return -1;
}

void simpleMatching( Ptr<DescriptorMatcher>& descriptorMatcher,
                     const Mat& descriptors1, const Mat& descriptors2,
                     vector<DMatch>& matches12)
{
    vector<DMatch> matches;
    descriptorMatcher->match( descriptors1, descriptors2, matches12 );
}

void crossCheckMatching( Ptr<DescriptorMatcher>& descriptorMatcher,
                         const Mat& descriptors1, const Mat& descriptors2,
                         vector<DMatch>& filteredMatches12, int knn=1 )
{
    filteredMatches12.clear();
    vector<vector<DMatch> > matches12, matches21;
    descriptorMatcher->knnMatch( descriptors1, descriptors2, matches12, knn );
    descriptorMatcher->knnMatch( descriptors2, descriptors1, matches21, knn );
    for( size_t m = 0; m < matches12.size(); m++ ) {
        bool findCrossCheck = false;
        for( size_t fk = 0; fk < matches12[m].size(); fk++ ) {
            DMatch forward = matches12[m][fk];

            for( size_t bk = 0; bk < matches21[forward.trainIdx].size(); bk++ ) {
                DMatch backward = matches21[forward.trainIdx][bk];
                if( backward.trainIdx == forward.queryIdx ) {
                    filteredMatches12.push_back(forward);
                    findCrossCheck = true;
                    break;
                }
            }
            if( findCrossCheck ) break;
        }
    }
}

void doIteration( const Mat& img1, Mat& img2,
                  vector<KeyPoint>& keypoints1, const Mat& descriptors1,
                  Ptr<FeatureDetector>& detector, Ptr<DescriptorExtractor>& descriptorExtractor,
                  Ptr<DescriptorMatcher>& descriptorMatcher, int matcherFilter, bool eval,
                  double ransacReprojThreshold)
{
    assert(!img1.empty());
    assert(!img2.empty());

    vector<KeyPoint> keypoints2;
    detector->detect( img2, keypoints2 );
    cout << "Total of " << keypoints2.size() << " keypoints extracted from second image" << endl;
    Mat descriptors2;
    descriptorExtractor->compute( img2, keypoints2, descriptors2 );

    vector<DMatch> filteredMatches;
    if(matcherFilter == CROSS_CHECK_FILTER) {
        crossCheckMatching(descriptorMatcher, descriptors1, descriptors2, filteredMatches, 1);
    } else {
        simpleMatching(descriptorMatcher, descriptors1, descriptors2, filteredMatches);
    }

    vector<int> queryIdxs(filteredMatches.size());
    vector<int> trainIdxs(filteredMatches.size());

    for(size_t i = 0; i < filteredMatches.size(); ++i) {
        queryIdxs[i] = filteredMatches[i].queryIdx;
        trainIdxs[i] = filteredMatches[i].trainIdx;
    }

    Mat H12;
    if(ransacReprojThreshold >= 0) {
        vector<Point2f> points1; KeyPoint::convert(keypoints1, points1, queryIdxs);
        vector<Point2f> points2; KeyPoint::convert(keypoints2, points2, trainIdxs);
        H12 = findHomography( Mat(points1), Mat(points2), CV_RANSAC, ransacReprojThreshold );
    }

    Mat drawImg;
    if(!H12.empty()) {// filter outliers
        vector<char> matchesMask(filteredMatches.size(), 0);
        vector<Point2f> points1; KeyPoint::convert(keypoints1, points1, queryIdxs);
        vector<Point2f> points2; KeyPoint::convert(keypoints2, points2, trainIdxs);
        Mat points1t; perspectiveTransform(Mat(points1), points1t, H12);

        double maxInlierDist = ransacReprojThreshold < 0 ? 3 : ransacReprojThreshold;
        for( size_t i1 = 0; i1 < points1.size(); ++i1) {
            if(norm(points2[i1] - points1t.at<Point2f>((int)i1,0)) <= maxInlierDist) // inlier
                matchesMask[i1] = 1;
        }
        // draw inliers
        drawMatches(img1, keypoints1, img2, keypoints2, filteredMatches, drawImg, CV_RGB(0, 255, 0), CV_RGB(0, 0, 255), matchesMask
#if DRAW_RICH_KEYPOINTS_MODE
                     , DrawMatchesFlags::DRAW_RICH_KEYPOINTS
#endif
                   );

#if DRAW_OUTLIERS_MODE
        // draw outliers
        for( size_t i1 = 0; i1 < matchesMask.size(); i1++ )
            matchesMask[i1] = !matchesMask[i1];
        drawMatches( img1, keypoints1, img2, keypoints2, filteredMatches, drawImg, CV_RGB(0, 0, 255), CV_RGB(255, 0, 0), matchesMask,
                     DrawMatchesFlags::DRAW_OVER_OUTIMG | DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
#endif
    }
    else {
      drawMatches(img1, keypoints1, img2, keypoints2, filteredMatches, drawImg);
    }
    imshow( winName, drawImg );
}

int main(int argc, char** argv)
{
    if(argc != 8) {
    	help(argv);
        return -1;
    }
    double ransacReprojThreshold = atof(argv[7]);
    ransacReprojThreshold = ransacReprojThreshold < 0 ? 0 : ransacReprojThreshold;

    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> descriptorExtractor;
    if(argv[1] == "SIFT") {
      detector = cv::xfeatures2d::SIFT::create();
      descriptorExtractor = cv::xfeatures2d::SIFT::create();
    } else {
      detector = cv::xfeatures2d::SIFT::create();
      descriptorExtractor = cv::xfeatures2d::SIFT::create();
    }
    Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create(argv[3]);

    if(detector.empty() || descriptorExtractor.empty() || descriptorMatcher.empty()) {
        cout << "Can not create detector or descriptor exstractor or descriptor matcher of given types" << endl;
        return -1;
    }
    int mactherFilterType = getMatcherFilterType(argv[4]);
    bool eval = true;
		
    Mat img1 = imread(argv[5]);
    Mat img2 = imread(argv[6]);
    resize(img1, img1, Size(640, 480));
    resize(img2, img2, Size(640,480));
    if(img1.empty() || img2.empty()) {
        cout << "Can not read images" << endl;
        return -1;
    }

    vector<KeyPoint> keypoints1;
    detector->detect( img1, keypoints1 );
    cout << "Total of " << keypoints1.size() << " keypoints extracted from first image" << endl;

    Mat descriptors1;
    descriptorExtractor->compute( img1, keypoints1, descriptors1 );

    namedWindow(winName, 1);
    doIteration( img1, img2, keypoints1, descriptors1,
                 detector, descriptorExtractor, descriptorMatcher, mactherFilterType, eval,
                 ransacReprojThreshold);

    waitKey(0);
    return 0;
}
