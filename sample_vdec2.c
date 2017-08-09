/******************************************************************************
  A simple program of Hisilicon HI3531 video decode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-12 Created
******************************************************************************/
// #include "tracker.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include "kcftracker.hpp"
#include "optflow.hpp"
using namespace std;
using namespace cv;


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "sample_comm.h"
// #include <opencv2/core/core_c.h>
// #include <opencv2/imgproc/imgproc_c.h>
// #include <opencv2/highgui/highgui_c.h>
// #include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>

#define SAMPLE_MAX_VDEC_CHN_CNT 8


typedef struct sample_vdec_sendparam
{
    pthread_t Pid;
    HI_BOOL bRun;
    VDEC_CHN VdChn;
    PAYLOAD_TYPE_E enPayload;
	HI_S32 s32MinBufSize;
    VIDEO_MODE_E enVideoMode;
}SAMPLE_VDEC_SENDPARAM_S;

//HI_S32 gs_VoDevCnt = 4;
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
HI_U32 gs_u32ViFrmRate = 0;
SAMPLE_VDEC_SENDPARAM_S gs_SendParam[SAMPLE_MAX_VDEC_CHN_CNT];
HI_S32 gs_s32Cnt;

//timeval结构定义为:minicom
// struct timeval{
// long tv_sec; /*秒*/
// long tv_usec; /*微秒*/
// };
struct timeval s1,e1;
double timeuse,timecount,timemean;
int fcount;

typedef struct
{
	Rect coord;
	Mat frame;
	bool flag;
}cap_aim;
cap_aim capture1_aim(Mat);
char** sample_yuv_transform(VIDEO_FRAME_INFO_S *, HI_VOID*, HI_VOID*);
//全局变量
Mat pregray;
const int areathresh = 1000; //帧差，光流法前景阈值
bool no_aim_frame = false, get_aim0 = false;
/******************************************************************************
* function : send stream to vdec
******************************************************************************/
void* SAMPLE_VDEC_SendStream(void* p)
{
    VDEC_STREAM_S stStream;
    SAMPLE_VDEC_SENDPARAM_S *pstSendParam;
    char sFileName[50], sFilePostfix[20];
    FILE* fp = NULL;
    HI_S32 s32BlockMode = HI_IO_NOBLOCK;
    HI_U8 *pu8Buf;
    HI_U64 u64pts;
    HI_S32 s32IntervalTime = 40000;
    HI_S32 i, s32Ret, len, start;
    HI_S32 s32UsedBytes, s32ReadLen;
    HI_BOOL bFindStart, bFindEnd;

    start = 0;
    u64pts= 0;
    s32UsedBytes = 0;
    pstSendParam = (SAMPLE_VDEC_SENDPARAM_S *)p;

    HI_S32 Imagelen0;
	HI_S32 Imagelen;
	HI_S32 Height, Width, Stride;
    HI_VOID* ptr1;//初始化虚拟内存指针
	HI_VOID* ptr2;
	const HI_S32 dwidth = 720;
	const HI_S32 dheight = 576;
    char pYuvBuf[dwidth * dheight * 3/2]; //初始化(数组的变量名即为指针)！只需对空间大小进行初始化即可，否则会出现内存错误
	// char *pYuvBuf;
    VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *) malloc(sizeof(VIDEO_FRAME_INFO_S)); //初始化结构体指针
	char** ptr;
	Mat yuvImg, frame;
	char name[50];
	Rect result;
	bool get_aim = false;
	cap_aim aim;
	aim.flag = false;
	FILE *ftarget = NULL;


    /******************* open the stream file *****************/
    SAMPLE_COMM_SYS_Payload2FilePostfix(pstSendParam->enPayload, sFilePostfix); // sFilePostfix文件后缀名
    sprintf(sFileName, "stream_chn0%s", sFilePostfix);
    fp = fopen(sFileName, "r");
    if (HI_NULL == fp)
    {
        SAMPLE_PRT("can't open file %s in send stream thread:%d\n", sFileName,pstSendParam->VdChn);
        return (HI_VOID *)(HI_FAILURE);
    }
    printf("open file [%s] ok in send stream thread:%d!\n", sFileName,pstSendParam->VdChn);


    /******************* malloc the  stream buffer in user space *****************/
    if(pstSendParam->s32MinBufSize!=0)
    {
        printf("%d", pstSendParam->s32MinBufSize);
		// pu8Buf=malloc(pstSendParam->s32MinBufSize);
        pu8Buf=(HI_U8 *)malloc(pstSendParam->s32MinBufSize);
        if(pu8Buf==NULL)
        {
            SAMPLE_PRT("can't alloc %d in send stream thread:%d\n",pstSendParam->s32MinBufSize,pstSendParam->VdChn);
            fclose(fp);
            return (HI_VOID *)(HI_FAILURE);
        }
    }
    else

    {
        SAMPLE_PRT("none buffer to operate in send stream thread:%d\n",pstSendParam->VdChn);
        return (HI_VOID *)(HI_FAILURE);
    }
	fcount = 0; //输出图像序列编号
	/************tracker parameters**************/
	bool HOG = true;
	bool FIXEDWINDOW = false;
	bool MULTISCALE = true;
	bool SILENT = true;
	bool LAB = false;
	KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);
	ftarget = fopen("/mnt/hi/vdec/text.txt","w+");
    while (pstSendParam->bRun)
    {
		gettimeofday(&s1,NULL);
        printf("Test loop!!\n");
        fseek(fp, s32UsedBytes, SEEK_SET);
        s32ReadLen = fread(pu8Buf, 1, pstSendParam->s32MinBufSize, fp);
        // printf("%d", pstSendParam->s32MinBufSize);
        if (s32ReadLen<=0)
        {
             printf("file end.\n");
             break;
        }

        /******************* cutting the stream for frame *****************/
        if( (pstSendParam->enVideoMode==VIDEO_MODE_FRAME) && (pstSendParam->enPayload== PT_H264) )
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            for (i=0; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                    ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                    ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindStart = HI_TRUE;
                    i += 4;
                    break;
                }
            }

            for (; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                    ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                    ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }

            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                SAMPLE_PRT("can not find start code in send stream thread:%d\n",pstSendParam->VdChn);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+5;
            }
        }
        else if( (pstSendParam->enPayload== PT_JPEG) || (pstSendParam->enPayload == PT_MJPEG) )
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            for (i=0; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    start = i;
                    bFindStart = HI_TRUE;
                    i = i + 2;
                    break;
                }
            }

            for (; i<s32ReadLen-4; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i+1]& 0xF0) == 0xE0)
                {
                     len = (pu8Buf[i+2]<<8) + pu8Buf[i+3];
                     i += 1 + len;
                }
                else
                {
                    break;
                }
            }

            for (; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }

            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                printf("\033[0;31mALERT!!!,can not find start code in send stream thread:%d!!!\033[0;39m\n",
                pstSendParam->VdChn);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+2;
            }
        }

         stStream.u64PTS  = u64pts;
         stStream.pu8Addr = pu8Buf + start;
         stStream.u32Len  = s32ReadLen;

        /******************* send stream *****************/
        if (s32BlockMode == HI_IO_BLOCK)
        {
            s32Ret=HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_BLOCK);
        }
        else if (s32BlockMode == HI_IO_NOBLOCK)
        {
            s32Ret=HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_NOBLOCK);
        }
        else
        {
            s32Ret=HI_MPI_VDEC_SendStream_TimeOut(pstSendParam->VdChn, &stStream, 8000);
        }

        if (HI_SUCCESS == s32Ret)
        {
            s32UsedBytes = s32UsedBytes +s32ReadLen + start;
        }
        else
        {
            if (s32BlockMode != HI_IO_BLOCK)
            {
                SAMPLE_PRT("failret:%x\n",s32Ret);
            }
            usleep(s32IntervalTime);
        }
        usleep(20000);

        /******************************************
        Self---function 1: get decoded pic from vdec
        ******************************************/

        if(pstSendParam->VdChn == 0 && s32BlockMode == HI_IO_NOBLOCK)
        {
			printf("Start getting images from vdec.\n");
			s32Ret = HI_MPI_VDEC_GetImage(pstSendParam->VdChn, pstFrameInfo, s32BlockMode);
			//   usleep(100000);
			if (s32Ret != HI_SUCCESS)
			{
			SAMPLE_PRT("HI_MPI_VDEC_GetImage failed with %#x!\n", s32Ret);
			}
			else
			{
			printf("get image from vdec success.\n");

			}
			Height = pstFrameInfo->stVFrame.u32Height;
			Width = pstFrameInfo->stVFrame.u32Width;
			Stride = pstFrameInfo->stVFrame.u32Stride[0];
			Imagelen0 = Stride * Height; //原始数据长度，数据存储的宽度为stride>=width.
			//stride是为了将数据长度向上按16位或是64位补齐
			Imagelen = Width * Height;//处理后的真实数据的长度．
			//Mmap作用是把物理内存地址 映射到用户虚拟内存空间，用户程序不能直接访问物理内存，只能访问虚拟内存！
			ptr1 = HI_MPI_SYS_Mmap(pstFrameInfo->stVFrame.u32PhyAddr[0], Imagelen0);
			ptr2 = HI_MPI_SYS_Mmap(pstFrameInfo->stVFrame.u32PhyAddr[1], Imagelen0 / 2);
			//   printf("addr[0]:%d,addr[1]:%d\n",pstFrameInfo->stVFrame.u32PhyAddr[0],pstFrameInfo->stVFrame.u32PhyAddr[1]);
			//   printf("stride:%d\n,width:%d\n,height:%d\n",pstFrameInfo->stVFrame.u32Stride[0],pstFrameInfo->stVFrame.u32Width,pstFrameInfo->stVFrame.u32Height);
			//   ptr = sample_yuv_dump(pstFrameInfo->stVFrame.enPixelFormat, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height, ptr1, ptr2);

			ptr = sample_yuv_transform(pstFrameInfo,ptr1,ptr2);//注意：将数据去stride以及转化为yuv420p，可能会影响到SDK中的其他函数的调用(这里是为了opencv处理方便)
			memcpy(pYuvBuf, *ptr, Imagelen); //memcpy较耗时，可优化（？）
			memcpy(pYuvBuf + Imagelen, *(ptr+1), Imagelen / 2);
			//   memcpy(pYuvBuf, ptr1, Imagelen0);
			//   memcpy(pYuvBuf + Imagelen, ptr2, Imagelen0 / 2);


			/******************************************
			Self---function 4: tracker function
			******************************************/
			// opencv_fun(pYuvBuf, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height, fcount);
			// yuvImg.create(Height * 3/2, Width, CV_8UC1);
			// memcpy(yuvImg.data, pYuvBuf, Imagelen * 3/2);
			yuvImg.create(Height, Width, CV_8UC1);
			memcpy(yuvImg.data, pYuvBuf, Imagelen);
			frame = yuvImg.clone();
			// cvtColor(yuvImg, frame, CV_YUV2BGR_I420);//得到Mat数据
			// sprintf(name, "/mnt/hi/vdec/output/%d.jpg", fcount);
			// imwrite(name,frame);
			if (frame.rows == 0 || frame.cols == 0)
			{
				printf("Waiting for frame!\n");
				continue;
			}
			if (get_aim == false)
			{
				get_aim0 = false;
				aim = capture1_aim(frame);
				if (aim.flag == true)
				{
					tracker.init(aim.coord, aim.frame);
					get_aim = true;
				}
				else
				{
					printf("finding target!\n");
				}
			}
			else
			{
				result = tracker.update(frame);
				printf("x:%d,y:%d,width:%d,height:%d\n",result.x,result.y,result.width,result.height);
				fprintf(ftarget,"x:%d,y:%d,width:%d,height:%d\n",result.x,result.y,result.width,result.height);
			}


			printf("帧:%d\n",fcount);
			HI_MPI_SYS_Munmap(ptr1, Imagelen0);
			HI_MPI_SYS_Munmap(ptr2, Imagelen0/2);
			(void) HI_MPI_VDEC_ReleaseImage(pstSendParam->VdChn, pstFrameInfo); //release与get应对应
			fcount++;
        }
		gettimeofday(&e1,NULL);
		timeuse = e1.tv_sec - s1.tv_sec + (e1.tv_usec - s1.tv_usec)/1000000.0;
		printf("time:%f\n",timeuse);
		timecount += timeuse;

    }
	fclose(ftarget);
    // free(pYuvBuf);
    // cvReleaseImageHeader(yuvImg);
	// cvReleaseImageHeader(rgbImg);

	printf("send steam thread %d return ...\n", pstSendParam->VdChn);
	fflush(stdout);
	if (pu8Buf != HI_NULL)
	{
        free(pu8Buf);
	}
	fclose(fp);
	return (HI_VOID *)HI_SUCCESS;
}

/****************************************************************************
Self--function 5:tracker subfunction
****************************************************************************/

//frame difference method
cap_aim capture1_aim(Mat frame)
{
	int capindex = 0, interval = 20;
	Mat gray,frameDelta,thresh,thresh1,kernel;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cap_aim aimset;
	aimset.flag = false;
	Rect rect_aim = Rect(-1, -1, -1, -1);
	// cvtColor(frame,gray,COLOR_BGR2GRAY);
	gray = frame.clone();
	GaussianBlur(gray, gray, Size(5,5), 0);
	if (no_aim_frame == false)
	{
		// printf("1\n");
		pregray = gray;
		no_aim_frame = true;
		return aimset;
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
		printf("2\n");
		if (contourArea(contours[i]) < areathresh)
			continue;
		rect_aim = boundingRect(contours[i]);
		// rectangle(frame, rect_aim, Scalar(255, 255, 255),2);
		get_aim0 = true;
		break;
	}
	if (get_aim0 == true)
	{
		//找到目标
		printf("3\n");
		aimset.coord = rect_aim;
		aimset.frame = frame;
		aimset.flag = true;
		return aimset;
	}
	return aimset;
}

/****************************************************************************
Self--function 2:yuv420sp TO yuv420p
****************************************************************************/


// char** sample_yuv_dump(PIXEL_FORMAT_E pixel, HI_U32 width, HI_U32 height, HI_VOID* ptr1, HI_VOID* ptr2)
// {
// 	unsigned int w, h, i;
// 	char * pVBufVirt_Y;
// 	char * pVBufVirt_C;
// 	char * pMemContent;
// 	// unsigned char TmpBuff[2000];                //如果这个值太小，图像很大的话存不了
// 	HI_U32 u32UvHeight;/* 存为planar 格式时的UV分量的高度 */
// 	const HI_U32 Len= width * height;
// 	char pYUV1[Len];
//     char pYUV2[Len * 1/2];
// 	char* pYUV[2];
//
// 	if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pixel)
// 	{
// 		// size = width * height * 3 / 2; //u32Stride 表示图像跨距：即图像宽度存储时占用的字节数。
// 		u32UvHeight = height / 2;
// 	}
// 	else
// 	{
// 		// size = (pVBuf->u32Stride[0])*(pVBuf->u32Height) * 2;
// 		// u32UvHeight = pVBuf->u32Height;
// 		printf("the stream is not the yuv420sp!");
// 		return 0;
// 	}
//
// 	pVBufVirt_Y = (char*) ptr1;
// 	pVBufVirt_C = (char*) ptr2;
//
// 	/* save Y ----------------------------------------------------------------*/
// 	// memcpy(pYUV1, pVBufVirt_Y, Len）;
//
// 	/* save U ----------------------------------------------------------------*/
// 	i = 0;
// 	for (h = 0; h<u32UvHeight; h++)
// 	{
// 		pMemContent = pVBufVirt_C + h * width;
//
// 		pMemContent += 1;// UV分量的第一个元素是V
//
// 		for (w = 0; w<width / 2; w++)
// 		{
// 			pYUV2[i++] = *pMemContent;
// 			pMemContent += 2;
// 		}
// 	}
// 	/* save V ----------------------------------------------------------------*/
// 	for (h = 0; h<u32UvHeight; h++)
// 	{
// 		pMemContent = pVBufVirt_C + h * width;
//
// 		for (w = 0; w<width / 2; w++)
// 		{
// 			pYUV2[i++] = *pMemContent;
// 			pMemContent += 2;
// 		}
// 	}
// 	pYUV[0] = pVBufVirt_Y;
// 	pYUV[1] = (char*)pYUV2;
// 	return pYUV;
// }


/****************************************************************************
Self--function 3:change pstFrameInfo(yuv420sp TO yuv420p)
****************************************************************************/


char** sample_yuv_transform(VIDEO_FRAME_INFO_S *pstFrameInfo, HI_VOID* ptr1, HI_VOID* ptr2)
{
	unsigned int w, h, i;
	char * pVBufVirt_Y;
	char * pVBufVirt_C;
	char * pMemContent;
	// unsigned char TmpBuff[2000];                //如果这个值太小，图像很大的话存不了
	HI_U32 u32UvHeight;/* 存为planar 格式时的UV分量的高度 */
	const HI_U32 width = pstFrameInfo->stVFrame.u32Width;
	const HI_U32 height = pstFrameInfo->stVFrame.u32Height;
	const HI_U32 stride = pstFrameInfo->stVFrame.u32Stride[0];
	const HI_U32 Len = width * height;
	char pYUV1[Len];
    char pYUV2[Len * 1/2];
	char* pYUV[2];

	if(PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pstFrameInfo->stVFrame.enPixelFormat)
	{
		// size = width * height * 3 / 2; //u32Stride 表示图像跨距：即图像宽度存储时占用的字节数。
		u32UvHeight = height / 2;
	}
	else
	{
		// size = (pVBuf->u32Stride[0])*(pVBuf->u32Height) * 2;
		// u32UvHeight = pVBuf->u32Height;
		printf("the stream is not the yuv420sp!");
		return 0;
	}

	pVBufVirt_Y = (char*) ptr1;
	pVBufVirt_C = (char*) ptr2;

	/* save Y ----------------------------------------------------------------*/
	// memcpy(pYUV1, pVBufVirt_Y, Len）;
	if(stride == width)
	{
		pYUV[0] = pVBufVirt_Y;//不用对Ｙ分量进行操作
	}
	else
	{
		i = 0;
		for(h = 0; h<height; h++)
		{
			pMemContent = pVBufVirt_Y + h * stride;
			for(w = 0; w<width; w++)
			{
				pYUV1[i++] = *pMemContent;
				pMemContent += 1;
			}
		}
		pYUV[0] = (char*)pYUV1;
	}

	/* save U ----------------------------------------------------------------*/
	i = 0;
	for (h = 0; h<u32UvHeight; h++)
	{
		pMemContent = pVBufVirt_C + h * stride;

		pMemContent += 1;// UV分量的第一个元素是V

		for (w = 0; w<width / 2; w++)
		{
			pYUV2[i++] = *pMemContent;
			pMemContent += 2;
		}
	}
	/* save V ----------------------------------------------------------------*/
	for (h = 0; h<u32UvHeight; h++)
	{
		pMemContent = pVBufVirt_C + h * stride;

		for (w = 0; w<width / 2; w++)
		{
			pYUV2[i++] = *pMemContent;
			pMemContent += 2;
		}
	}

	pYUV[1] = (char*)pYUV2;
	return pYUV;
}

/******************************************************************************
* function : create vdec chn
******************************************************************************/
static HI_S32 SAMPLE_VDEC_CreateVdecChn(HI_S32 s32ChnID, SIZE_S *pstSize, PAYLOAD_TYPE_E enType, VIDEO_MODE_E enVdecMode)
{
    VDEC_CHN_ATTR_S stAttr;
    VDEC_PRTCL_PARAM_S stPrtclParam;
    HI_S32 s32Ret;

    memset(&stAttr,0,sizeof(VDEC_CHN_ATTR_S));

    stAttr.enType = enType;
    stAttr.u32BufSize = pstSize->u32Height * pstSize->u32Width;//This item should larger than u32Width*u32Height*3/4
    stAttr.u32Priority = 1;//�˴���������0
    stAttr.u32PicWidth = pstSize->u32Width;
    stAttr.u32PicHeight = pstSize->u32Height;

    switch (enType)
    {
        case PT_H264:
    	    stAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    	    stAttr.stVdecVideoAttr.enMode = enVdecMode;
    	    stAttr.stVdecVideoAttr.s32SupportBFrame = 0;
            break;
        case PT_JPEG:
            stAttr.stVdecJpegAttr.enMode = enVdecMode;
            break;
        case PT_MJPEG:
            stAttr.stVdecJpegAttr.enMode = enVdecMode;
            break;
        default:
            SAMPLE_PRT("err type \n");
            return HI_FAILURE;
    }

    s32Ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VDEC_GetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_GetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    stPrtclParam.s32MaxSpsNum = 21;
    stPrtclParam.s32MaxPpsNum = 22;
    stPrtclParam.s32MaxSliceNum = 100;
    s32Ret = HI_MPI_VDEC_SetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_SetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* function : force to stop decoder and destroy channel.
*            stream left in decoder will not be decoded.
******************************************************************************/
void SAMPLE_VDEC_ForceDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }

    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }
}

/******************************************************************************
* function : wait for decoder finished and destroy channel.
*            Stream left in decoder will be decoded.
******************************************************************************/
void SAMPLE_VDEC_WaitDestroyVdecChn(HI_S32 s32ChnID, VIDEO_MODE_E enVdecMode)
{
    HI_S32 s32Ret;
    VDEC_CHN_STAT_S stStat;

    memset(&stStat, 0, sizeof(VDEC_CHN_STAT_S));

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
        return;
    }

    /*** wait destory ONLY used at frame mode! ***/
    if (VIDEO_MODE_FRAME == enVdecMode)
    {
        while (1)
        {
            //printf("LeftPics:%d, LeftStreamFrames:%d\n", stStat.u32LeftPics,stStat.u32LeftStreamFrames);
            usleep(40000);
            s32Ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VDEC_Query failed errno 0x%x \n", s32Ret);
                return;
            }
            if ((stStat.u32LeftPics == 0) && (stStat.u32LeftStreamFrames == 0))
            {
                printf("had no stream and pic left\n");
                break;
            }
        }
    }
    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
        return;
    }
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VDEC_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t0) H264 -> VPSS -> VO(HD).\n");
    printf("\t1) JPEG ->VPSS -> VO(HD).\n");
    printf("\t2) MJPEG -> VO(SD).\n");
    printf("\t3) H264 -> VPSS -> VO(HD PIP PAUSE STEP).\n");
    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VDEC_HandleSig(HI_S32 signo)
{
    HI_S32 i;

    if (SIGINT == signo || SIGTSTP == signo)
    {
        printf("SAMPLE_VDEC_HandleSig\n");
        for (i=0; i<gs_s32Cnt; i++)
        {
            if (HI_FALSE != gs_SendParam[i].bRun)
            {
                gs_SendParam[i].bRun = HI_FALSE;
                pthread_join(gs_SendParam[i].Pid, 0);
            }
            printf("join thread %d.\n", i);
        }

        SAMPLE_COMM_SYS_Exit();

        printf("program exit abnormally!\n");
		printf("平均耗时：%f\n",timecount/fcount);
    }

    exit(-1);
}

/******************************************************************************
* function : vdec process
*            vo is sd : vdec -> vo
*            vo is hd : vdec -> vpss -> vo
******************************************************************************/
HI_S32 SAMPLE_VDEC_Process(PIC_SIZE_E enPicSize, PAYLOAD_TYPE_E enType, HI_S32 s32Cnt, VO_DEV VoDev)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    SIZE_S stSize;
    VB_CONF_S stVbConf;
    HI_S32 i;
    VPSS_GRP VpssGrp;
    VIDEO_MODE_E enVdecMode;
    HI_CHAR ch;

    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;
    HI_U32 u32WndNum, u32BlkSize;

    HI_BOOL bVoHd; // through Vpss or not. if vo is SD, needn't through vpss

    // VIDEO_FRAME_INFO_S *pstFrameInfo;
    // HI_S32 s32BlockMode = HI_IO_BLOCK;
    // VDEC_CHN VdChn = 0;



    /******************************************
     step 1: init varaible.
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)?25:30;

    if (s32Cnt > SAMPLE_MAX_VDEC_CHN_CNT || s32Cnt <= 0)
    {
        SAMPLE_PRT("Vdec count %d err, should be in [%d,%d]. \n", s32Cnt, 1, SAMPLE_MAX_VDEC_CHN_CNT);

        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return HI_FAILURE;
    }
    if (704 == stSize.u32Width)
    {
        stSize.u32Width = 720;
    }
    else if (352 == stSize.u32Width)
    {
        stSize.u32Width = 360;
    }
    else if (176 == stSize.u32Width)
    {
        stSize.u32Width = 180;
    }

    // through Vpss or not. if vo is SD, needn't through vpss
    if (SAMPLE_VO_DEV_DHD0 != VoDev )
    {
        bVoHd = HI_FALSE;
    }
    else
    {
        bVoHd = HI_TRUE;
    }
    /******************************************
     step 2: mpp system init.
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /* hist buf*/
    stVbConf.astCommPool[0].u32BlkSize = (196*4);
    stVbConf.astCommPool[0].u32BlkCnt = s32Cnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("mpp init failed!\n");
        return HI_FAILURE;
    }

    /******************************************
     step 3: start vpss, if ov is hd.
    ******************************************/
    if (HI_TRUE == bVoHd)
    {
        s32Ret = SAMPLE_COMM_VPSS_Start(s32Cnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("vpss start failed!\n");
            goto END_0;
        }
    }

    /******************************************
     step 4: start vo
    ******************************************/
    u32WndNum = 1;
    enVoMode = VO_MODE_1MUX;

    if (HI_TRUE == bVoHd)
    {
        if(VIDEO_ENCODING_MODE_PAL== gs_enNorm)
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
        }
        else
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_720P60;
        }

        stVoPubAttr.enIntfType = VO_INTF_VGA;
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }
    else
    {
        if(VIDEO_ENCODING_MODE_PAL== gs_enNorm)
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
        }
        else
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_NTSC;
        }

        stVoPubAttr.enIntfType = VO_INTF_CVBS;
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }

    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_1;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_2;
    }

    if (HI_TRUE == bVoHd)
    {
        /* if it's displayed on HDMI, we should start HDMI */
        if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
        {
            if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
            {
                SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
                goto END_1;
            }
        }
    }

    for(i=0;i<u32WndNum;i++)
    {
        VoChn = i;

        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
                goto END_2;
            }
        }
    }

    /******************************************
     step 5: start vdec & bind it to vpss or vo
    ******************************************/
    if (PT_H264 == enType)
    {
        while(1)
        {
            printf("please choose vdec mode:\n");//考虑到h264的压缩，stream实际上是取了多帧!
            printf("\t0) frame mode.\n"); //frame每一帧的编码长度会有不同，因为h264涉及帧间编码
            printf("\t1) stream mode.\n");
            ch = getchar();
            getchar();
            if ('0' == ch)
            {
                enVdecMode = VIDEO_MODE_FRAME;
                break;
            }
            else if ('1' == ch)
            {
                enVdecMode = VIDEO_MODE_STREAM;
                break;
            }
            else
            {
                printf("input invaild! please try again.\n");
                continue;
            }
        }
    }
    else if (PT_JPEG == enType || PT_MJPEG ==enType)
    {
        /* JPEG, MJPEG must be Frame mode! */
        enVdecMode = VIDEO_MODE_FRAME;
    }

    for (i=0; i<s32Cnt; i++)
    {
        /***** create vdec chn *****/
        VdChn = i;
        printf("%d\n",enVdecMode);
        s32Ret = SAMPLE_VDEC_CreateVdecChn(VdChn, &stSize, enType, enVdecMode);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("create vdec chn failed!\n");
            goto END_3;
        }

        /***** bind vdec to vpss *****/
        // if (HI_TRUE == bVoHd)
        // {
        //     VpssGrp = i;
        //     s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
        //     if (HI_SUCCESS !=s32Ret)
        //     {
        //         SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
        //         goto END_3;
        //     }
        // }
        // else
    	// {
        //     VoChn =  i;
        //     s32Ret = SAMLE_COMM_VDEC_BindVo(VdChn, VoDev, VoChn);
        //     if (HI_SUCCESS !=s32Ret)
        //     {
        //         SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
        //         goto END_3;
        //     }
        // }


		// VoChn =  i;
		// s32Ret = SAMLE_COMM_VDEC_BindVo(VdChn, VoDev, VoChn);
		// if (HI_SUCCESS !=s32Ret)
		// {
		// 	SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
		// 	goto END_3;
		// }
    }



    /******************************************
     step 6: open file & video decoder
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        gs_SendParam[i].bRun = HI_TRUE;
        gs_SendParam[i].VdChn = i;
        gs_SendParam[i].enPayload = enType;
        gs_SendParam[i].enVideoMode = enVdecMode;
        gs_SendParam[i].s32MinBufSize = stSize.u32Height * stSize.u32Width * 3/4;
        pthread_create(&gs_SendParam[i].Pid, NULL, SAMPLE_VDEC_SendStream, &gs_SendParam[i]);
    }

    if (PT_JPEG != enType)
    {
        printf("you can press ctrl+c to terminate program before normal exit.\n");
    }
    /******************************************
     step 7: join thread
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        pthread_join(gs_SendParam[i].Pid, 0);
        printf("join thread %d.\n", i);
    }

	// (void)HI_MPI_VDEC_ReleaseImage(0, pstFrameInfo);

    printf("press two enter to quit!\n");
    getchar();
    getchar();
    /******************************************
     step 8: Unbind vdec to vpss & destroy vdec-chn
    ******************************************/
END_3:
    for (i=0; i<s32Cnt; i++)
    {
        VdChn = i;
        SAMPLE_VDEC_WaitDestroyVdecChn(VdChn, enVdecMode);
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
        else
        {
            VoChn = i;
            SAMLE_COMM_VDEC_UnBindVo(VdChn, VoDev, VoChn);
        }
    }
    /******************************************
     step 9: stop vo
    ******************************************/
END_2:
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    for(i=0;i<u32WndNum;i++)
    {
        VoChn = i;
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
        }
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_1:
    if (HI_TRUE == bVoHd)
    {
        if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
        {
            SAMPLE_COMM_VO_HdmiStop();
        }
        SAMPLE_COMM_VPSS_Stop(s32Cnt, VPSS_MAX_CHN_NUM);
    }
    /******************************************
     step 10: exit mpp system
    ******************************************/
END_0:
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}


/****************************************************************************
* function: main
****************************************************************************/
int main(int argc, char* argv[])
{
    HI_S32 s32Index;
    if (argc != 2)
    {
        SAMPLE_VDEC_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);

    s32Index = atoi(argv[1]);

    switch (s32Index)
    {
        case 0: /* H264 -> VPSS -> VO(HD) */
            gs_s32Cnt = 1;
            SAMPLE_VDEC_Process(PIC_D1, PT_H264, gs_s32Cnt, SAMPLE_VO_DEV_DHD0);
            break;
        case 1: /* JPEG ->VPSS -> VO(HD) */
            gs_s32Cnt = 1;
            SAMPLE_VDEC_Process(PIC_D1, PT_JPEG, gs_s32Cnt, SAMPLE_VO_DEV_DHD0);
            break;
        case 2: /* MJPEG -> VO(SD) */
            gs_s32Cnt = 1;
            SAMPLE_VDEC_Process(PIC_D1, PT_MJPEG, gs_s32Cnt, SAMPLE_VO_DEV_DSD0);
            break;
        // case 3:  /* H264 -> VPSS -> VO(HD PIP PAUSE STEP)*/
        //     gs_s32Cnt = 1;
        //     if (HI_SUCCESS != SAMPLE_VDEC_ProcessForPip(PIC_D1, PT_H264, gs_s32Cnt, SAMPLE_VO_DEV_DHD0))
        //     {
        //         break;
        //     }
        //     break;
        default:
            printf("the index is invaild!\n");
            SAMPLE_VDEC_Usage(argv[0]);
            return HI_FAILURE;
        break;
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
