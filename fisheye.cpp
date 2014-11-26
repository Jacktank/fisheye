/************************************************************************
*	fisheye calibration
*	author ZYF
*	date 2014/11/22
************************************************************************/

#include "fisheye.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using cv::Point2f;
using std::asin;

#ifndef PI
#define PI (3.1415926)
#endif
/************************************************************************/
/* PointMap
/* ��һ������ͼ���ϵĵ�ӳ�䵽����ͼ���ϵ�һ����
/************************************************************************/
void PointMap(Point2f sp, Point2f &dp, float r)
{
	PointMap(sp.x,sp.y,dp.x,dp.y,r);
}

/************************************************************************/
/* PointMap
/* ��һ������ͼ���ϵĵ�ӳ�䵽����ͼ���ϵ�һ����,ʹ��������ϵ��ƽ��
/* ������
/*		x,y: ���������������ͼ���ϵĵ������
/*		new_x, new_y : �������������ͼ���ϵ������
/*		r : �����������뾶
/************************************************************************/
void PointMap(float x, float y, float& new_x, float& new_y, float r)
{
	//l is the distance between the point (x,y) to origin
	float l = sqrt(x*x + y*y);

	float alpha(0);
	if ( 0 == x) 
		alpha = PI / 2;
	else
		alpha = atan( y / x);

	float theta = l / r;
	float d = r * sin(theta);

	float tx = d * cos(alpha);
	float ty = d * sin(alpha);

	if ( x > 0)
		new_x = abs(tx);
	else if (x < 0)
		new_x = -abs(tx);
	else
		new_x = 0;

	if (y > 0)
		new_y = abs(ty);
	else if (y < 0)
		new_y = -abs(ty);
	else
		new_y = 0;
}

/************************************************************************/
/* PointMap2
/* ��һ������ͼ���ϵĵ�ӳ�䵽����ͼ���ϵ�һ���㣬ʹ��γ�Ȳ��䷨ӳ��
/* ������
/*		x,y: ���������������ͼ���ϵĵ������
/*		new_x, new_y : �������������ͼ���ϵ������
/*		r : ���������Բ�뾶
/************************************************************************/
void PointMap2(float x, float y, float& new_x, float& new_y, float r)
{
	float theta_x = x / r;
	float xx = r * sin(theta_x);
	float theta_y = y / r;
	float yy = r * sin(theta_y);

	//��������xx,yy
	float scale = 1.0f; // x,y��������ű�����Ĭ��Ϊ1�������˲�����ı�ӳ����
	int iters = 1;
	for (int i = 0; i < iters; ++i) {
		float rr = sqrt(r*r - yy*yy);
		float xx1 = rr * xx / r;
		rr = sqrt( r*r - xx*xx);
		float yy1 = rr * yy / r;
		xx = xx1; yy = yy1;
	}

	if (x == 0)
		new_x = 0;
	else
		new_x = (x > 0 ? 1 : -1) * abs(xx);

	if (y == 0)
		new_y = 0;
	else
		new_y = (y > 0 ? 1 : -1) * abs(yy);
}

/************************************************************************/
/* ����Ӵ�����ͼ�񵽻���ͼ�������ӳ�����  
������
	r �� Բ�뾶��ȡֵ600����Ч���ȽϺ�
/************************************************************************/
void RectifyMap(Mat& mapx, Mat& mapy, float r /* = 600 */)
{
	//int width = ceil(PI * r / 2) * 2;
	int width = 1000; //ӳ��ͼ��Ŀ��
	float s = 480.0f / 640.0f; //ͼ��ߺͿ�ı���
	int height = width * s;
	int center_x = width / 2, center_y = height / 2;
	
	mapx.create(height,width,CV_32F);
	mapy.create(height,width,CV_32F);

	for (int i = 0; i < height; ++i) {
		float y = center_y - i;
		float* px = (float*)(mapx.data + i * mapx.step);
		float* py = (float*)(mapy.data + i * mapy.step);
		for (int j = 0; j < width; ++j) {
			float x = j - center_x;
			float nx,ny;
			PointMap2(x,y,nx,ny,r);
			PointMap(nx,ny,nx,ny,300);
			px[j] = nx;
			py[j] = ny;
		}
	}
}

/************************************************************************/
/* ����ͼ��                                                                     */
/************************************************************************/
void UndisImage(Mat distort_image, Mat& undistort_image, Mat mapx, Mat mapy)
{
	assert(mapx.rows == mapy.rows && mapy.cols == mapy.cols);

	int height = distort_image.rows;
	int width = distort_image.cols;

	float cx = width / 2;
	float cy = height / 2;
	//cx = 320; cy = 260; //����ͼ�������λ��

	int un_height = mapx.rows;
	int un_width = mapy.cols;
	float center_x = un_width / 2;
	float center_y = un_height / 2;
	undistort_image.create(un_height,un_width,distort_image.type());
	undistort_image.setTo(0);
	int channel = undistort_image.channels();
	cv::Mat_<cv::Vec3b> _distrot_image = distort_image;
	cv::Mat_<float> _mapx = mapx;
	cv::Mat_<float> _mapy = mapy;

	for (int i = 0; i < un_height; ++i) {
		uchar* pdata = undistort_image.data + i * undistort_image.step;
		//float* pmapx = (float*)(mapx.data + i * mapx.step);
		//float* pmapy = (float*)(mapy.data + i * mapy.step);
		for (int j = 0; j < un_width; ++j) {
// 			if ((i - center_y)*(i - center_y) + (j - center_x)*(j - center_x) > un_width * un_width / 4) {
// 				continue;
// 			}
			//int x = pmapx[j] + cx;
			//int y = cy - pmapy[j];
			int x = _mapx(i,j) + cx;
			int y = cy - _mapy(i,j);
			if ((x < 0 || x >= width || y < 0 || y >= height)) {
				continue;
			}
			if ((x - cx)*(x - cx) + (y - cy)*(y - cy) > 290*290)
				continue;
			for (int k = 0; k < channel; ++k) {

				pdata[j * channel + k] = _distrot_image(y,x)[k];
			}
		}
	}
	cv::resize(undistort_image,undistort_image,distort_image.size());
}