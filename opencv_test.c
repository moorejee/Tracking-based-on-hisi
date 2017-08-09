#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <stdio.h>

//using namespace std;
//using namespace cv;

int main(int argc, char *argv[])
{
	const int width = 704;
	const int height = 576;
	int framecount;
	char *pYuvBuf1[width*height];//初始化数组变量时，其空间大小一定是const！！
	char *pYuvBuf2[width * height * 3/2 ];
	IplImage *rgbImg = cvCreateImageHeader(cvSize(width, height), 8, 1);
	IplImage *yuvImg = cvCreateImageHeader(cvSize(width, height * 3/2), 8, 1);
	char name[50];
	//printf("yuv file w: %d, h: %d \n", w, h);
	//errno_t err;
	FILE* pFileIn;
	//err = fopen_s(&pFileIn, "K:/ZJU/Code/videos/ffmpeg/chn0_720_576_229fps.yuv", "rb+");
	pFileIn = fopen("SAMPLE_420_D1_704_576.yuv", "rb+");
	int bufLen = width *height * 3 / 2;
	// unsigned char* pYuvBuf1 = new unsigned char[bufLen];
	// unsigned char* pYuvBuf2 = new unsigned char[bufLen];
	int i;
	// calculate the frame num
	fseek(pFileIn, 0, SEEK_END);
	framecount = (int)((int)ftell(pFileIn) / ((width * height * 3) / 2));  //ftell():Returns the current value of the position indicator of the stream.用于求文件大小
	printf("frame num is %d \n", framecount);
	//printf("file length is   %d", ftell(f));
	fseek(pFileIn, 0, SEEK_SET);
	for (i = 0; i<framecount; i++)
	{
		//一次读取yuv所有数据
		//fread(pYuvBuf1, bufLen * sizeof(unsigned char), 1, pFileIn); //fread():The position indicator of the stream is advanced by the total amount of bytes read.
		//Mat yuvImg;
		//yuvImg.create(height * 3/2 , width, CV_8UC1);
		//memcpy(yuvImg.data, pYuvBuf1, bufLen * sizeof(unsigned char));
		printf("while!\n");
		//分别读取y,uv数据
		fread(pYuvBuf1, height * width , 1, pFileIn);
		//cvSetImageData(yuvImg, pYuvBuf1, width);
		memcpy(pYuvBuf2, pYuvBuf1, height * width);
		//memset(pYuvBuf1, 0, height * width * sizeof(unsigned char));
		fread(pYuvBuf1, height * width * 1 / 2 , 1, pFileIn);
		// cvSetData(yuvImg, pYuvBuf1, width);
		memcpy(pYuvBuf2 + height * width, pYuvBuf1, height * width * 1 / 2 );
		//yuvImg.create(height * 3 / 2, width, CV_8UC1);
		cvSetData(yuvImg, pYuvBuf2, width);
		// cvCvtColor(yuvImg, rgbImg, CV_YCrCb2RGB);//有问题（？）
		// cvShowImage("img", rgbImg);
		// if (cvWaitKey(1) == 27)
		// 	break;
		//printf("%d \n", iCount++);
		sprintf(name,"%d.jpg", i);
		// printf("%d");
		cvSaveImage(name , yuvImg, 0);//注意打开地址的写入权限！！！
	}

	// delete[] pYuvBuf1;
	// delete[] pYuvBuf2;
	// free(pYuvBuf1);
	// free(pYuvBuf2);
    // cvReleaseImageHeader(yuvImg);
	// cvReleaseImageHeader(rgbImg);
	fclose(pFileIn);
	return 0;
}


// #include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
// #include <iostream>
// #include <stdio.h>
// #include <string>
// #include <stdlib.h>
//
// using namespace std;
// using namespace cv;
// int main(int argc,char *argv[])
// {
// 	int framecount;
// 	// char index[50];
// 	const int width = 704;
// 	const int height = 576;
// 	Mat yuvImg;
// 	Mat rgbImg;
// 	char name[50];
// 	//printf("yuv file w: %d, h: %d \n", w, h);
// 	//errno_t err;
// 	FILE* pFileIn;
// 	//err = fopen_s(&pFileIn, "K:/ZJU/Code/videos/ffmpeg/chn0_720_576_229fps.yuv", "rb+");
// 	pFileIn = fopen("/mnt/hi/tmp/SAMPLE_420_D1_704_576.yuv", "rb+");
// 	int bufLen = width *height * 3 / 2;
// 	unsigned char* pYuvBuf1 = new unsigned char [bufLen];
// 	unsigned char* pYuvBuf2 = new unsigned char [bufLen];
// 	int i;
// 	// calculate the frame num
// 	fseek(pFileIn, 0, SEEK_END);
// 	framecount = (int)((int)ftell(pFileIn) / ((width * height * 3) / 2));  //ftell():Returns the current value of the position indicator of the stream.用于求文件大小
// 	// printf("frame num is %d \n", framecount);
// 	//printf("file length is   %d", ftell(f));
// 	fseek(pFileIn, 0, SEEK_SET);
// 	for(i=0; i<framecount; i++)
// 	{
// 		//一次读取yuv所有数据
// 		//fread(pYuvBuf1, bufLen * sizeof(unsigned char), 1, pFileIn); //fread():The position indicator of the stream is advanced by the total amount of bytes read.
// 		//Mat yuvImg;
// 		//yuvImg.create(height * 3/2 , width, CV_8UC1);
// 		//memcpy(yuvImg.data, pYuvBuf1, bufLen * sizeof(unsigned char));
//
// 		//分别读取y,uv数据
// 		fread(pYuvBuf1, height * width, 1, pFileIn);
// 		memcpy(pYuvBuf2, pYuvBuf1, height * width);
// 		//memset(pYuvBuf1, 0, height * width * sizeof(unsigned char));
// 		fread(pYuvBuf1, height * width * 1/2, 1, pFileIn);
// 		memcpy(pYuvBuf2 + height * width, pYuvBuf1, height * width * 1/2);
// 		yuvImg.create(height * 3 / 2, width, CV_8UC1);
// 		memcpy(yuvImg.data, pYuvBuf2, bufLen);
// 		cvtColor(yuvImg, rgbImg, CV_YUV2BGR_I420);
// 		// cvtColor(rgbImg, rgbImg, CV_BGR2RGB);
// 		//cvtColor(rgbImg, yuvImg, CV_RGB2YUV_I420);
// 		// imshow("img", yuvImg);
// 		// if (waitKey(1) == 27)
// 		// 	break;
// 		//printf("%d \n", iCount++);
// 		// itoa(i,index,10);
// 		// name = "Walk" + index + ".jpg";
// 		sprintf(name,"/mnt/hi/tmp/output/%d.jpg", i);
// 		// name = "pic.jpg";
// 		imwrite(name,rgbImg);
// 	}
//
// 	// delete[] pYuvBuf1;
// 	// delete[] pYuvBuf2;
// 	fclose(pFileIn);
// 	return 0;
// }
