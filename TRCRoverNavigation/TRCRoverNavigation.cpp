#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <math.h>
#include <iostream>
#define PI 3.14159265
#define ScreenWidth 1280//512
#define ScreenHeight 720//384
using namespace cv;
using namespace std;

FILE* file;

// Main Variable
float angleW_A = 90;
float angleH_A = 90;
float actualDistance_A = 0;
float angleW_B = 90;
float angleH_B = 90;
float actualDistance_B = 0;

// Label Matrix
int currentLabel = 1;
int label[ScreenWidth][ScreenHeight];

// Blob Centre
int centerAreaX_A = 0;
int centerAreaY_A = 0;
int sizeA = 0;
int centerAreaX_B = 0;
int centerAreaY_B = 0;
int sizeB = 0;

// Edge coordinate
int leftA = 0, rightA = 0, topA = 0, bottomA = 0;
int leftB = 0, rightB = 0, topB = 0, bottomB = 0;

// HSV Variable
int HLow = 45, SLow = 100, VLow = 100;//int HLow = 45, SLow = 100, VLow = 100;
int HHigh = 75, SHigh = 255, VHigh = 255;//int HHigh = 75, SHigh = 255, VHigh = 255;

// Morphology Variable
int morph_elem = 0;
int morph_size = 3;
int morph_operator = 0;
int const max_operator = 4;
int const max_elem = 2;
int const max_kernel_size = 21;

// Blob Angle Measure Variable
const float d_ruler = 6.3;//6.0
const float w_ruler = 4.1;//3.7;
const float h_ruler = 2.45;//2.35;
const float angleOfVisionR = atan(h_ruler / d_ruler);
const float h_cone_max = 22.2;
const float d_cone_max = h_cone_max / tan(angleOfVisionR);

VideoCapture cap;
Mat capture, colour;

int deviceID = 0;
int apiID = CAP_ANY;


void resetVaraible();
int setupCamera();
void imageProcessing();
void resetLable();
void BlobLabel_1();
void BlobLabel_2();
void plotLabel(int);
void colourSecondBlob();
void calculateBlobCentre();
void calculatePositionAngle();
void plotBlobCentre();
void checkHeightAndWidth();
void plotBoundary();
void calculateDistance();
void swapBlob();
void showImage();


int iter = 0;
int main()
{
	// Setup Camera
	setupCamera();
	if (!cap.isOpened()) {
		cerr << "ERROR! Unable to open camera\n";
		return -1;
	}


	// Main Loop
	int mode = 1;
	//std::cout << "Start Capturing" << std::endl;
	while (true)
	{
		cout << iter << "\n";
		iter++;
		resetVaraible();

		if (mode == 1)
		{
			cap.read(capture);

			imageProcessing();
			resetLable();
			BlobLabel_1();
			BlobLabel_2();
			colourSecondBlob();
			calculateBlobCentre();

			if (sizeA > 0)
			{
				calculatePositionAngle();
				checkHeightAndWidth();
				calculateDistance();
				plotBlobCentre();
				plotBoundary();
				swapBlob();
			}

			showImage();
			std::cout << std::endl;

			/*file = fopen("Checkpoint3/turning.dat", "w");
			if (file != NULL)
			{
			if (angleW > 5)
			{
			fprintf(file, "%3f", 1);
			}
			else if (angleW < -5)
			{
			fprintf(file, "%3f", -1);
			}
			else
			{
			fprintf(file, "%3f", 0);
			}
			}
			fclose(file);*/
		}
		else if (mode == 2)
		{
			cap.read(capture);
			imshow("capture", capture);
		}
		else if (mode == 3)
		{
			cap.read(capture);
			GaussianBlur(capture, capture, Size(7, 7), 1.5, 1.5);
			imshow("capture", capture);
		}

		// Keyboard Input Change Mode
		int key = cv::waitKey(5);
		key = (key == 255) ? -1 : key;
		if (key == 113)
		{
			break;
		}
		if (key == 49)
		{
			mode = 1;
		}
		else if (key == 50)
		{
			mode = 2;
		}
		else if (key == 51)
		{
			mode = 3;
		}
	}
	std::cin.get();
	return 0;
}

void resetVaraible()
{
	angleW_A = 90;
	angleH_A = 90;
	actualDistance_A = 0;
	angleW_B = 90;
	angleH_B = 90;
	actualDistance_B = 0;

	// Label Matrix
	currentLabel = 1;
	label[ScreenWidth][ScreenHeight];

	// Blob Centre
	centerAreaX_A = 0;
	centerAreaY_A = 0;
	sizeA = 0;
	centerAreaX_B = 0;
	centerAreaY_B = 0;
	sizeB = 0;

	// Edge coordinate
	leftA = 0, rightA = 0, topA = 0, bottomA = 0;
	leftB = 0, rightB = 0, topB = 0, bottomB = 0;
}

int setupCamera()
{
	cap.open(deviceID, apiID);

	if (!cap.isOpened())
	{
		std::cout << "Failed to Open Camera" << std::endl;
		return -1;
	}
	cap.set(CAP_PROP_FRAME_WIDTH, ScreenWidth);
	cap.set(CAP_PROP_FRAME_HEIGHT, ScreenHeight);

	namedWindow("capture", 1);

	return 0;
}

void imageProcessing()
{
	//std::cout << "Start Image Processing" << std::endl;
	cvtColor(capture, colour, COLOR_BGR2HSV);
	inRange(colour, Scalar(HLow, SLow, VLow), Scalar(HHigh, SHigh, VHigh), colour);
	Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));
	morphologyEx(colour, colour, 3, element);
	morphologyEx(colour, colour, 2, element);
}

void resetLable()
{
	currentLabel = 1;
	for (int y = 0; y < colour.rows; y++)
	{
		for (int x = 0; x < colour.cols; x++)
		{
			label[x][y] = 0;
		}
	}
}

void BlobLabel_1()
{
	//std::cout << "Start Blob Label Scan 1" << std::endl;
	for (int y = 0; y < colour.rows; y++)
	{
		const uchar *ptr = colour.ptr(y);
		for (int x = 0; x < colour.cols; x++)
		{
			const uchar *pixel = ptr;
			if (pixel[0] == 255)
			{
				if (label[x - 1][y - 1] != 0)
				{
					label[x][y] = label[x - 1][y - 1];
				}
				else if (label[x][y - 1] != 0)
				{
					label[x][y] = label[x][y - 1];
				}
				else if (label[x + 1][y - 1] != 0)
				{
					label[x][y] = label[x + 1][y - 1];
				}
				else if (label[x - 1][y] != 0)
				{
					label[x][y] = label[x - 1][y];
				}
				else
				{
					label[x][y] = currentLabel;
					currentLabel++;
				}
			}
			ptr += 1;
		}
	}
}

void BlobLabel_2()
{
	//std::cout << "Start Blob Label Scan 2" << std::endl;
	for (int y = 0; y < ScreenHeight; y++)
	{
		for (int x = (ScreenWidth - 1); x > -1; x--)
		{
			if (x < ScreenWidth && y > 0 && label[x + 1][y - 1] > 0 && label[x + 1][y - 1] < label[x][y])
			{
				label[x][y] = label[x + 1][y - 1];
			}
			else if (y > 0 && label[x][y - 1] > 0 && label[x][y - 1] < label[x][y])
			{
				label[x][y] = label[x][y - 1];
			}
			else if (x > 0 && y > 0 && label[x - 1][y - 1] > 0 && label[x - 1][y - 1] < label[x][y])
			{
				label[x][y] = label[x + 1][y - 1];
			}
			else if (x < ScreenWidth && label[x + 1][y] > 0 && label[x + 1][y] < label[x][y])
			{
				label[x][y] = label[x + 1][y];
			}
		}
	}
	//std::cout << "Start Blob Label Scan 3" << std::endl;
	for (int y = (ScreenHeight)-1; y > 0; y--)
	{
		for (int x = 0; x < ScreenWidth; x++)
		{
			if (x > 0 && y < ScreenHeight && label[x - 1][y + 1] > 0 && label[x - 1][y + 1] < label[x][y])
			{
				label[x][y] = label[x - 1][y + 1];
			}
			else if (y < ScreenHeight &&label[x][y + 1] > 0 && label[x][y + 1] < label[x][y])
			{
				label[x][y] = label[x][y - 1];
			}
			else if (x < ScreenWidth && y < ScreenHeight &&label[x + 1][y + 1] > 0 && label[x + 1][y + 1] < label[x][y])
			{
				label[x][y] = label[x + 1][y + 1];
			}
			else if (x>0 && label[x - 1][y] > 0 && label[x - 1][y] < label[x][y])
			{
				label[x][y] = label[x - 1][y];
			}
		}
	}
}

void plotLabel(int iter)
{
	//std::cout << "Ploting Label" << std::endl;
	if (iter == 49)
	{
		for (int y = 0; y < ScreenHeight; y++)
		{
			for (int x = 0; x < ScreenWidth; x++)
			{
				std::cout << label[x][y];
			}
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}

void colourSecondBlob()
{
	//std::cout << "Start colour minor blob" << std::endl;
	for (int y = 0; y < ScreenHeight; y++)
	{
		for (int x = 0; x < ScreenWidth; x++)
		{
			if (label[x][y] > 1)
			{
				uchar *ptr = colour.ptr(y);
				ptr += x;
				uchar *pixel = ptr;
				pixel[0] = 128;
			}
		}
	}
}

/*void colourSecondBlob()
{
	std::cout << "Start colour minor blob" << std::endl;
	int secBlobLabel, secCentreX, secCentreY;

	if (sizeA < sizeB)
	{
		secBlobLabel = label[centerAreaX_A][centerAreaY_A];
		secCentreX = centerAreaX_A;
		secCentreY = centerAreaY_A;
	}
	else
	{
		secBlobLabel = label[centerAreaX_B][centerAreaY_B];
		secCentreX = centerAreaX_B;
		secCentreY = centerAreaY_B;
	}
	
	for (int y = secCentreY; y < ScreenHeight; y++)
	{
		for (int x = secCentreX; x < ScreenWidth; x++)
		{
			if (label[x][y] == secBlobLabel)
			{
				uchar *ptr = colour.ptr(y);
				ptr += x;
				uchar *pixel = ptr;
				pixel[0] = 128;
			}
		}
	}
	for (int y = secCentreY; y > 0; --y)
	{
		for (int x = secCentreX; x > 0; --x)
		{
			if (label[x][y] == secBlobLabel)
			{
				uchar *ptr = colour.ptr(y);
				ptr += x;
				uchar *pixel = ptr;
				pixel[0] = 128;
			}
		}
	}
}*/

void calculateBlobCentre()
{
	//std::cout << "Calculate Blob Centre of Area" << std::endl;

	// Calculate major blob
	double m10A = 0;
	double m01A = 0;
	double m00A = 0;

	for (int y = 0; y < colour.rows; ++y)
	{
		const uchar *ptr = colour.ptr(y);
		for (int x = 0; x < colour.cols; x++)
		{
			const uchar *pixel = ptr;
			if (pixel[0] == 255)
			{
				m10A += y;
				m01A += x;
				m00A += 1;
			}
			ptr += 1;
		}
	}

	centerAreaX_A = m01A / m00A;
	centerAreaY_A = m10A / m00A;
	sizeA = m00A;

	if (!(centerAreaX_A > 0 && centerAreaX_A < ScreenWidth && centerAreaY_A > 0 && centerAreaY_A < ScreenHeight))
	{
		centerAreaX_A = -1;
		centerAreaY_A = -1;
	}

	//std::cout << "Blob A Centre X: " << centerAreaX_A << " Blob A Centre Y: " << centerAreaY_A << " Size A:" << sizeA << std::endl;

	// Calculate minor blob
	double m10B = 0;
	double m01B = 0;
	double m00B = 0;

	for (int y = 0; y < colour.rows; ++y)
	{
		const uchar *ptr = colour.ptr(y);
		for (int x = 0; x < colour.cols; x++)
		{
			const uchar *pixel = ptr;
			if (pixel[0] == 128)
			{
				m10B += y;
				m01B += x;
				m00B += 1;
			}
			ptr += 1;
		}
	}

	centerAreaX_B = m01B / m00B;
	centerAreaY_B = m10B / m00B;
	sizeB = m00B;

	if (!(centerAreaX_B > 0 && centerAreaX_B < ScreenWidth && centerAreaY_B > 0 && centerAreaY_B < ScreenHeight))
	{
		centerAreaX_B = -1;
		centerAreaY_B = -1;
	}

	//std::cout << "Blob B Centre X: " << centerAreaX_B << " Blob B Centre Y: " << centerAreaY_B << " Size B:" << sizeB << std::endl;
}

void calculatePositionAngle()
{
	if (centerAreaX_A != -1 && centerAreaY_A != -1)
	{
		float centerAAreaXcm = -(ScreenWidth / 2 - (float)centerAreaX_A)*((float)w_ruler / (ScreenWidth / 2));
		float centerAAreaYcm = (ScreenHeight / 2 - (float)centerAreaY_A)*((float)h_ruler / (ScreenHeight / 2));
		angleW_A = atan(centerAAreaXcm / d_ruler)*(360 / 2 / PI);
		angleH_A = atan(centerAAreaYcm / d_ruler)*(360 / 2 / PI);
		//std::cout << "AngleW_A: " << angleW_A << " AngleH_A: " << angleH_A << std::endl;
	}

	if (centerAreaX_B != -1 && centerAreaY_B != -1)
	{
		float centerBAreaXcm = -(ScreenWidth / 2 - (float)centerAreaX_B)*((float)w_ruler / (ScreenWidth / 2));
		float centerBAreaYcm = (ScreenHeight / 2 - (float)centerAreaY_B)*((float)h_ruler / (ScreenHeight / 2));
		angleW_B = atan(centerBAreaXcm / d_ruler)*(360 / 2 / PI);
		angleH_B = atan(centerBAreaYcm / d_ruler)*(360 / 2 / PI);
		//std::cout << "AngleW_B: " << angleW_B << " AngleH_B: " << angleH_B << std::endl;
	}
}

void plotBlobCentre()
{
	// Plot major blob center
	if (centerAreaY_A > 5 && centerAreaX_A > 5 && centerAreaX_A < ScreenWidth - 5 && centerAreaY_A < ScreenHeight - 5)
	{
		//std::cout << "Plot Blob A Centre of Area" << std::endl;
		for (int y = centerAreaY_A - 5; y < centerAreaY_A + 5; y++)
		{
			for (int x = centerAreaX_A - 5; x < centerAreaX_A + 5; x++)
			{
				colour.at<uchar>(y, x) = 8;
			}
		}
	}


	// Plot minor blob center
	if (centerAreaY_B > 5 && centerAreaX_B > 5 && centerAreaX_B < ScreenWidth - 5 && centerAreaY_B < ScreenHeight - 5)
	{
		//std::cout << "Plot Blob B Centre of Area" << std::endl;
		for (int y = centerAreaY_B - 5; y < centerAreaY_B + 5; y++)
		{
			for (int x = centerAreaX_B - 5; x < centerAreaX_B + 5; x++)
			{
				colour.at<uchar>(y, x) = 8;
			}
		}
	}
}

void checkHeightAndWidth()
{
	//std::cout << "Check Height and Width" << std::endl;
	if (centerAreaY_A > 5 && centerAreaX_A > 5 && centerAreaX_A < ScreenWidth - 5 && centerAreaY_A < ScreenHeight - 5)
	{
		for (int y = centerAreaY_A; y > 0; y--)
		{
			if ((int)colour.at<uchar>(y, centerAreaX_A) == 0)
			{
				topA = y;
				break;
			}
		}
		for (int y = centerAreaY_A; y < ScreenHeight - 1; y++)
		{
			if ((int)colour.at<uchar>(y, centerAreaX_A) == 0)
			{
				bottomA = y;
				break;
			}
		}
		for (int x = centerAreaX_A; x > 0; x--)
		{
			if ((int)colour.at<uchar>(centerAreaY_A, x) == 0)
			{
				leftA = x;
				break;
			}
		}
		for (int x = centerAreaX_A; x < ScreenWidth - 1; x++)
		{
			if ((int)colour.at<uchar>(centerAreaY_A, x) == 0)
			{
				rightA = x;
				break;
			}
		}
	}

	if (centerAreaY_B > 5 && centerAreaX_B > 5 && centerAreaX_B < ScreenWidth - 5 && centerAreaY_B < ScreenHeight - 5)
	{
		for (int y = centerAreaY_B; y > 0; y--)
		{
			if ((int)colour.at<uchar>(y, centerAreaX_B) == 0)
			{
				topB = y;
				break;
			}
		}
		for (int y = centerAreaY_B; y < ScreenHeight - 1; y++)
		{
			if ((int)colour.at<uchar>(y, centerAreaX_B) == 0)
			{
				bottomB = y;
				break;
			}
		}
		for (int x = centerAreaX_B; x > 0; x--)
		{
			if ((int)colour.at<uchar>(centerAreaY_B, x) == 0)
			{
				leftB = x;
				break;
			}
		}
		for (int x = centerAreaX_B; x < ScreenWidth - 1; x++)
		{
			if ((int)colour.at<uchar>(centerAreaY_B, x) == 0)
			{
				rightB = x;
				break;
			}
		}
	}
}

void plotBoundary()
{
	//std::cout << "Plot Boundery" << std::endl;
	if (centerAreaX_A != -1 && centerAreaY_A != -1)
	{
		if (topA > 3 && topA < ScreenHeight - 3)
		{
			for (int y = topA - 3; y < topA + 3; y++)
			{
				for (int x = centerAreaX_A - 3; x < centerAreaX_A + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (bottomA < ScreenHeight - 3 && bottomA >3)
		{
			for (int y = bottomA - 3; y < bottomA + 3; y++)
			{
				for (int x = centerAreaX_A - 3; x < centerAreaX_A + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (leftA > 3 && leftA < ScreenWidth - 3)
		{
			for (int y = centerAreaY_A - 3; y < centerAreaY_A + 3; y++)
			{
				for (int x = leftA - 3; x < leftA + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (rightA < ScreenWidth - 3 && rightA > 3)
		{
			for (int y = centerAreaY_A - 3; y < centerAreaY_A + 3; y++)
			{
				for (int x = rightA - 3; x < rightA + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
	}


	if (centerAreaX_B != -1 && centerAreaY_B != -1)
	{
		if (topB > 3 && topB < ScreenHeight - 3)
		{
			for (int y = topB - 3; y < topB + 3; y++)
			{
				for (int x = centerAreaX_B - 3; x < centerAreaX_B + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (bottomB < ScreenHeight - 3 && bottomB >3)
		{
			for (int y = bottomB - 3; y < bottomB + 3; y++)
			{
				for (int x = centerAreaX_B - 3; x < centerAreaX_B + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (leftB > 3 && leftB < ScreenWidth - 3)
		{
			for (int y = centerAreaY_B - 3; y < centerAreaY_B + 3; y++)
			{
				for (int x = leftB - 3; x < leftB + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
		if (rightB < ScreenWidth - 3 && rightB > 3)
		{
			for (int y = centerAreaY_B - 3; y < centerAreaY_B + 3; y++)
			{
				for (int x = rightB - 3; x < rightB + 3; x++)
				{
					colour.at<uchar>(y, x) = 64;
				}
			}
		}
	}
}

void calculateDistance()
{
	//std::cout<<"Start Calculate distance" << std::endl;
	if (centerAreaX_A != -1 && centerAreaY_A != -1)
	{
		int width_pxA = rightA - leftA;
		int height_pxA = bottomA - topA;
		float projectedHeightA = (float)((float)height_pxA*h_cone_max / (ScreenHeight / 2));
		float projectedAngleA = atan(projectedHeightA / d_cone_max);
		actualDistance_A = h_cone_max / tan(projectedAngleA);
	}

	if (centerAreaX_B != -1 && centerAreaY_B != -1)
	{
		int width_pxB = rightB - leftB;
		int height_pxB = bottomB - topB;
		float projectedHeightB = (float)((float)height_pxB*h_cone_max / (ScreenHeight / 2));
		float projectedAngleB = atan(projectedHeightB / d_cone_max);
		actualDistance_B = h_cone_max / tan(projectedAngleB);
	}
}

void swapBlob()
{
	if (sizeA < sizeB)
	{
		int tempDistance, tempHAngle, tempWAngle;
		tempDistance = actualDistance_A;
		tempHAngle = angleH_A;
		tempWAngle = angleW_A;

		actualDistance_A = actualDistance_B;
		angleH_A = angleH_B;
		angleW_A = angleW_B;

		actualDistance_B = tempDistance;
		angleH_B = tempHAngle;
		angleW_B = tempWAngle;
	}

	std::cout << "DistanceA: " << actualDistance_A << "cm Horizontal Angle A: " << angleW_A << " Vertical Angle A: " << angleH_A << std::endl;
	std::cout << "Distance B: " << actualDistance_B << "cm Horizontal Angle B: " << angleW_B << " Vertical Angle B: " << angleH_B << std::endl;
}

void showImage()
{
	imshow("colour", colour);
	imshow("capture", capture);
}


