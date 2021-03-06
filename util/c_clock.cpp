#include "stdafx.h"

// Copyright(c) 2012 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// c_clock.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// c_clock.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with c_clock.cpp.  If not, see <http://www.gnu.org/licenses/>. 
#include <time.h>

#include <iostream>

using namespace std;

#include "c_clock.h"

static const char * strWday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char * strMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

const char * getMonthStr(int month)
{
	if(month < 0 || month >= 12)
		return NULL;
	return strMonth[month];
}

const char * getWeekStr(int wday)
{
	if(wday < 0 || wday >= 7)
		return NULL;

	return strWday[wday];
}

void gmtimeex(long long msec, tmex & tm)
{
	time_t sec = (time_t) (msec / 1000);
	struct tm * ptm = gmtime(&sec);
	tm.tm_hour = ptm->tm_hour;
	tm.tm_isdst = ptm->tm_isdst;
	tm.tm_mday = ptm->tm_mday;
	tm.tm_min = ptm->tm_min;
	tm.tm_mon = ptm->tm_mon;
	tm.tm_sec = ptm->tm_sec;
	tm.tm_wday = ptm->tm_wday;
	tm.tm_yday = ptm->tm_yday;
	tm.tm_year = ptm->tm_year;
	tm.tm_msec = msec % 1000;
}

long long mkgmtimeex(tmex & tm)
{
	long long sec = (long long) mkgmtime((struct tm*) &tm);
	return sec * 1000 + tm.tm_msec;
}

long long mkgmtimeex_tz(tmex & tm /* local time */, 
	int tzm /* time zone in minute */)
{
	return mkgmtimeex(tm) - tzm * 60000;
}

bool decTmStr(char * tmStr, tmex & tm)
{
	// tmStr should begin with "[" and end with "]"
	if(tmStr[0] != '[')
		return false;
	// tmStr[1-3] is week day.
	for(tm.tm_wday = 0; tm.tm_wday < 7;	tm.tm_wday++){
		const char * ptr1 = strWday[tm.tm_wday];
		char * ptr2 = &tmStr[1];
		if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2]){
			break;
		}	
	}

	if(tm.tm_wday == 7){
		cerr << "Error in decTmStr. Failed to decode week day." << endl;
		return false;
	}

	// tmStr[5-7] is month
	for(tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++){
		const char * ptr1 = strMonth[tm.tm_mon];
		char * ptr2 = &tmStr[5];
		if(ptr1[0] == ptr2[0] && ptr1[1] == ptr2[1] && ptr1[2] == ptr2[2]){
			break;
		}
	}

	if(tm.tm_mon == 12){
		cerr << "Error in decTmStr. Failed to decode month." << endl;
		return false;
	}

	// tmStr[9-10] is day
	tm.tm_mday = 10 * (tmStr[9] - '0') + tmStr[10] - '0';
	if(tm.tm_mday < 1 || tm.tm_mday >= 31){
		cerr << "Error in decTmStr. Day is out of range." << endl;
		return false;
	}

	// tmStr[12-13] is hour
	tm.tm_hour = 10 * (tmStr[12] - '0') + tmStr[13] - '0';
	if(tm.tm_hour < 0 || tm.tm_hour > 23){
		cerr << "Error in decTmStr. Hour is out of range." << endl;
		return false;
	}

	// tmStr[15-16] is min
	tm.tm_min = 10 * (tmStr[15] - '0') + tmStr[16] - '0';
	if(tm.tm_min < 0 || tm.tm_min > 59){
		cerr << "Error in decTmStr. Minute is out of range." << endl;
		return false;
	}

	// tmStr[18-19] is sec
	tm.tm_sec = 10 * (tmStr[18] - '0') + tmStr[19] - '0';
	if(tm.tm_sec < 0 || tm.tm_sec > 61){
		cerr << "Error in decTmStr. Second is out of range." << endl;
		return false;
	}

	// tmStr[21-23] is msec
	tm.tm_msec = 100 * (tmStr[21] - '0') + 10 * (tmStr[22] - '0') + tmStr[23] - '0';
	if(tm.tm_msec < 0 || tm.tm_msec > 999){
		cerr << "Error in decTmStr. Millisecond is out of range." << endl;
		return false;
	}

	// tmStr[25-28] is year
	tm.tm_year = 1000 * (tmStr[25] - '0') + 100 * (tmStr[26] - '0')
		+ 10 * (tmStr[27] - '0') + tmStr[28] - '0' - 1900;
	if(tm.tm_year < 0){
		cerr << "Error in decTmStr. Year is out of range." << endl;
		return false;
	}

	// tmStr
	if(tmStr[29] != ']')
		return false;

	tm.tm_isdst = -1;
	return true;
}

c_clock::c_clock(void):m_rate(1), m_delta(0), m_delta_adjust(1 * MSEC)
#ifdef _WIN32
	,m_token(NULL), m_offset(0), m_bonline(false), m_state(STOP)
#else
		      ,m_offset(0), m_bonline(false), m_state(STOP)
#endif
{
}

c_clock::~c_clock(void)
{
}

bool c_clock::start(unsigned period, unsigned delay,
	long long offset, bool online, int rate)
{
	m_bonline = online;
	if(m_bonline)
		m_rate = 1;
	else
		m_rate = rate;
	m_period = period;
	m_offset = offset;

	if(m_state == STOP){
#ifdef _WIN32
		HRESULT hr;
		static const GUID IID_IReferenceClock = 
		{0x56a86897,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70};
		static const GUID CLSID_SystemClock = 
		{0xe436ebb1, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70};
		hr = CoCreateInstance(CLSID_SystemClock, NULL ,CLSCTX_INPROC_SERVER,
			IID_IReferenceClock , (LPVOID*)&m_pclk);

		if(FAILED(hr)){
			cerr << "Failed to get interface to the ref clock" << endl;
			return false;
		}

		m_sem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
		REFERENCE_TIME ts;
		m_pclk->GetTime(&ts);
		ts += (REFERENCE_TIME) delay;
		m_pclk->AdvisePeriodic(ts, (REFERENCE_TIME) period, m_sem, &m_token); 
		m_tcur = offset;
#else 		
		clock_gettime(CLOCK_REALTIME, &m_ts_start);
		m_tcur = 0;
#endif
		
	}else if(m_state == RUN){
		stop();
		start(period, delay, offset, online, rate);
	}

	m_state = RUN;
	return true;
}

bool c_clock::restart()
{
	if(m_state == PAUSE){
		m_state = RUN;
	}else
		return false;

	return true;
}

bool c_clock::step(int cycle)
{
	if(m_state == PAUSE){
		m_tcur += (long long) m_rate * (long long) m_period * cycle;
	}else
		return false;
	return true;
}

bool c_clock::step(long long tabs)
{
	if(m_state = PAUSE){
		m_tcur = tabs;
	}else{
		return false;
	}
	return true;
}

// set_time method adjust aws time to the value specified.
// The method only calculates difference as delta, and the value is gradually reduced in multiple call of wait()
// Note that, aws clock never reset system time. The implementation for Linux system uses system clock to reduce drift, 
// that's why the change in system time affects on the aws time. 
void c_clock::set_time(tmex & tm)
{
	long long new_time = mkgmtimeex(tm) * MSEC; // converting to 100ns precision
#ifdef _WIN32
	m_delta = new_time - m_tcur;
#else 
	m_delta = new_time - m_tcur - m_offset;
#endif
}

void c_clock::set_time(long long & t){
#ifdef _WIN32
	m_delta = t - m_tcur;
#else
	m_delta = t - m_tcur - m_offset;
#endif
}

void c_clock::set_time_delta(long long & delta){
	m_delta = delta;
}

long long c_clock::get_time()
{
	if(m_state == PAUSE)
		return m_tcur;
#ifdef _WIN32
	return m_tcur;
#else
	return m_tcur + m_offset;
#endif
}

void c_clock::wait()
{
	long long delta_adjust = 0;
	if(abs(m_delta) < m_delta_adjust || abs(m_delta) > (300 * (long long) SEC))
		delta_adjust = m_delta;
	else
		delta_adjust = (m_delta < 0 ? -m_delta_adjust : m_delta_adjust);

	m_delta -= delta_adjust;

#ifdef _WIN32
	WaitForSingleObject(m_sem, INFINITE);
	if(m_state == RUN)
		m_tcur +=  (long long) m_rate * m_period + delta_adjust;
#else
	timespec ts, trem;
	long long ttotal, tcur, tnext;	
	clock_gettime(CLOCK_REALTIME, &ts);

	// calculating total time from the epoch in 100ns.
	ttotal = (long long) ((long long)(ts.tv_sec - m_ts_start.tv_sec) * (long long) SEC ) + (long long)((ts.tv_nsec - m_ts_start.tv_nsec) / 100);
	//cout << "Got (" << ts.tv_sec << "," << ts.tv_nsec <<
	//  ") Start (" << m_ts_start.tv_sec << "," << m_ts_start.tv_nsec << ")" << endl;
	// then current time is the addition of the offset.
	tcur = (long long) m_rate * ttotal;

	// the next time should be a period ahead to previous time.
	tnext = m_tcur +(long long) m_rate * m_period;

	//cout << "ttotal " << ttotal << " tcur " << tcur << " tnext " << tnext << endl;

	if(tcur >= tnext){
	  // if current time is larger than the next time, 
	  // current time is set at that calculated from ts 
	  // obtained right above
	 if(m_state == PAUSE) 
		m_offset += m_tcur - tcur;
	 m_tcur = tcur;
	 m_offset += delta_adjust;
	 //	 cout << "Toffset " << m_offset << " (+" << delta_adjust << "/" << m_delta << ")" << endl;
	}else{
	  // if tnext is larger than the tcur, the difference 
	  // is consumed here with the nanosleep. Then the current
	  // time is set at tnext.
	  long long tdiff = tnext - tcur;
	  ts.tv_sec = tdiff / SEC;
	  ts.tv_nsec = (tdiff - ts.tv_sec * SEC) * 100;
	  // cout << "tdiff " << tdiff << endl;
	  while(nanosleep(&ts, &trem)){
	    ts = trem;
	  }
	  
	  if(m_state == PAUSE)
		m_offset += m_tcur - tnext;
	  m_tcur = tnext;
	  m_offset += delta_adjust;
	  //cout << "Toffset " << m_offset << " (+" << delta_adjust << "/" << m_delta << ")" << endl;
	}
#endif
}

void c_clock::stop()
{
	m_state = STOP;
#ifdef _WIN32
	m_pclk->Unadvise(m_token);
	m_pclk->Release();
	CloseHandle(m_sem);
	m_sem = NULL;
#endif
}

bool c_clock::pause()
{
	if(m_state != RUN)
		return false;

	m_state = PAUSE;
	return true;
}
