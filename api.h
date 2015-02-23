//Project 4

#ifndef _API_H_
#define _API_H_

#include <stdio.h>
#include <stdlib.h>

//define memory sizes
#define RAM_SIZE 25
#define SSD_SIZE 100
#define DISK_SIZE 1000
#define SIZE_PAGE_TABLE 1000

// Set delay time
#define RAM_SLEEP 0 			//0s
#define SSD_SLEEP 250000 		//0.25s
#define HDISK_SLEEP 2500000		//2.5s


typedef signed short vAddr;

enum {
	LRU = 0,
	FIFO = 1,
} //eviction types

typedef enum {
	RAM = 0,
	SSD = 1,
	HD = 2,
	NONE = -1,
} Level;

/* page struct definition */
struct Page{
//	int data;				// data is only single interger ??
	vAddr page_number;		// page frame number (vAddr address), 0 to 1000
	int valid;				// valid bit, make sure valid bits are zero at start ??
//	int modified; 			//set dirty bit ??
//	int referenced; 		//??
	int lock;				// locked for current user
	int counter;			// for LRU, increments with every memory access ??
	Level location;			// where is the page, RAM, SSD, HD ssd or hd 
	double timeAccessed; 	// records last time page was accessed ??
};

struct Page table[SIZE_PAGE_TABLE];//page table

//memory arrays
int ram[RAM_SIZE];
int ssd[SSD_SIZE];
int disk[DISK_SIZE];

//which eviction algorithm to use
int setEviction;				//eviction type to use

time_t clk_start;				//begins clock

//prototypes
vAddr allocateNewInt();
int * accessIntPtr(vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);

void init_arrays();
vAddr findPage();
void printPage();

#endif //_API_H_
