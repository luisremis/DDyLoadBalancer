/* ============================================================
University of Illinois at Urbana Champaign
Breadth-First-Search Optimized
Author: Luis Remis
Date:	Apr 5 2015
============================================================ */

#include "ChronoCpu.h"

#include <cstring>
#include <iostream>

using namespace std;

// *****************************************************************************
// Public methods definitions
// *****************************************************************************
ChronoCpu::ChronoCpu(const string& name) :
		 Chrono     (name)
		,ticCounter (0)
{
	memset((void*)&lastTicTime, 0, sizeof(lastTicTime));
	memset((void*)&ticTime, 0, sizeof(ticTime));
	memset((void*)&tacTime, 0, sizeof(tacTime));
}

ChronoCpu::~ChronoCpu(void)
{
}


// *****************************************************************************
// Private/Protected methods definitions
// *****************************************************************************
void ChronoCpu::doTic(void)
{
	lastTicTime = ticTime;

	if (clock_gettime(CLOCK_REALTIME, &ticTime) != 0){
		++errors;
		cerr << "ChronoCpu::doTic - " << name << ": clock_gettime() failed!" << endl;
		return;
	}

	++ticCounter;

	if (ticCounter > 1){
		float period_s = (float)(ticTime.tv_sec - lastTicTime.tv_sec);
		float period_ns = (float)(ticTime.tv_nsec - lastTicTime.tv_nsec);
		periodStats.lastTime_ms = period_s * 1e3f + period_ns / 1e6f;
		updateStats(periodStats);
	}
}

void ChronoCpu::doTac(void)
{
	if (clock_gettime(CLOCK_REALTIME, &tacTime) != 0){
		++errors;
		cerr << "ChronoCpu::doTac - " << name << ": clock_gettime() failed!" << endl;
		return;
	}

	float elapsed_s = (float)(tacTime.tv_sec - ticTime.tv_sec);
	float elapsed_ns = (float)(tacTime.tv_nsec - ticTime.tv_nsec);
	elapsedStats.lastTime_ms = elapsed_s * 1e3f + elapsed_ns / 1e6f;
	updateStats(elapsedStats);
}

