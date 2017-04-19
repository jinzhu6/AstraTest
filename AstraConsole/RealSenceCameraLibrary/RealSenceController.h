#pragma once

#include "opencv2/highgui.hpp" // basic opencv header. needed for all openCV functionality

namespace Intel
{
	namespace RealSense
	{
		class SenseManager;
		class Projection;
		class Image;
	}
}


class RealSenceController
{
public:
	RealSenceController();
	~RealSenceController();

	void Run(int numFrames, int saveVertexFrequency);
	void RunTillStopped();

	void SetShowIR(bool s) { m_ShowIr = s;  }
	void SetShowColour(bool s) { m_ShowColour = s; }
	void SetShowDepth(bool s) { m_ShowDepth = s; }
	void Stop() { m_Stop = true; }
	void SaveNextFrame(std::string fileName);

private:
	bool Initialise();
	void SaveVertexMap();
	void GetNextFrame();
	void Release();


private:
	Intel::RealSense::SenseManager* m_pipeline;
	Intel::RealSense::Projection* m_projection;
	Intel::RealSense::Image* m_lastDepthImage;
	Intel::RealSense::Image* m_lastColourImage;
	//short m_lowConfidanceDepthValue;

	bool m_ShowDepth;
	bool m_ShowColour;
	bool m_ShowIr;
	bool m_Stop;
	bool m_SaveNextFrame;
	std::string m_SaveFileName;

};
