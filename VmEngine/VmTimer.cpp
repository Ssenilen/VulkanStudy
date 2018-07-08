#include "stdafx.h"
#include "VmTimer.h"


VmTimer::VmTimer() :
	m_dSecondPerCount(0), m_dDeltaTime(0), m_nBaseTime(0), m_nPausedTime(0),
m_nStopTime(0), m_nPrevTime(0), m_nCurrTime(0)
{
	__int64 nCountsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&nCountsPerSec);
	m_dSecondPerCount = 1.0 / (double)nCountsPerSec;
}


VmTimer::~VmTimer()
{
}

void VmTimer::CalculateFrameStats()
{
}

void VmTimer::Tick()
{
	if (m_bStopped)
	{
		m_dDeltaTime = 0.0;
		return;
	}

	__int64 nCurrTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&nCurrTime);
	m_nCurrTime = nCurrTime;

	// 이전 프레임과의 격차
	m_dDeltaTime = (m_nCurrTime - m_nPrevTime) * m_dSecondPerCount;
	m_nPrevTime = m_nCurrTime;

	// DeltaTime이 음수가 되지 않게 하는 처리.
	// 프로세서가 절전 모드로 들어가거나 다른 프로세서와 엉키는 경우 deltaTime이 음수가 될 수 있다.
	// (DX11 책 참고)
	if (m_dDeltaTime < 0.0)
	{
		m_dDeltaTime = 0.0;
	}
}

void VmTimer::Start()
{
	__int64 nStartTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&nStartTime);
	
	// 중단 상태에서 재시작하는 경우
	if (m_bStopped)
	{
		m_nPausedTime += (nStartTime - m_nStopTime);

		m_nPrevTime = nStartTime;
		m_nStopTime = 0;
		m_bStopped = false;
	}
}

void VmTimer::Stop()
{
	if (!m_bStopped)
	{
		__int64 nCurrTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&nCurrTime);

		m_nStopTime = nCurrTime;
		m_bStopped = true;
	}
}

void VmTimer::Reset()
{
	__int64 nCurrTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&nCurrTime);

	m_nBaseTime = nCurrTime;
	m_nPrevTime = nCurrTime;
	m_nStopTime = 0;
	m_bStopped = false;
}

float VmTimer::TotalTime() const
{
	// Reset 호출 이후 흐른 시간. (일시정지된 시간은 제외)
	if (m_bStopped)
	{
		return (float)(((m_nStopTime - m_nPausedTime) - m_nBaseTime) * m_dSecondPerCount);
	}
	else
	{
		return (float)(((m_nCurrTime - m_nPausedTime) - m_nBaseTime) * m_dSecondPerCount);
	}
	
}
