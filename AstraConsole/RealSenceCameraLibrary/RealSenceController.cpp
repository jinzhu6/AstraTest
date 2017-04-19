#include "stdafx.h"
#include "RealSenceController.h"
#include "IImageDisplayer.h"

#include "RealSense/SenseManager.h"
#include "RealSense/SampleReader.h"

#include "opencv2/highgui.hpp" // basic opencv header. needed for all openCV functionality
#include <opencv2/imgproc.hpp> // needed for overlay on images 

#include <iostream>
#include <fstream>

using namespace Intel::RealSense;

RealSenceController::RealSenceController()
	: m_pipeline(NULL)
	, m_lastColourImage(NULL)
	, m_lastDepthImage(NULL)
	, m_projection(NULL)
	, m_ShowColour(false)
	, m_ShowDepth(false)
	, m_ShowIr(false)
	, m_Stop(false)
	, m_SaveNextFrame(false)
	, m_SaveFileName("RealVertex.csv")
//	, m_lowConfidanceDepthValue(0)
{
}


RealSenceController::~RealSenceController()
{
}

void RealSenceController::Run(int numFrames, int saveVertexFrequency)
{
	if (!Initialise())
	{
		return;
	}

	for (int frameCount = 0; frameCount < numFrames; frameCount++)
	{
		if ((frameCount % saveVertexFrequency) == 0)
		{
			SaveVertexMap();
		}
		GetNextFrame();
		std::cout << "Frame: " << frameCount << "\n";
	}
	Release();
}

void RealSenceController::RunTillStopped()
{
	if (!Initialise())
	{
		return;
	}

	while (!m_Stop)
	{
		if (m_SaveNextFrame)
		{
			SaveVertexMap();
			m_SaveNextFrame = false;
		}
		GetNextFrame();
	}
	Release();
}





void RealSenceController::SaveVertexMap()
{
	if (NULL == m_lastDepthImage || NULL == m_lastColourImage)
	{
		return;
	}

	ImageInfo dI = m_lastDepthImage->QueryInfo();

	int numPoints = dI.width * dI.height;
	std::vector<Point3DF32> vertices(numPoints);
	Status status = m_projection->QueryVertices(m_lastDepthImage, &vertices[0]);
	std::vector<PointF32> uvMap(numPoints);
	status = m_projection->QueryUVMap(m_lastDepthImage, &uvMap[0]);

	ImageInfo cI = m_lastColourImage->QueryInfo();
	ImageData ddata;
	m_lastColourImage->AcquireAccess(Image::ACCESS_READ, Image::PIXEL_FORMAT_RGBA, &ddata);


	std::ofstream file(m_SaveFileName, std::ios::out);

	file << "X,Y,Z,R,G,B\n";

	int* colourPixel = (int*)ddata.planes[0];
	for (int p = 0; p < numPoints; ++p)
	{
		Point3DF32 vertex = vertices[p];
		if (vertex.z != 0)
		{
			if (uvMap[p].x >= 0 && uvMap[p].x < 1.0f && uvMap[p].y >= 0 && uvMap[p].y < 1.0f)
			{
				int x = (int)(uvMap[p].x * cI.width);
				int y = (int)(uvMap[p].y * cI.height);
				file << vertex.x << ',' << vertex.y << ',' << vertex.z << ',';
				int pixelValue = *(colourPixel + y*cI.width + x);
				for (int c = 0; c < 3; c++)
				{
					int colour = (int)(pixelValue & 0xFF);
					file << colour << ',';
					pixelValue = pixelValue >> 8;
				}
				file << "\n";
			}
		}
	}
	m_lastColourImage->ReleaseAccess(&ddata);
}

void RealSenceController::GetNextFrame()
{
	/* Waits until new frame is available and locks it for application processing */
	Status status = m_pipeline->AcquireFrame(false);

	if (status == Status::STATUS_NO_ERROR)
	{
		/* Render streams, unless -noRender is selected */
		const Capture::Sample *sample = m_pipeline->QuerySample();
		if (sample)
		{
			if (m_ShowIr && (NULL != sample->ir))
			{
				IImageDisplayer::ShowImage(sample->ir, "IR");
			}
			if (NULL != sample->depth)
			{
				m_lastDepthImage = sample->depth;
				if (m_ShowDepth)
				{
					IImageDisplayer::ShowImage(sample->depth, "Depth");
				}

			}
			if (NULL != sample->color)
			{
				m_lastColourImage = sample->color;
				if (m_ShowColour)
				{
					IImageDisplayer::ShowImage(sample->color, "Colour");
				}
			}

			cv::waitKey(1); // refresh

		}
	}


	/* Releases lock so pipeline can process next frame */
	m_pipeline->ReleaseFrame();
}

void RealSenceController::Release()
{
	m_pipeline->Release();
}


void RealSenceController::SaveNextFrame(std::string fileName)
{ 
	m_SaveFileName = fileName;
	m_SaveNextFrame = true; 
}

bool RealSenceController::Initialise()
{
	/* Creates an instance of the SenseManager */
	m_pipeline = SenseManager::CreateInstance();
	if (!m_pipeline)
	{
		std::cout << L"Unable to create the SenseManager\n";
		return false;
	}


	m_pipeline->EnableStream(Capture::STREAM_TYPE_COLOR, 640, 480);
	m_pipeline->EnableStream(Capture::STREAM_TYPE_DEPTH);
	m_pipeline->EnableStream(Capture::STREAM_TYPE_IR);
	Status status = m_pipeline->Init();
	if (STATUS_NO_ERROR != status)
	{
		return false;
	}

	DataDesc desc = {};
	desc.deviceInfo.streams = Capture::STREAM_TYPE_COLOR | Capture::STREAM_TYPE_DEPTH;

	Capture::Device *device = m_pipeline->QueryCaptureManager()->QueryDevice();
	device->ResetProperties(Capture::STREAM_TYPE_ANY);

	//m_lowConfidanceDepthValue = m_pipeline->QueryCaptureManager()->QueryDevice()->QueryDepthLowConfidenceValue();
	m_projection = m_pipeline->QueryCaptureManager()->QueryDevice()->CreateProjection();

	return (STATUS_NO_ERROR == status);
}
