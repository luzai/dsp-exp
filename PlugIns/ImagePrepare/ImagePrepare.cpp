// ImagePrepare.cpp : Defines the initialization routines for the DLL.

#include "stdafx.h"
#include "ImagePrepare.h"
//
#include "BufStruct.h"
#include "ImageProc.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//
/////////////////////////////////////////////////////////////////////////////
// CImagePrepareApp
BEGIN_MESSAGE_MAP(CImagePrepareApp, CWinApp)
//{{AFX_MSG_MAP(CImagePrepareApp)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG_MAP
END_MESSAGE_MAP()
//
/////////////////////////////////////////////////////////////////////////////
// CImagePrepareApp construction
CImagePrepareApp::CImagePrepareApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}
/////////////////////////////////////////////////////////////////////////////
// The one and only CImagePrepareApp object
CImagePrepareApp theApp;

char sInfo[] = "人脸跟踪-摄像头视频流图片截取处理插件";
bool bLastPlugin = false;

DLL_EXP void ON_PLUGIN_BELAST(bool bLast)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //模块状态切换
	bLastPlugin = bLast;
}
//插件名称
DLL_EXP LPCTSTR ON_PLUGININFO(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //模块状态切换
	return sInfo;
}
//
DLL_EXP void ON_INITPLUGIN(LPVOID lpParameter)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //模块状态切换
												 //theApp.dlg.Create(IDD_PLUGIN_SETUP);
												 //theApp.dlg.ShowWindow(SW_HIDE);
}
DLL_EXP int ON_PLUGINCTRL(int nMode, void *pParameter)
{
	//模块状态切换
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int nRet = 0;
	return nRet;
}

/****************************************************************************************/
/*                             摄像头视频流图片截取、重采样等处理                       */
/****************************************************************************************/

// extern "C" _declspec(ddlimport) void myHeapAllocInit(BUF_STRUCT *pBufStruct);
DLL_INP void myHeapAllocInit(BUF_STRUCT *pBufStruct);
DLL_INP DLL_EXP bool ReSample(aBYTE *ThisImage, int Width, int Height, //源图片指针及大小
							  int newWidth, int newHeight,			   //目标图片大小
							  bool InsMode,							   //true：双线性内插法，false：邻近点采样法
							  bool bGray,							   //true：灰度图片采样，false：彩色图片采样
							  aBYTE *result							   //目标图片指针
							  );

void copy_img(aBYTE *src_img, aBYTE *dst_img, int w, int h)
{
	int sum = w * h;
	for (int i = 0; i < sum; i++)
		dst_img[i] = src_img[i];
}

void InitTraceObject(TRACE_OBJECT *obj, char *mode)
{
	obj->rcObject = {0, 0, 0, 0};
	obj->fvObject->size = sizeof(FeatureVector4P) * 5;
	// todo
	for (int i = 0; i < 4; i++)
		obj->fvObject->Vector[i] = (0, 0);
	obj->fvObject_org->size = sizeof(FeatureVector4P) * 5;
	for (int i = 0; i < 4; i++)
		obj->fvObject_org->Vector[i] = (0, 0);
	obj->spdxObj = obj->spdyObj = 0;
	obj->nMinDist = 0x7fffffff;
	obj->bBrokenTrace = false;
	obj->bSaveit = false;
	obj->nBrokenTimes = 0;
	obj->sName = "lz-tr";
}

DLL_EXP void ON_PLUGINRUN(int w, int h, BYTE *pYBits, BYTE *pUBits, BYTE *pVBits, BYTE *pBuffer)
{
	//pYBits 大小为w*h
	//pUBits 和 pVBits 的大小为 w*h/2
	//pBuffer 的大小为 w*h*6
	//下面算法都基于一个假设，即w是16的倍数
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //模块状态切换

	int i, j;
	BUF_STRUCT *BUF = (BUF_STRUCT *)pBuffer;

	if (BUF->bNotInited)
	{
		BUF->colorBmp = pBuffer + sizeof(BUF_STRUCT);
		BUF->grayBmp = BUF->colorBmp;
		BUF->clrBmp_1d8 = BUF->grayBmp + w * h * 2;
		BUF->grayBmp_1d16 = BUF->clrBmp_1d8 + w * h / 4;
		BUF->TempImage1d8 = BUF->grayBmp_1d16 + w * h / 16;
		BUF->lastImageQueue1d16m8 = BUF->TempImage1d8 + w * h / 8;
		BUF->pOtherVars = (OTHER_VARS *)(BUF->lastImageQueue1d16m8 + w * h * 2);
		BUF->pOtherData = (aBYTE *)BUF->pOtherVars + sizeof(OTHER_VARS);
		for (i = 0; i <= 7; i++)
			BUF->pImageQueue[i] = BUF->lastImageQueue1d16m8 + i * (w * h / 16);

		BUF->W = w;
		BUF->H = h;
		BUF->cur_allocSize = 0;
		BUF->allocTimes = 0;
		BUF->cur_maxallocsize = 0;
		BUF->bLastEyeChecked = false;
		BUF->EyeBallConfirm = true;
		BUF->EyePosConfirm = true;
		BUF->nImageQueueIndex = -1;
		BUF->nLastImageIndex = -1;
		for (i = 0; i <= 255; i++)
		{
			if (i >= 85 && i <= 126)
				BUF->pOtherVars->byHistMap_U[i] = 1;
			else
				BUF->pOtherVars->byHistMap_U[i] = 0;
		}
		for (i = 0; i <= 255; i++)
		{
			if (i >= 130 && i < 165)
				BUF->pOtherVars->byHistMap_V[i] = 1;
			else
				BUF->pOtherVars->byHistMap_V[i] = 0;
		}

		InitTraceObject(&BUF->pOtherVars->objNose, "nosetrac");
		InitTraceObject(&BUF->pOtherVars->objLefteye, "Leyetrac");
		InitTraceObject(&BUF->pOtherVars->objRighteye, "Reyetrac");

		// 根据需求初始化跟踪体特征全局向量FeaProcBuf todo

		BUF->max_allocSize = w * h * 17 / 16 - sizeof(BUF_STRUCT) - sizeof(OTHER_VARS);
		myHeapAllocInit(BUF);
		BUF->bNotInited = false;
	}

	BUF->displayImage = pYBits;

	BYTE *p = BUF->colorBmp;
	copy_img(pYBits, p, w, h);
	p += w * h;
	copy_img(pUBits, p, w, h / 2);
	p += w * h / 2;
	copy_img(pVBits, p, w, h / 2);

	ReSample(BUF->colorBmp, w, h, w / 2, h / 4, true, false, BUF->clrBmp_1d8);

	ReSample(BUF->grayBmp, w, h, w / 4, h / 4, true, true, BUF->grayBmp_1d16);

	if (bLastPlugin)
	{
		// todo blastplugin? copytorect?
		CopyToRect(BUF->clrBmp_1d8, //源图片指针
				   pYBits,			//目标图片指针,用displayimage会报错
				   320, 120,		//区域图片大小
				   640, 480,		//目标图片大小
				   1, 1,			//区域位置:图片左上角在目标图片的坐标
				   false);
		CopyToRect(BUF->grayBmp_1d16, //源图片指针
				   pYBits,			  //目标图片指针
				   160, 120,		  //区域图片大小
				   640, 480,		  //目标图片大小
				   480, 1,			  //区域位置:图片左上角在目标图片的坐标
				   true);
	}
}
}

DLL_EXP void ON_PLUGINEXIT()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //模块状态切换
												 //theApp.dlg.DestroyWindow();
}
