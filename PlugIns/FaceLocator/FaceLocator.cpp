// FaceLocator.cpp : Defines the initialization routines for the DLL.
#include "stdafx.h"
#include "FaceLocator.h"
#include "BufStruct.h"
#include "ImageProc.h"

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
// CFaceLocatorApp
BEGIN_MESSAGE_MAP(CFaceLocatorApp, CWinApp)
	//{{AFX_MSG_MAP(CFaceLocatorApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CFaceLocatorApp construction
CFaceLocatorApp::CFaceLocatorApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}
/////////////////////////////////////////////////////////////////////////////
// The one and only CFaceLocatorApp object
CFaceLocatorApp theApp;
char sInfo[] = "��������-���ڲ�ɫ��Ϣ�������ָ����";
bool bLastPlugin = false;
DLL_EXP void ON_PLUGIN_BELAST(bool bLast)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	bLastPlugin = bLast;
}
DLL_EXP LPCTSTR ON_PLUGININFO(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	return sInfo;
}
DLL_EXP void ON_INITPLUGIN(LPVOID lpParameter)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	//theApp.dlg.Create(IDD_PLUGIN_SETUP);
	//theApp.dlg.ShowWindow(SW_HIDE);
}
DLL_EXP int ON_PLUGINCTRL(int nMode,void* pParameter)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int nRet = 0;
	switch(nMode)
	{
	case 0:
		{
			//theApp.dlg.ShowWindow(SW_SHOWNORMAL);
			//theApp.dlg.SetWindowPos(NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
		}
		break;
	}
	return nRet;
}
/****************************************************************************************/
/*                             ��������붨λ                                           */
/****************************************************************************************/


void Erosion(aBYTE *tempImg, int width, int height, int N)
{
	int i, j, k, flag;
	//����һ����ʱ����
	aBYTE *tempImg1 = myHeapAlloc(width * height);
	//�ȸ�����ָ��ָ�������,�����߽�����
	memcpy(tempImg1, tempImg, width * height * sizeof(aBYTE));
	//�д���
	for (i = 0; i < height; i++)
	{
		//Ϊ�˷�ֹԽ��,������߽�n/2������
		for (j = N / 2; j < width - N / 2; j++)
		{
			flag = 1;
			for (k = 0; k < N; k++)
				if (tempImg[i * width + j - N / 2 + k] == 0)
				{
					//һ���ڽṹԪ�������ҵ�һ����ɫ����,��Ǹ�ʴ���˳�ѭ��
					flag = 0;
					break;
				}
			tempImg1[i * width + j] = flag ? 255 : 0; //����flag�����Ƿ�ʴ
		}
	}

	memcpy(tempImg, tempImg1, width * height * sizeof(aBYTE));

	for (i = N / 2; i < height - N / 2; i++)
	{
		for (j = 0; j < width; j++)
		{
			flag = 1;
			for (k = 0; k < N; k++)
			{
				if (tempImg1[(i - N / 2 + k) * width + j] == 0)
				{
					flag = 0;
					break;
				}
			}
			tempImg[i * width + j] = flag ? 255 : 0;
		}
	}
	myHeapFree(tempImg1);
}

void Dilation(aBYTE *tempImg, int width, int height, int N)
{
	int i, j, k, flag;
	aBYTE *tempImg2 = myHeapAlloc(width * height * sizeof(aBYTE));
	memcpy(tempImg2, tempImg, width * height);
	for (i = 0; i < height; i++)
	{
		for (j = N / 2; j < width - N / 2; j++)
		{
			flag = 0;
			for (k = 0; k < N; k++)
			{
				if (tempImg[i * width + j - N / 2 + k] == 255)
				{
					flag = 1;
					break;
				}
			}
			tempImg2[i * width + j] = flag ? 255 : 0;
		}
	}
	memcpy(tempImg, tempImg2, width * height * sizeof(aBYTE));
	for (i = N / 2; i < height - N / 2; i++)
	{
		for (j = 0; j < width; j++)
		{
			flag = 0;
			for (k = 0; k < N; k++)
			{
				if (tempImg2[(i - N / 2 + k) * width + j] == 255)
				{
					flag = 1;
					break;
				}
			}
			tempImg[i * width + j] = flag ? 255 : 0;
		}
	}
	myHeapFree(tempImg2);
}


aRect get_bbox(aBYTE *src, int w, int h, int value, aBYTE *dst)
{
	int minx = w, maxx = 0, miny = h, maxy = 0;
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			if (src[j * w + i] == value)
			{
				dst[j * w + i] = 255;
				if (i < minx)
					minx = i;
				if (i > maxx)
					maxx = i;
				if (j < miny)
					miny = j;
				if (j > maxy)
					maxy = j;
			}
			else
				dst[j * w + i] = 0;
		}
	}
	aRect res = {minx,miny,maxx-minx,maxy-miny}; 
	return res;
}

DLL_IMP int RegionLabel(aBYTE *tempImg, int width, int height);

DLL_INP void CopyRectTo(aBYTE* ThisImage,  //src img 
				aBYTE* anImage, // dest img 
				int Width, int Height,  //src img size 
				aRect rcvRect, // src img bbox 
				bool bGray);
DLL_INP void CopyToRect(
	         aBYTE* ThisImage,//����ͼƬָ��
			 aBYTE* anImage,//Ŀ��ͼƬָ��
			 int W,int H,//����ͼƬ��С
		     int DestW,int DestH,//Ŀ��ͼƬ��С
			 int nvLeft,int nvTop,//����λ�ã�ͼƬ���Ͻ���Ŀ��ͼƬ������
			 bool bGray//�Ҷ�ͼƬ��ǣ�true���Ҷȣ�false����ɫ��
					  );
DLL_EXP void ON_PLUGINRUN(int w,int h,BYTE* pYBits,BYTE* pUBits,BYTE* pVBits,BYTE* pBuffer)
{
//pYBits ��СΪw*h
//pUBits �� pVBits �Ĵ�СΪ w*h/2
//pBuffer �Ĵ�СΪ w*h*4
//�����㷨������һ�����裬��w��16�ı���

	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	
      //���д��Ӧ�������
	int i, j, max;
	int pixelnum[256] = {0};
	int minx, miny, maxx, maxy;

	BUF_STRUCT *BUF = (BUF_STRUCT *)pBuffer;
	aBYTE *tempImg = myHeapAlloc(w * h / 16);

	BYTE *pU = BUF->clrBmp_1d8 + w * h / 8;
	BYTE *pV = pU + w * h / 16;

	for (i = 0; i < w * h / 16; i++)
		tempImg[i] = 255 * BUF->pOtherVars->byHistMap_U[pU[i]] * BUF->pOtherVars->byHistMap_V[pV[i]];
	
	Erosion(tempImg, w / 4, h / 4, 3);
	Dilation(tempImg, w / 4, h / 4, 3);

	Dilation(tempImg, w / 4, h / 4, 7);
	Erosion(tempImg, w / 4, h / 4, 7);
	if (bLastPlugin){
		CopyToRect(tempImg,
			   pYBits,
			   160, 120,
			   640, 480,
			   480, 0,
			   true);
		//myHeapFree(tempImg);
		//return;
	}
	RegionLabel(tempImg, w / 4, h / 4);
	//5��������λ����ʾ
	//ͳ�Ƹ���ͨ����������
	for (i = 0; i < w * h / 16; i++)
		pixelnum[tempImg[i]]++;

	max = 1;
	for (i = 2; i < 256; i++)
		if (pixelnum[i] > pixelnum[max])
			max = i;
		
	aRect res = get_bbox(tempImg, w/4,h/4, max, tempImg) ; 

	BUF->rcnFace.height = res.height;
	BUF->rcnFace.left = 2 * res.left;
	BUF->rcnFace.top = res.top;
	BUF->rcnFace.width = 2 * res.width;
	BUF->nFacePixelNum = max; 
	
	//ReSample(tempImg, w/2, h/4, w/4, h/4, true, true, BUF->clrBmp_1d8); 
	for (i = 0; i < w * h / 8; i++)
		BUF->clrBmp_1d8[i] = tempImg[i / 2];

	if (bLastPlugin){
		aRect rcn = {BUF->rcnFace.left * 2,
					 BUF->rcnFace.top * 4,
					 BUF->rcnFace.width * 2,
					 BUF->rcnFace.height * 4};
		for (i=0;i<3;i++){
			DrawRectangle(
				pYBits, w, h,
				rcn,
				TYUV(59, 15, 10),  
				false);
			rcn.left+=1;
			rcn.top+=1;
			rcn.width-=2;
			rcn.height-=2;
		}
		CopyToRect(BUF->clrBmp_1d8, //ԴͼƬָ��
				   pYBits,			//Ŀ��ͼƬָ��,��displayimage�ᱨ��
				   320, 120,		//����ͼƬ��С
				   640, 480,		//Ŀ��ͼƬ��С
				   0, 0,			//����λ��:ͼƬ���Ͻ���Ŀ��ͼƬ������
				   false);
	}
	myHeapFree(tempImg);
}

DLL_EXP void ON_PLUGINEXIT()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	//theApp.dlg.DestroyWindow();
}

