#pragma once
class VmTimer
{
public:
	VmTimer();
	~VmTimer();

	void CalculateFrameStats();
	
	void Tick();
	void Start();
	void Stop();
	void Reset();

	float TotalTime() const;
	float GetDeltaTime() const { return (float)m_dDeltaTime; }

private:
	//std::wostringstream m_sTitleString;

	double m_dSecondPerCount;
	double m_dDeltaTime;

	__int64 m_nBaseTime;
	__int64 m_nPausedTime;
	__int64 m_nStopTime;
	__int64 m_nPrevTime;
	__int64 m_nCurrTime;

	bool m_bStopped;
};

