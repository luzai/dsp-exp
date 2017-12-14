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

int RegionLabel(aBYTE *tempImg, int width, int height)
{
	int i, classnum, L = 0;
	int Lmax, Lmin;
	int LK[1024], LK1[1024]; //�ȼ۱�
	int Left = 0, Up = 0;
	for (i = 0; i < 1024; i++)
		LK[i] = i;
	WORD *tempLabel = (WORD *)myHeapAlloc(width * height * sizeof(WORD)); //WORD�ͱ���Ϊ�����ֽ�

	for (i = 0; i < width * height; i++)
	{
		if (tempImg[i] != 0)
		{
			if (i < width || tempImg[i - width] == 0)
			{
				if (i % width == 0 || tempImg[i - 1] == 0) //���Ҳ�Ǻڵ�,�ж�һ��������
					tempLabel[i] = ++L;
				else
					tempLabel[i] = tempLabel[i - 1];
			}
			else
			{
				if (i % width == 0 || tempImg[i - 1] == 0)
					tempLabel[i] = tempLabel[i - width]; //�����ϱ߱��
				else if (tempLabel[i - width] == tempLabel[i - 1])
					tempLabel[i] = tempLabel[i - width];
				else
				{
					tempLabel[i] = tempLabel[i - 1];
					if (Up != tempLabel[i - width] && Left != tempLabel[i - 1])
					{
						Lmax = tempLabel[i - 1];
						Lmin = tempLabel[i - width];
						while (Lmax != LK[Lmax])
							Lmax = LK[Lmax];
						while (Lmin != LK[Lmin])
							Lmin = LK[Lmin];
						LK[Lmax] = Lmin;
						Up = tempLabel[i - width];
						Left = tempLabel[i - 1];
					}
				}
			}
		}
		else
			tempLabel[i] = 0;
	}
	for (i = 1; i < 1024; i++)
	{
		Lmax = LK[i];
		while (Lmax != LK[Lmax])
			Lmax = LK[Lmax];
	}
	//��ͨ�������·���
	for (LK1[0] = 0, i = 1, classnum = 1; i <= L; i++)
		if (i == LK[i])
			LK1[i] = classnum++;
		else
			LK1[i] = LK1[LK[i]];
	//ͼƬ����,ע��������ͨ���Ų��ܴ���255
	for (i = 0; i < width * height; i++)
		tempImg[i] = LK1[tempLabel[i]];
	myHeapFree((aBYTE *)tempLabel);
	return 0;
}

aRect get_bbox(aBYTE *src, int w, int h, int value, aBYTE *dst)
{
	int minx = w, maxx = h, miny = 0, maxy = 0;
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			if (src[j * w + i] == value)
			{
				if (!dst)
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
			else if (!dst)
				dst[j * w + i] = 0;
		}
	}
	aRect res = {minx,miny,maxx-minx,maxy-miny}; 
	return res;
}
void CopyToRect(
	         aBYTE* ThisImage,//����ͼƬָ��
			 aBYTE* anImage,//Ŀ��ͼƬָ��
			 int W,int H,//����ͼƬ��С
		     int DestW,int DestH,//Ŀ��ͼƬ��С
			 int nvLeft,int nvTop,//����λ�ã�ͼƬ���Ͻ���Ŀ��ͼƬ������
			 bool bGray//�Ҷ�ͼƬ��ǣ�true���Ҷȣ�false����ɫ��
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

