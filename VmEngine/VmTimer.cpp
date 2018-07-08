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

	// ���� �����Ӱ��� ����
	m_dDeltaTime = (m_nCurrTime - m_nPrevTime) * m_dSecondPerCount;
	m_nPrevTime = m_nCurrTime;

	// DeltaTime�� ������ ���� �ʰ� �ϴ� ó��.
	// ���μ����� ���� ���� ���ų� �ٸ� ���μ����� ��Ű�� ��� deltaTime�� ������ �� �� �ִ�.
	// (DX11 å ����)
	if (m_dDeltaTime < 0.0)
	{
		m_dDeltaTime = 0.0;
	}
}

void VmTimer::Start()
{
	__int64 nStartTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&nStartTime);
	
	// �ߴ� ���¿��� ������ϴ� ���
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
	// Reset ȣ�� ���� �帥 �ð�. (�Ͻ������� �ð��� ����)
	if (m_bStopped)
	{
		return (float)(((m_nStopTime - m_nPausedTime) - m_nBaseTime) * m_dSecondPerCount);
	}
	else
	{
		return (float)(((m_nCurrTime - m_nPausedTime) - m_nBaseTime) * m_dSecondPerCount);
	}
	
}
