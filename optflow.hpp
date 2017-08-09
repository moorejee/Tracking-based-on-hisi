#include <iostream>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

Mat draw_hsv(Mat flow)
{
	int h, w;
	Mat flow_channel[2], v, hsv;
	Mat pmatrix(flow.rows, flow.cols, CV_64FC1, Scalar(255));
	//printf("%d", pmatrix);
	h = flow.rows;
	w = flow.cols;
	split(flow, flow_channel);
	v = flow_channel[0].mul(flow_channel[0]) + flow_channel[1].mul(flow_channel[1]);
	//pow(flow_channel[0], 2, v);
	sqrt(v, v);
	v = v * 4;
	hsv =  min(v , 255);//minµÄsrc matrix±ØÐëÊÇcontant
	return hsv;
}
