// ImagePrepare.cpp : Defines the initialization routines for the DLL.

#include "stdafx.h"
#include "ImagePrepare.h"
//
#include "assert.h"
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
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//模块状态切换
	bLastPlugin = bLast;
}
//插件名称
DLL_EXP LPCTSTR ON_PLUGININFO(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//模块状态切换
	return sInfo;
}
//
DLL_EXP void ON_INITPLUGIN(LPVOID lpParameter)
{   
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//模块状态切换
	//theApp.dlg.Create(IDD_PLUGIN_SETUP);
	//theApp.dlg.ShowWindow(SW_HIDE);
}
DLL_EXP int ON_PLUGINCTRL(int nMode,void* pParameter)
{
//模块状态切换
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int nRet = 0;
	return nRet;
}

/****************************************************************************************/
/*                             摄像头视频流图片截取、重采样等处理                       */
/****************************************************************************************/
//DLL_INP void ShowDebugMessage(char* format,...);

void (*AddMessageToList)(char * message)=NULL;

DLL_EXP void InitMessageOpFunction(void (*AMTL)(char *))
{
	AddMessageToList = AMTL;
}

DLL_EXP void ShowDebugMessage(char* format,...)
{
	va_list ap;
	char sBuffer[1024];
	va_start(ap, format);
	vsprintf(sBuffer,format,ap);
	va_end(ap);
	if( AddMessageToList )
		(*AddMessageToList)(sBuffer);
}

DLL_INP aBYTE *myHeapAlloc(int size);

DLL_INP bool ReSample(aBYTE *ThisImage, int Width, int Height, int newWidth, int newHeight, bool InsMode, bool bGray, aBYTE *result);
//DLL_INP void CopyToRect(aBYTE *ThisImage,aBYTE *anImage, int W, int H, int DestW, int DestH, int nvLeft, int nvTop,bool bGray);

void copy_img(aBYTE *src_img, aBYTE *dst_img, int w, int h)
{
	int sum = w * h;
	memcpy(dst_img,src_img,sum);
}
DLL_EXP void InitFeatureVector(FeatureVector *pThis)
{
	int i;
	ASSERT(pThis);
	FeatureVector4P *pVector = (FeatureVector4P*)pThis->Vector;
	pVector->pNL_LeftTop = (FeatureVector4P*)((aBYTE*)pVector+sizeof(FeatureVector4P));
	pVector->pNL_RightTop = (FeatureVector4P*)((aBYTE*)pVector+2*sizeof(FeatureVector4P));
	pVector->pNL_LeftBottom = (FeatureVector4P*)((aBYTE*)pVector+3*sizeof(FeatureVector4P));
	pVector->pNL_RightBottom = (FeatureVector4P*)((aBYTE*)pVector+4*sizeof(FeatureVector4P));
	pThis->size = sizeof(FeatureVector4P)*5;
	pVector->nLevels = 2;
	pVector->pNL_LeftTop->nLevels = 1;
	pVector->pNL_RightTop->nLevels = 1;
	pVector->pNL_LeftBottom->nLevels = 1;
	pVector->pNL_RightBottom->nLevels = 1;
	for(i=0; i<4; i++)
	{
		pVector->pNL_LeftTop->Vector[i].x = pVector->pNL_LeftTop->Vector[i].y = 0;
		pVector->pNL_RightTop->Vector[i].x = pVector->pNL_RightTop->Vector[i].y = 0;
		pVector->pNL_LeftBottom->Vector[i].x = pVector->pNL_LeftBottom->Vector[i].y = 0;
		pVector->pNL_RightBottom->Vector[i].x = pVector->pNL_RightBottom->Vector[i].y = 0;
	}
}

void InitTraceObject(TRACE_OBJECT *pThis, char name[8])
{
	memset(&pThis->rcObject, 0, sizeof(aRect));
	pThis->spdxObj = pThis->spdyObj = 0;
	pThis->nMinDist = 0x7fffffff;
	pThis->bBrokenTrace = false;
	pThis->nBrokenTimes = 0;
	pThis->bSaveit = false;
	InitFeatureVector(&pThis->fvObject_org);
	strncpy(pThis->sName, name, 8);
	pThis->sName[7] = 0;
}

DLL_EXP void CopyToRect(
	         aBYTE* ThisImage,//区域图片指针
			 aBYTE* anImage,//目标图片指针
			 int W,int H,//区域图片大小
		     int DestW,int DestH,//目标图片大小
			 int nvLeft,int nvTop,//区域位置：图片左上角在目标图片的坐标
			 bool bGray//灰度图片标记，true：灰度；false：彩色。
					  )
{
    aBYTE  * lpSrc = ThisImage;
    aBYTE  * lpDes = anImage;
    aBYTE  * lps, * lpd;
    int h;
	ASSERT(ThisImage && anImage);
    ASSERT( nvLeft+W<=DestW && nvTop+H<=DestH && nvLeft>=0 && nvTop>=0 );
	//Y
    for(h=0;h<H;h++){
        lpd = lpDes+(nvTop+h)*DestW+nvLeft;
        lps = lpSrc+(DWORD)h*W;
		memcpy(lpd,lps,W);
    }
	if( bGray )	return;
	//U
	lpSrc += W*H;
	lpDes += DestW*DestH;
    for(h=0;h<H;h++){
        lpd = lpDes+(nvTop+h)*(DestW/2)+nvLeft/2;
        lps = lpSrc+(DWORD)h*W/2;
		memcpy(lpd,lps,W/2);
    }
	//V
	lpSrc += W*H/2;
	lpDes += DestW*DestH/2;
    for(h=0;h<H;h++){
        lpd = lpDes+(nvTop+h)*(DestW/2)+nvLeft/2;
        lps = lpSrc+(DWORD)h*W/2;
		memcpy(lpd,lps,W/2);
    }
}


DLL_EXP void ON_PLUGINRUN(int w,int h,BYTE* pYBits,BYTE* pUBits,BYTE* pVBits,BYTE* pBuffer)
{
//pYBits 大小为w*h
//pUBits 和 pVBits 的大小为 w*h/2
//pBuffer 的大小为 w*h*6
//下面算法都基于一个假设，即w是16的倍数
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//模块状态切换
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
		BUF->pOtherVars = (OTHER_VARS *)(BUF->lastImageQueue1d16m8 + w * h / 2);
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

		ShowDebugMessage("hi\n");
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
		ShowDebugMessage("hi\n");
		CopyToRect(BUF->grayBmp_1d16, //源图片指针
				   pYBits,			  //目标图片指针
				   160, 120,		  //区域图片大小
				   640, 480,		  //目标图片大小
				   0, 0,			  //区域位置:图片左上角在目标图片的坐标
				   true);
		CopyToRect(BUF->clrBmp_1d8, //源图片指针
				   pYBits,			//目标图片指针,用displayimage会报错
				   320, 120,		//区域图片大小
				   640, 480,		//目标图片大小
				   w/2, 0,			//区域位置:图片左上角在目标图片的坐标
				   false);
		
	}
    
}



DLL_EXP void ON_PLUGINEXIT()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//模块状态切换
	//theApp.dlg.DestroyWindow();
}
