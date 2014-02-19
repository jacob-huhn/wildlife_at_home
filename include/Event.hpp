#ifndef EVENT_HEADER
#define EVENT_HEADER

#include <vector>
#include "EventType.hpp"

using namespace std;

class Event {
	 EventType *type;
	 int startTime;
	 int endTime;
    public:
     Event();
     Event(EventType*, int, int);
     void setType(EventType*);
     void setStartTime(int);
     void setEndTime(int);
     EventType* getType();
     int getStartTime();
     int getEndTime();

     void addDescriptors(cv::Mat descriptors);
     void addKeypoints(vector<cv::KeyPoint> keypoints);
     cv::Mat getDescriptors();
     vector<cv::KeyPoint> getKeypoints();
     string getTypeId();
};

#endif
