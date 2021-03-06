/*Copyright (c) 2015, Drexel University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

/*
 * Authors: Karthik Sangaiah & Ankit More
 *
 * SynchroTrace.hh
 *
 */

#ifndef __CPU_SYNCHROTRACE_SYNCHROTRACE_HH
#define __CPU_SYNCHROTRACE_SYNCHROTRACE_HH
 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "cpu/testers/synchrotrace/lib/static_deque.hh"
#include <cassert>
#include <stdlib.h>
#include <algorithm>
#include <gzstream.hh>
 
#include "cpu/testers/synchrotrace/ST_Event.hh"
 
#include "base/hashmap.hh"
#include "mem/ruby/common/DataBlock.hh"
#include "mem/ruby/common/Global.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "mem/ruby/system/RubyPort.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "params/SynchroTrace.hh"
 
#define COMP_ENTRIES                    6 //8
#define COMM_ENTRIES                    2 //5
#define COMM_SHARED_INFO_ENTRIES        4
#define COMP_WRITE_ENTRIES              2
#define COMP_READ_ENTRIES		            2
#define PTHREAD_ENTRIES                 3
#define PTHREAD_TAG                     "pth_ty"

#define MAX_REQUEST_SIZE		8 //Bytes
 
#define SHARED_SPACE    1
#define LOCAL_SPACE     0
 
#define MEM_LOCAL_READ          0
#define MEM_LOCAL_WRITE         1
#define MEM_ACTION_TYPES        2
 
#define READ_BLOCK              1000
#define MIN_EVENTS_SIZE         100
//#define CPI_IOPS                1
//#define CPI_FLOPS               2
#define MAX_COMM_WAIT           1000
#define PRINT_EVENT_WAKE_COUNT  50000
// Cache miss stats
#define MISS_RATE_L1            0.5
#define MISS_PENALTY_L1         10
#define HIT_CYCLES_L1           3
#define MISS_RATE_L2            0.1
#define MISS_PENALTY_L2         100
#define HIT_CYCLES_L2           10
#define MEM_ACCESS_TIME         (HIT_CYCLES_L1 + MISS_RATE_L1 * (HIT_CYCLES_L2 + (MISS_RATE_L2 * MISS_PENALTY_L2)))
 
#define HIST_BIN_SIZE           10

using namespace std;
 
class SynchroTrace : public MemObject
{
  public:
    class CpuPort : public MasterPort
    {
      private:
        SynchroTrace *tester;
 
      public:
        CpuPort(const std::string &_name, SynchroTrace *_tester,
                PortID _id)
            : MasterPort(_name, _tester, _id), tester(_tester)
        {}
 
      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvRetry()
        { panic("%s does not expect a retry\n", name()); }
    };
 
    typedef SynchroTraceParams Params;
    SynchroTrace(const Params *p);
    ~SynchroTrace();
 
    virtual MasterPort &getMasterPort(const std::string &if_name,
                                      int idx = -1);
 
    MasterPort* getCpuPort(int idx);
 
    virtual void init();
 
    void wakeup();
    void wakeup(int procID);
 
    void printStats(std::ostream& out) const {}
    void clearStats() {}
    void printConfig(std::ostream& out) const {}
 
    void print(std::ostream& out) const;

  protected:
    class SynchroTraceStartEvent : public Event
    {
      private:
        SynchroTrace *tester;
 
      public:
        SynchroTraceStartEvent(SynchroTrace *_tester)
            : Event(CPU_Tick_Pri), tester(_tester)
        {}
        void process() { tester->wakeup(); }
        virtual const char *description() const { return "SynchroTrace tick"; }
    };
 
    class SynchroTraceCoreEvent : public Event
    {
      private:
        SynchroTrace *tester;
        int procID;
 
      public:
        SynchroTraceCoreEvent(SynchroTrace *_tester, int _procID)
            : Event(CPU_Tick_Pri), tester(_tester), procID(_procID)
        {}
        void process() { tester->wakeup(procID); }
        virtual const char *description() const { return "Core event tick"; }
    };
 
    SynchroTraceStartEvent synchroTraceStartEvent;
    vector<SynchroTraceCoreEvent*> coreEvents;
 
    MasterID _masterId;
 
  private:
    void hitCallback(NodeID proc, PacketPtr pkt);
 
    vector<MasterPort*> ports;
    int m_num_cpus;
    int m_num_threads;
    int m_deadlock_threshold;
    string m_eventDir;
    string m_outputDir;
    bool m_skipLocalRW;
    int m_master_wakeup_frequency;
 
    uint64 m_num_cache_lines;
 
    // probably not going to be used
    int m_directory_shared_frac;
    int m_dir_shared_frac_bits;
    int m_random_add_gen_bits;

    int printThreadEventCounters; //Paco - Adding a counter for a print function
    bool roiFlag; //Paco - Set when entering ROI (Parallel Region)
    int worker_thread_count; //Paco - Keeps track of # of worker threads for ROI 
    // abstract cpi estimation for integer and floating point ops
    float CPI_IOPS;
    float CPI_FLOPS;

    // Private copy constructor and assignment operator
    SynchroTrace(const SynchroTrace& obj);
    SynchroTrace& operator=(const SynchroTrace& obj);
 
    void checkForDeadlock(int procID);
    void replenishEvents();
    void replenishEvents(unsigned int threadID);
    void extendEventQueue(unsigned int threadID);
    void processPthreadFile();
    void initSigilFilePointers();
    void initStats();
    void initialThreadMapping();
    void generateEventQueue();
    void checkCompletion();

    void printThreadEvents(); //Paco - Print EventID# for every thread at 500k wakeups
    void printEvent(int threadID, bool start_end); //Paco (8/14) - Print EventID# for specific thread before/after event loaded/completed
    void readEventFile(int threadID);
    void createSubEvents();
    void createSubEvents(int procID, bool eventIDPassed=false, int eventThreadID=0);
    void progressEvents(int procID);
    void progressPthreadEvent(ST_Event *thisEvent, int procID);
 
    // thread swap functions
    void swapThreads(int procID);
    void swapStalledThreads(int procID);
    void moveThreadToHead(int procID, int threadID);
 
    // event type read functions
    void processCommEvent(string thisEvent, size_t hashPos);
    void processCompEvent(string thisEvent);
    void processCompMainEvent(string thisEvent, ST_Event *newEvent);
    void processCompWriteEvent(string dependencyInfo, ST_Event *newEvent);
    void processCompReadEvent(string dependencyInfo, ST_Event *newEvent);
    void processPthreadEvent(string thisEvent, size_t caretPos);
    void processAddressToID(string line, size_t hashPos);  //vasil again
    void processBarrierEvent(string line, size_t starPos); //vasil again
    int memTypeToInsert(const unsigned long loc_reads, const unsigned long loc_writes, const unsigned long max_loc_reads, const unsigned max_loc_writes, const int read_or_write);
 
    // message trigger functions
    void triggerCommReadMsg(const int procID, const int threadID, subEvent* thisSubEvent);
    void triggerCompWriteMsg(const int procID, const int threadID, subEvent* thisSubEvent);
    void triggerCompReadMsg(const int procID, const int threadID, subEvent* thisSubEvent);
 
    bool checkCommDependency(sharedInfo* commEvent, int ThreadID);
    bool checkAllCommDependencies(ST_Event* thisEvent);
    bool checkBarriers(ST_Event *thisEvent); //Paco - 11/5
    void outputRTT_hist();
 
    vector<Time> m_last_progress_vector;
 
    StaticDeque<ST_Event *> **eventMap;
    vector<threadStats> threadStatistics;
    vector<long> curEventListPos;
    vector< vector<int> > threadMap;
    vector<bool> threadStartedMap;
    vector<bool> threadMutexMap; // Paco (11/6) - Added map to check if thread has a mutex lock
    vector<bool> threadContMap; // Paco (11/12) - Added map to check if thread is done with barrier
    vector<int> lastMsgTriggerTime;
    vector< m5::hash_map<int, int> > roundTripTime;
    vector<int> maxRTT; //RTT --> rount trip time
    set<uint64_t> mutexLocks;
    set<uint64_t> spinLocks;
    map<uint64_t, int>  AddresstoIdMap; // Vasil - Needed to map the address to the id
    map<uint64_t, set<int> > BarrierMap; // Vasil - Needed for the barrier
    map<uint64_t, set<int> > threadWaitMap; // Vasil - Needed for the barrier

    vector<igzstream *> inputFilePointer;
    vector<ogzstream *> outputFilePointer;
    ifstream pthreadFilePointer; // Vasil addition

    bool stopped;
    int num_of_mapped_processors;
 
    inline int log_int(long long n) {
        assert(n > 0);
        int counter = 0;
        while (n >= 2) {
            counter++;
            n = n>>(long long)(1);
        }
        return counter;
    }
 
    inline bool is_power_of_2(long long n)
    {
        return (n == ((long long)(1) << log_int(n)));
    }

    //Added by Sid to keep track of % of cycles from Shared Reads/Writes and % of cycles from Local Read/Writes
    //vector<vector<int>> sharedEventHist;
    //vector<vector<int>> localEventHist;
  int sharedEventHist[10];
  int localEventHist[10];
  
};
 
#endif // __CPU_SYNCHROTRACE_SYNCHROTRACE_HH
