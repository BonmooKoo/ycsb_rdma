#include "rdma_common.h"
#include "rdma_verb.h"
#include "zipf.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <cmath>
#include <climits>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <signal.h>
using namespace std;
//#define TOTALOP 32000000//32M
#define TOTALOP 10
int* key=new int[TOTALOP];
int cs_num;
int threadcount;

int read_key(){
    const int key_range = 1600000;
    // 1) Zipf
    /*
    ZipfGenerator zipf(key_range, 0.99);
    for (int i = 0; i < TOTALOP; ++i) {
        key[i] = zipf.Next();
    }
    */
    // 2) Uniform
    /*
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, key_range - 1);
    for (int i = 0; i < TOTALOP; ++i) {
        key[i] = dist(rng);
    }
    */
    for (int i=0;i<key_range;i++){
	key[i] = i;
    }
    return 0;
}
void
cleanup_rdma ()
{
  client_disconnect_and_clean (threadcount);
}

void
sigint_handler (int sig)
{
  printf ("\n[INFO] Ctrl+C 감지. 자원 정리 중...\n");
  cleanup_rdma ();
}

int
thread_setup (int id)
{
  int ret;
  client_connection (cs_num, threadcount, id);
  return 0;
}

int
test_smallread (int id)
{
  printf ("[%d]START\n", id);
  for (int i = id; i < TOTALOP; i += threadcount)
    {
      //rdma_read(uint64_t serveraddress, uint32_t datalength,int server,int thread)
      int suc = rdma_read ((key[i]*4*1024)%ALLOCSIZE,8, 0, id);   //return current value
    }
  printf ("[%d]END\n", id);  
}

int
test_cas (int id)
{
  int result;
  printf ("[%d]START\n", id);
  for (int i = id; i < TOTALOP; i += threadcount)
    {
      while(true){
      //rdma_CAS(compare,swap,serveradd,datalength,server,threadid)
      int result = rdma_CAS_returnvalue (0, 1, (key[i]*4*1024)%ALLOCSIZE, 8, 0, id);
      if (result == 0)
	{
	  //success so unlock it 
	  int result = rdma_CAS_returnvalue (1, 0, (key[i]*4*1024)%ALLOCSIZE, 8, 0, id);
	  break;
	}
      else
	{
	  //fail
	  
	}
      }
    }
  printf ("[%d]END\n", id);
}

int
test_read (int id,int reader)
{
  printf ("[%d]START\n", id);
  for (int i = id; i < TOTALOP; i += threadcount)
    {
      //rdma_read(uint64_t serveraddress, uint32_t datalength,int server,int thread)
      int suc = rdma_read ((key[i]*4*1024)%ALLOCSIZE, 4*1024, 0, id);	//return current value
    }
  printf ("[%d]END\n", id);
}

int
main (int argc, char **argv)
{
  int option;
  int test;
  int reader=1,caser=0,smallreader=0;
  cs_num=1;
  while ((option = getopt (argc, argv, "c:t:")) != -1)
    {
      // alloc dst
      switch (option)
	{
	case 'c':
	  cs_num = atoi (optarg);
	  break;
	case 't'://A:24READ|B : 24READ & 8 READ | C : 24READ & 8 CAS | D : 32READ
	  test = atoi (optarg);
          switch(test){
		case 0 : 
			reader = 24;
			break;
		case 1 : 
			reader = 24;
			smallreader = 8;
			break;
		case 2 : 
			reader = 24;
			caser = 8;
			break;
		case 3 : 
			reader = 32;
			break;
		default : 
			break;
	  }
	  break;
	default:
	  break;
	}
    }
  signal (SIGINT, sigint_handler);
  int read_key();
  
  printf("read key end\n");
  thread threadlist[threadcount];
  threadcount = reader + smallreader + caser;
  //setup RDMA connection
  printf("InitRDMA %d\n",threadcount);
  for (int i = 0; i < threadcount; i++)
    {
      threadlist[i] = thread (&thread_setup, i);
    }
  for (int i = 0; i < threadcount; i++)
    {
      threadlist[i].join ();
    }

  return 0;
  printf ("Start test\n");
  timespec t1, t2;
  clock_gettime (CLOCK_REALTIME, &t1);
  for (int i = reader+smallreader; i < threadcount; i++)
    {
	  threadlist[i] = thread (&test_cas, i);
	  break;
    }
  for (int i = 0; i < reader; i++)
    {
	  threadlist[i] = thread (&test_read, i,reader);
	  break;
    }
  for (int i = reader; i < reader+smallreader; i++)
    {
	  threadlist[i] = thread (&test_smallread, i);
	  break;
    } 
for (int i = 0; i < threadcount; i++)
    {
      threadlist[i].join ();
    }
  clock_gettime (CLOCK_REALTIME, &t2);
  //end time
  printf ("End test\n");
  unsigned long timer =
    (t2.tv_sec - t1.tv_sec) * 1000000000UL + t2.tv_nsec - t1.tv_nsec;
  printf ("Time : %lu msec\n", timer / 1000);
  client_disconnect_and_clean (threadcount);
  return 0;
}
