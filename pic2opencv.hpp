#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <stdlib.h>
#include "sample_comm.h"
#include <stdio.h>

using namespace std;
using namespace cv;


void opencv_fun(HI_VOID* pYuvBuf, HI_S32 dWidth, HI_S32 dHeight, int fcount)
{
	char name[50];
	Mat yuvImg;
	Mat rgbImg,rgbImg0;
	int bufLen;
	// bufLen = dWidth * dHeight * 3/2;
	// yuvImg.create(dHeight * 3/2, dWidth, CV_8UC1);
	// memcpy(yuvImg.data, pYuvBuf, bufLen);
	bufLen = dWidth * dHeight;
	yuvImg.create(dHeight, dWidth, CV_8UC1);
	memcpy(yuvImg.data, pYuvBuf, bufLen);
	// cvtColor(yuvImg, rgbImg0, CV_YUV2BGR_I420);//得到Mat数据
	// cvtColor(rgbImg0, rgbImg, CV_BGR2RGB);
	// printf("rows:%d,cols:%d",rgbImg0.rows,rgbImg0.cols);

	// sprintf(name, "/mnt/hi/vdec/output/%d.jpg", fcount);
	// imwrite(name,yuvImg);


}
