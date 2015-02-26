// OS Project 4

#ifndef _API_H_
#define _API_H_

#include <stdio.h>
#include <stdlib.h>

//define memory sizes
#define RAM_SIZE 25
#define SSD_SIZE 100
#define HDD_SIZE 1000
unsigned int memory_sizes[] = {RAM_SIZE, SSD_SIZE, HDD_SIZE};

#define SIZE_PAGE_TABLE 1000

// Set delay time
#define RAM_SLEEP 0 			//0s
#define SSD_SLEEP 250000 		//0.25s
#define HDD_SLEEP 2500000		//2.5s
unsigned int memory_delay_times[] = {RAM_SLEEP, SSD_SLEEP, HDD_SLEEP};

// Helps to have Booleans
#define boolean unsigned short
#define True 1
#define False 0

// Type for data
typedef int data;

// Type for page number (an index into the page table) - virtual memory address
typedef signed short vAddr;

// Type for the physical address of data (an index into one of the emulated physical memories)
typedef int data_address;

// Memory hierarchy levels
typedef enum {
	RAM = 0,
	SSD = 1,
	HD = 2,
	NONE = -1
} Level;

typedef unsigned long program_time;

// Page Table Struct
typedef struct{
//	vAddr page_number;		// This page's element index in the page table, used for printing. (a vAddr for the page frame number from 0 to SIZE_PAGE_TABLE)
	Level location;			// What is the lowest level of the memory hierarchy where this page can be found? (RAM, SSD, HDD)
	data_address address;	// index (address) into the corresponding physical memory (emulated by the arrays below).
	boolean allocated;		// is the page allocated/valid?
//	boolean modified; 		// set dirty bit
//	int referenced; 		// ??
	boolean lock;				// locked for current user
	int counter;			// for LRU, increments with every memory access
	program_time timeAccessed; // records last time page was accessed
} Page;
Page table[SIZE_PAGE_TABLE]; // Page table

// Memory bitmaps, used to keep track of what physical memory is being used
boolean ram_bitmap[RAM_SIZE];
boolean ssd_bitmap[SSD_SIZE];
boolean hdd_bitmap[HDD_SIZE];
boolean* memory_bitmaps[] = {ram_bitmap, ssd_bitmap, hdd_bitmap};

// Memory arrays (emulating actual storage devices)
data ram[RAM_SIZE];
data ssd[SSD_SIZE];
data hdd[HDD_SIZE];
data* physical_memories[] = {ram, ssd, hdd};

// API prototypes
vAddr allocateNewInt();				// Reserves a new memory location, which is of sizeof(int), in the emulated RAM. It evicts other pages from RAM if needed. Returns -1 if no memory is available.
data * accessIntPtr(vAddr address); // Obtains the indicated memory page, from lower levels of the hierarchy if needed, and returns a pointer to the corresponding data (integer) in RAM.
void unlockMemory(vAddr address);	// Allows the user to indicate that the page can be swapped to disk, if needed, invalidating any previous pointers they had to the memory.
void freeMemory(vAddr address);		// Frees the page and deletes any swapped out copies.

#endif //_API_H_
