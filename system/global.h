#ifndef GLOBAL_H
#define GLOBAL_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "conque.h"
#include <ext/hash_set>
#include <ext/hash_map>
#include <cstring>
#include <stack>
#include <bits/stdc++.h>
// If not Linux (e.g. Windows), include direct.h because it doesn't work in Linux.
#ifndef __linux__
#include <direct.h>
#endif

#define hash_map __gnu_cxx::hash_map // ??
#define hash_set __gnu_cxx::hash_set // ??
// Used for S_IRWXU
#include <sys/stat.h>

using namespace std;

// Number of idle compers
atomic<int> global_num_idle(0);
atomic<bool> global_end_label(false);

// In micro sec
#define WAIT_TIME_WHEN_IDLE 100000

// Used when refill from data_stack in comper
size_t MINI_BATCH_NUM = 40;

// Global big task queue
void *global_Qbig;
// Global big task queue mtx
mutex Qbig_mtx;

// A queue to manage big file names for spill/refill operations
conque<string> global_Lbig;
// Number of files in global_Lbig
atomic<int> global_Lbig_num;

size_t Qbig_capacity = 16;
// How many big tasks per file to spill
int BT_TASKS_PER_FILE = 4;
// A threshold to refill Qbig
int BT_THRESHOLD_FOR_REFILL = 8;
// Global regular task queue
void *global_Qreg;
// Global regular task queue mtx
mutex Qreg_mtx;

size_t Qreg_capacity = 512;
// How many reg tasks to spill.
int RT_TASKS_PER_FILE = 32;
// Threshold to refill Qreg.
int RT_THRESHOLD_FOR_REFILL = 128;

// A queue to manage reg file names for spill/refill operations
conque<string> global_Lreg;
// Number of files in global_Lreg
atomic<int> global_Lreg_num;
string TASK_DISK_BUFFER_DIR;

// Number of compers. Compers means threads
size_t num_compers = 32;
int BIGTASK_THRESHOLD = 200;

mutex data_stack_mtx;
void *global_data_stack;

// Number of tasks assigned to each comper
size_t tasks_per_fetch_g = 1;

condition_variable cv_go;
mutex mtx_go;
// Protected by mtx_go above
bool ready_go = true;

bool enable_log = false;
void log(const string &text)
{
	if (enable_log)
		cout << "Thread id: " << this_thread::get_id() << " " << text << endl;
}

bool enable_log_time = false;

typedef std::chrono::_V2::steady_clock::time_point timepoint;
typedef std::chrono::milliseconds ms;
typedef std::chrono::system_clock clk;

mutex cout_mtx;
float log_time(const string msg, const timepoint start_time, const float latest_elapsed_time)
{
	float elapsed_time = 0;
	if (enable_log_time)
	{
		cout_mtx.lock();
		auto end_time = std::chrono::_V2::steady_clock::now();
		elapsed_time = (float)std::chrono::duration_cast<ms>(end_time - start_time).count() / 1000;
		time_t date_time_now = clk::to_time_t(clk::now());
		char *ctime_no_newline = strtok(ctime(&date_time_now), "\n"); // strtok() to replace \n with \0

		cout << "Thread id: " << this_thread::get_id() << ", " << ctime_no_newline << ", "
			 << msg << ", elapsed_time:" << elapsed_time << " seconds"
			 << ", step time," << elapsed_time - latest_elapsed_time << endl;
		cout_mtx.unlock();
	}
	return elapsed_time;
}

// Disk operations
void make_directory(const char *name)
{
#ifdef __linux__ // check if linux
	mkdir(name, S_IRWXU);
#else
	// Works on Windows when inclue direct.h, but direct.h doesn't work on linux as it was "provided by Microsoft Windows".
	_mkdir(name); 
#endif
}

// Reference: http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
void recursive_mkdir(const char *dir)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';
	for (p = tmp + 1; *p; p++)
		if (*p == '/')
		{
			*p = 0;
			// Supports both Linux and Windows
			make_directory(tmp);
			*p = '/';
		}
	make_directory(tmp);
}

#endif