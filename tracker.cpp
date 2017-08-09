/*
修改自KCFcpp，官方代码
只使用opencv3.2.0，不使用opencv_contribute
最后修改时间：2016.7.28
*/
#include <iostream>
#include <algorithm>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "kcftracker.hpp"
#include "optflow.hpp"
#include <time.h>
#include <string>
#include <stdio.h>

using namespace std;
using namespace cv;


typedef struct
{
	Rect coord;
	Mat frame;
	bool flag = false;
}cap_aim;

const int areathresh = 500; //帧差，光流法前景阈值


//frame difference method
cap_aim capture1_aim(VideoCapture cap)
{
	bool no_aim_frame = false, get_aim0 = false;
	int capindex = 0, interval = 20;

	Mat frame,gray,pregray,frameDelta,thresh,thresh1,kernel;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cap_aim aimset;
	Rect rect_aim = Rect(-1, -1, -1, -1);
	while(true)
	{
		cap >> frame;
		if (frame.rows == 0 || frame.cols == 0)
			break;
		//间隔一定帧数进行检测
		if (capindex == 0)
		{
			capindex = 5;
			cvtColor(frame,gray,COLOR_BGR2GRAY);
			GaussianBlur(gray, gray, Size(5,5), 0);
			if (no_aim_frame == false)
			{
				pregray = gray;
				no_aim_frame = true;
				continue;
			}
			absdiff(pregray, gray, frameDelta);
			threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
			pregray = gray.clone();
			//形态学，闭运算
			kernel = getStructuringElement(MORPH_RECT, Size(20,20)); //size()的值是否过大？
			morphologyEx(thresh, thresh1, MORPH_CLOSE, kernel);
			findContours(thresh1, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
			for (int i = 0; i < contours.size(); i++)
			{
				if (contourArea(contours[i]) < areathresh)
					continue;
				rect_aim = boundingRect(contours[i]);
				rectangle(frame, rect_aim, Scalar(255, 255, 255),2);
				get_aim0 = true;
				break;
			}
		}
		if (get_aim0 == true)
		{
			//找到目标
			aimset.coord = rect_aim;
			aimset.frame = frame;
			destroyWindow("capture");
			break;
		}
		capindex -= 1;
		imshow("capture", frame);
		if (waitKey(interval) == 27)
		{
			//用于判断退出capture1的情况
			aimset.coord = Rect(0,0,0,0);
			break;
		}
	}
	return aimset;
}
//optflow method
cap_aim capture2_aim(VideoCapture cap)
{
	bool no_aim_frame = false, get_aim = false;
	int capindex = 0, interval = 20;
	Mat frame,scale_frame,flow,gray,pregray,frameDelta,thresh,thresh1,kernel,hsv;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cap_aim aimset;
	Rect rect_aim = Rect(-1, -1, -1, -1);
	double fx = 0.5, fy = 0.5;
	while(true)
	{
		cap >> frame;
		//scale_frame = frame.clone();
		if (frame.rows == 0 || frame.cols == 0)
			break;
		if (capindex == 0)
		{
			capindex = 2;
			//缩小图像为1/4，加快运算
			resize(frame, scale_frame, Size(0,0), fx, fy);
			cvtColor(scale_frame, gray, COLOR_BGR2GRAY);
			if (no_aim_frame == false)
			{
				pregray = gray;
				no_aim_frame = true;
				continue;
			}
			//前帧，当前帧，金字塔参数0.5，金字塔层数，窗口大小，迭代次数，像素邻域，高斯标准差，flags
			calcOpticalFlowFarneback(pregray, gray, flow, 0.5, 2, 20, 2, 5, 1.2, 0);
			pregray = gray.clone();
			hsv = draw_hsv(flow);//optflow中可确定目标轮廓
			threshold(hsv, thresh, 5, 255, THRESH_BINARY);
			//划定轮廓，形态学
			kernel = getStructuringElement(MORPH_RECT, Size(20,20)); //size()的值是否过大？
			morphologyEx(thresh, thresh, MORPH_OPEN, kernel);
			morphologyEx(thresh, thresh, MORPH_CLOSE, kernel);
			//减小轮廓范围
			kernel = getStructuringElement(MORPH_CROSS, Size(15, 15));
			erode(thresh, thresh, kernel);
			thresh.convertTo(thresh1, CV_8U);//findContours默认处理cv_8u的数据类
			//cout << "depth:" << thresh.depth() << endl;
			findContours(thresh1, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
			//printf("%d", contours.size());
			for (int i = 0; i < contours.size(); i++)
			{
				if (contourArea(contours[i]) < areathresh/4)
					continue;
				rect_aim = boundingRect(contours[i]);
				rect_aim = rect_aim + Point(rect_aim.x, rect_aim.y) + Size(rect_aim.width, rect_aim.height);
				rectangle(frame, rect_aim, Scalar(255, 255, 255),2);
				get_aim = true;
				break;
			}
			imshow("thresh", thresh);
			if (waitKey(interval) == 27)
				break;
		}
		if (get_aim == true)
		{
			aimset.coord = rect_aim;
			aimset.frame = frame;
			destroyWindow("capture");
			destroyWindow("thresh");
			break;
		}
		capindex -= 1;
		imshow("capture", frame);
		if (waitKey(interval) == 27)
		{
			aimset.coord = Rect(0,0,0,0);
			break;
		}
	}
	return aimset;
}


int opencv_fun(HI_VOID* pYuvBuf, HI_S32 dWidth, HI_S32 dHeight, int index)
{

	bool HOG = false;
	bool FIXEDWINDOW = false;
	bool MULTISCALE = true;
	bool SILENT = true;
	bool LAB = false;
	// set input video
	// std::string video = "Walk.mpg";
	// VideoCapture cap(video);
	//VideoCapture cap(0);
	// Create KCFTracker object
	KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);
	// int interval = 20;
	// clock_t track_start, track_end;
	// double duration;//计算算法的运行速度
	// string duration_str;
	Mat frame;
	// double rate = cap.get(CV_CAP_PROP_FPS);
	// double resolution_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	// double resolution_high = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	// cout << "fps:" << rate << endl;
	// cout << "resolution:" << resolution_width <<"*"<< resolution_high << endl;
	Rect result;


	Mat yuvImg;
	Mat rgbImg,rgbImg0;
	int bufLen;
	bufLen = dWidth * dHeight * 3/2;
	yuvImg.create(dHeight * 3/2, dWidth, CV_8UC1);
	memcpy(yuvImg.data, pYuvBuf, bufLen);
	cvtColor(yuvImg, rgbImg0, CV_YUV2BGR_I420);//得到Mat数据

	// do the tracking
	// printf("Start the tracking process, press ESC to quit.\n");
	bool get_aim = false;
	cap_aim aim;
	while (true)
	{
		// get frame from the video
		cap >> frame;
		// stop the program if no more images
		if (frame.rows == 0 || frame.cols == 0)
			break;
		if (get_aim == false)
		{
			aim = capture1_aim(cap);
			//aim = capture2_aim(cap);
			if (aim.coord == Rect(0,0,0,0))
			{
				//printf("Esc退出");
				break;
			}
			if (aim.coord == Rect(-1,-1,-1,-1))
			{
				//printf("无目标");
				break;
			}
			tracker.init(aim.coord, aim.frame);
			get_aim = true;
			if (aim.coord.width == 0 || aim.coord.height == 0)
				break;
			continue;
		}
		// track_start = clock();
		result = tracker.update(frame);
		// track_end = clock();
		// duration = (double)(track_end - track_start) / CLOCKS_PER_SEC;
		//duration = 0.01*0.8 + duration*0.2;
		//duration_str = to_string(1/duration);
		// putText(frame, "FPS:"+duration_str, Size(8, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,0,255));
		// rectangle(frame, Point(result.x, result.y), Point(result.x + result.width, result.y + result.height), Scalar(0, 255, 255), 1, 8);
		printf("x:%d,y:%d,width:%d,height:%d",result.x,result.y,result.width,result.height);
		// imshow("Tracking", frame);
		// //quit on ESC button
		// if (waitKey(interval) == 27)
		// 	break;
		//capture1_aim(cap);
	}
}
