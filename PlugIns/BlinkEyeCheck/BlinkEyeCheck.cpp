// BlinkEyeCheck.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "BlinkEyeCheck.h"
#include "BufStruct.h"
#include "ImageProc.h"

//
#define MAX_EYE_AMP         1
#define MAX_EYE_Y_DIFF      (3*MAX_EYE_AMP)//5
#define MIN_EYE_X_DIFF      (15*MAX_EYE_AMP)//4
#define MAX_EYE_X_DIFF      (40*MAX_EYE_AMP)//30
#define MAX_EYE_SIZE        (150*MAX_EYE_AMP)//200
//#define MIN_EYE_SIZE        (20*MAX_EYE_AMP)//200
//
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
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
// CBlinkEyeCheckApp

BEGIN_MESSAGE_MAP(CBlinkEyeCheckApp, CWinApp)
	//{{AFX_MSG_MAP(CBlinkEyeCheckApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBlinkEyeCheckApp construction
CBlinkEyeCheckApp::CBlinkEyeCheckApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBlinkEyeCheckApp object

CBlinkEyeCheckApp theApp;
//
#define PC_MODE
/***************************************************************
��Щ����ֻ��PC���д��ڣ��������
��DSP�汾�в�������
***************************************************************/
#ifdef PC_MODE
aBYTE open_eye_left[32*24*2];
aBYTE open_eye_right[32*24*2];
aBYTE close_eye_left[32*24];
aBYTE close_eye_right[32*24];
aBYTE st_nose[32*48*2];
#endif



char sInfo[] = "��������-գ�ۼ��������λ������";

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
	AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
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


void copy_img(aBYTE *src_img, aBYTE *dst_img, int w, int h)
{
	int sum = w * h;
	for (int i = 0; i < sum; i++)
		dst_img[i] = src_img[i];
}


aBYTE conv(aBYTE *img, double *mask, int row, int col, int w, int h)
{
	double sum = 0;

	for (int i = -1; i < 2; i++)
	{
		if (row + i < 0 || row + i >= w)
			continue;
		for (int j = -1; j < 2; j++)
		{
			if (col + j < 0 || col + 1 >= h)
				continue;
			sum = sum + img[(row + i) * w + h + j] * mask[(i + 1) * 3 + j + 1];
		}
	}

	return (aBYTE)sum;
}

void gs_filter(aBYTE *gray, int width, int height)
{
	double mask[9] = {0, 1.0 / 7, 0, 1.0 / 7, 3.0 / 7, 1.0 / 7, 0, 1, 0};
	aBYTE *temp_img = myHeapAlloc(width * height);

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
			temp_img[i * width + j] = conv(gray,mask, i, j, width, height);
	}

	copy_img(temp_img, gray, width, height);

	myHeapFree(temp_img);
}
aRect get_bbox(aBYTE *src, int w, int h, int value)
{
	int minx = w, maxx = h, miny = 0, maxy = 0;
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			if (src[j * w + i] == value)
			{				
				if (i < minx)
					minx = i;
				if (i > maxx)
					maxx = i;
				if (j < miny)
					miny = j;
				if (j > maxy)
					maxy = j;
			}
			
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

void Erosion(aBYTE *tempImg, int width, int height, int N1, int N2)
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
		for (j = N1 / 2; j < width - N1 / 2; j++)
		{
			flag = 1;
			for (k = 0; k < N1; k++)
				if (tempImg[i * width + j - N1 / 2 + k] == 0)
				{
					//һ���ڽṹԪ�������ҵ�һ����ɫ����,��Ǹ�ʴ���˳�ѭ��
					flag = 0;
					break;
				}
			tempImg1[i * width + j] = flag ? 255 : 0; //����flag�����Ƿ�ʴ
		}
	}

	memcpy(tempImg, tempImg1, width * height * sizeof(aBYTE));

	for (i = N2 / 2; i < height - N2 / 2; i++)
	{
		for (j = 0; j < width; j++)
		{
			flag = 1;
			for (k = 0; k < N2; k++)
			{
				if (tempImg1[(i - N2 / 2 + k) * width + j] == 0)
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

void Dilation(aBYTE *tempImg, int width, int height, int N1, int N2)
{
	int i, j, k, flag;
	aBYTE *tempImg2 = myHeapAlloc(width * height * sizeof(aBYTE));
	memcpy(tempImg2, tempImg, width * height);
	for (i = 0; i < height; i++)
	{
		for (j = N1 / 2; j < width - N1 / 2; j++)
		{
			flag = 0;
			for (k = 0; k < N1; k++)
			{
				if (tempImg[i * width + j - N1 / 2 + k] == 255)
				{
					flag = 1;
					break;
				}
			}
			tempImg2[i * width + j] = flag ? 255 : 0;
		}
	}
	memcpy(tempImg, tempImg2, width * height * sizeof(aBYTE));

	int N = N2;
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


bool thresh_img(aBYTE *img, int w, int h)
{
	int N0 = 50, N1 = 10; // todo tuning
	int sum = 0, thresh = 255,i;
	int n[256] = {0};

	for ( i = 0; i < w * h; i++)
	{
		n[img[i]]++;
	}
	for (thresh = 255 ; thresh > 0; thresh--)
	{
		sum += n[thresh];
		if (sum > N0)
			break;
	}

	for ( i = 0; i < sum; i++)
	{
		img[i] = (img[i] > thresh) ? 255 : 0;
	}

	if (thresh > N1)
		return true;
	else
		return false;
}

void diff_img(aBYTE *last_img, aBYTE *this_img, int w, int h, aBYTE *dst)
{
	for (int i = 0; i < w * h; i++)
	{
		if (last_img[i] < this_img[i])
			dst[i] = 0;
		else
			dst[i] = last_img[i] - this_img[i];
	}
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
typedef struct vector_
{
	int *data;
	int size;
	int count;
} vector;
void vector_init(vector *v)
{
    v->size = 10;
    v->data = (int*)malloc(sizeof(int) * v->size);
    memset(v->data, '\0', sizeof(int) * v->size);
    v->count = 0;
}

int vector_count(vector *v)
{
    return v->count;
}


void vector_add(vector *v, int e)
{
    if (v->size == 0) {
        v->size = 10;
        v->data = (int*)malloc(sizeof(int) * v->size);
        memset(v->data, '\0', sizeof(int) * v->size);
    }

    if (v->size == v->count) {
        v->size *= 2;
        v->data = (int*)realloc(v->data, sizeof(int) * v->size);
    }

    v->data[v->count] = e;
    v->count++;
}

void vector_set(vector *v, int index, int e)
{
    if (index >= v->count) {
        return;
    }

    v->data[index] = e;
}

int vector_get(vector *v, int index)
{
    if (index >= v->count) {
        return NULL;
    }

    return v->data[index];
}

void vector_delete(vector *v, int index)
{
	int i,j; 
    if (index >= v->count) {
        return;
    }

    for ( i = index, j = index; i < v->count; i++) {
        v->data[j] = v->data[i];
        j++;
    }

    v->count--;
}

int vector_sum(vector* v)
{
	int s = 0,i;
	for ( i = 0; i < vector_count(v); i++)
	{
		s += (int)vector_get(v, i);
	}
	return s;
}
void vector_free(vector *v)
{
    free(v->data);
}
 
/*******************************************************************/
//գ�ۼ�����۾���λ���
/*******************************************************************/
DLL_EXP void ON_PLUGINRUN(int w, int h, BYTE *pYBits, BYTE *pUBits, BYTE *pVBits, BYTE *pBuffer)
{
	//pYBits ��СΪw*h
	//pUBits �� pVBits �Ĵ�СΪ w*h/2
	//pBuffer �Ĵ�СΪ w*h*4
	//�����㷨������һ�����裬 ��w��16�ı���
	AFX_MANAGE_STATE(AfxGetStaticModuleState()); //ģ��״̬�л�
												 //���д��Ӧ�������
	BUF_STRUCT *pBS = (BUF_STRUCT *)pBuffer;
	BUF_STRUCT *BUF = pBS;
	int i,j,x,y; 
	// ���и�˹�˲�
	gs_filter(pBS->grayBmp_1d16, w / 4, h / 4);

	// ��ͼ�������ʷͼ��
	if (pBS->nImageQueueIndex == -1)
	{
		for (int i = 0; i < 8; i++)
			copy_img(pBS->grayBmp_1d16, pBS->pImageQueue[i], w / 4, h / 4);
	}
	else
	{
		copy_img(pBS->grayBmp_1d16, pBS->pImageQueue[(pBS->nImageQueueIndex + 1) % 8], w / 4, h / 4);
		pBS->nImageQueueIndex++;
		pBS->nImageQueueIndex = pBS->nImageQueueIndex % 8;
		pBS->nLastImageIndex = (pBS->nImageQueueIndex - 3) % 8;
	}
	diff_img(pBS->pImageQueue[pBS->nImageQueueIndex],
			 pBS->pImageQueue[pBS->nLastImageIndex],
			 w / 4, h / 4,
			 pBS->TempImage1d8);
	CopyToRect(pBS->TempImage1d8, BUF->displayImage,
		w/4,h/4,w,h,0,0,true); 
	bool blink = thresh_img(pBS->TempImage1d8, w / 4, h / 4);
	if (blink == false)
		return ;

	aBYTE *tempImg = pBS->TempImage1d8;

	const int MAX_K = 256;
	int w4 = w / 4;
	int h4 = h / 4;
	float center_x[MAX_K] = {0.0}, center_y[MAX_K] = {0.0};
	float size[MAX_K] = {0.0}, N[MAX_K] = {0.0};
	vector region_x[MAX_K], region_y[MAX_K];

	int max_k = 0,k;

	for ( k = 0; k < MAX_K; k++)
	{
		vector_init(&region_x[k]);
		vector_init(&region_y[k]);
	}

	Erosion(tempImg, w4, h4, 3, 1);
	Dilation(tempImg, w4, h4, 3, 1);
	RegionLabel(tempImg, w4, h4);
	for ( i = 0; i < w4 * h4; i++)
	{
		max_k = (tempImg[i] > max_k)? tempImg[i] : max_k ;
		N[tempImg[i]] = N[tempImg[i]]+ 1;
	}

	ASSERT(max_k <= MAX_K);

	for ( y = 0; y <= h4; y++)
	{
		for ( x = 0; x <= w4; x++)
		{
			int k = tempImg[y * w4 + x];
			vector_add(&region_x[k], x);
			vector_add(&region_y[k], y);
		}
	}

	for ( k = 1; k <= max_k; k++)
	{
		aRect bbox = get_bbox(tempImg, w4, h4, k);
		size[k] = bbox.width * bbox.width + bbox.height * bbox.height;
		center_x[k] = vector_sum(&region_x[k]) / N[k];
		center_y[k] = vector_sum(&region_y[k]) / N[k];
	}

	bool find_eyes = false;
	for (i = 0; i < max_k; i++)
	{
		for (j = i + 1; j < max_k; j++)
		{
			if (abs(center_y[i] - center_y[i]) < 4 &&
				abs(center_x[i] - center_y[j]) > 15 &&
				abs(center_x[i] - center_x[i] < 30) &&
				size[i] < 200 &&
				size[j] < 200)
			{
				find_eyes = true;
			}
		}
	}
	if (find_eyes == false)
		return;
	if (center_x[i] > center_y[j]) // i -- left, small -- left
	{
		int tmp; 
		tmp=i; i=j; j=tmp; 
	}
		

	aPOINT pt_left_eye, pt_right_eye;

	pt_left_eye.x = center_x[i];
	pt_left_eye.y = center_y[i];
	pt_right_eye.x = center_x[j];
	pt_right_eye.y = center_y[j];

	DrawCross(BUF->displayImage, w, h,
			  pt_left_eye.x * 4,
			  pt_left_eye.y * 4,
			  5,
			  TYUV(59, 15, 10),
			  false);
	DrawCross(BUF->displayImage, w, h,
			  pt_right_eye.x * 4,
			  pt_right_eye.y * 4,
			  5,
			  TYUV(59, 15, 10),
			  false);

	pt_left_eye.x *= 4;
	pt_left_eye.y *= 4;
	pt_right_eye.x *= 4;
	pt_right_eye.y *= 4;
	aPOINT pt_nose;
	pt_nose.x = (pt_left_eye.x + pt_right_eye.x) / 2;
	pt_nose.y = pt_left_eye.y;
	float n_eye_dist = abs(pt_left_eye.x - pt_right_eye.x);
	float n_eye_width = 2.0 / 3 * n_eye_dist;
	float n_eye_height = 1.0 / 2 * n_eye_dist;

	aRect eye = get_bbox(tempImg, w4, h4, i);
	if (eye.width * 4 > n_eye_width || eye.height * 4 > n_eye_height)
		return ;
	eye = get_bbox(tempImg, w4, h4, j);
	if (eye.width * 4 > n_eye_width || eye.height * 4 > n_eye_height)
		return ;

	if	(!BUF->bLastEyeChecked ||
		!BUF->pOtherVars->objLefteye.bBrokenTrace ||
		!BUF->pOtherVars->objRighteye.bBrokenTrace ||
		!BUF->pOtherVars->objNose.bBrokenTrace) return;

	float n_nose_width = 3.0 / 4 * n_eye_dist;
	float n_nose_height = n_eye_dist;

	aRect rcv_left_eye = {int(pt_left_eye.x - n_eye_width / 2),
						  int(pt_left_eye.y - n_eye_height / 2),
						  int(n_eye_width / 4) * 4,
						  int(n_eye_height / 4) * 4};

	aRect rcv_right_eye = {int(pt_right_eye.x - n_eye_width / 2),
						   int(pt_right_eye.y - n_eye_height / 2),
						   int(n_eye_width / 4) * 4,
						   int(n_eye_height / 4) * 4};

	aRect rcv_nose = { int(pt_nose.x - n_nose_width / 2),
					   int(pt_nose.y - n_nose_height / 2),
					   int(n_nose_width / 4) * 4,
					   int(n_nose_height / 4) * 4  } ;
	
	BUF->pOtherVars->objLefteye.rcObject=rcv_left_eye; 
	BUF->pOtherVars->objRighteye.rcObject=rcv_right_eye; 
	BUF->pOtherVars->objNose.rcObject=rcv_nose;  

	DrawHLine(BUF->displayImage, BUF->W, BUF->H, pt_left_eye.x-5, pt_left_eye.x+5,
		pt_left_eye.y, RGB(200,12,45),false);  
	DrawVLine(BUF->displayImage, BUF->W, BUF->H, pt_left_eye.y-5, pt_left_eye.y+5, 
		pt_left_eye.x, RGB(200,12,45),false);

}

/*******************************************************************/

/*******************************************************************/              
DLL_EXP void ON_PLUGINEXIT()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());//ģ��״̬�л�
	//theApp.dlg.DestroyWindow();
}

