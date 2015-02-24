// API.c - defines user functions for accessing memory 
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h> // usleep
#include <time.h>
#include "api.h"
#include "Error.h"

vAddr evictRAM1(int memory);

void evict(vAddr page_table_index, Level level_to_evict_from){
}

vAddr get_free_ram(){
	vAddr newPage = findPage();
	if (newPage == -1){
		vAddr page_to_evict = get_page_to_evict(RAM);
		evict(page_to_evict, RAM);
	}
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	int i;
	vAddr address;
	vAddr newPage;
	
	//checks if memory is available
	if(newPage == -1){
		printf("NO MEMORY: Exiting...\n");
		exit(0);
	}
	
	//finds next unvalid page
	newPage = get_free_ram();//findPage();
	
	//checks if there is any space available
	for(i=0; i < RAM_SIZE; i++){
		if(ram[i] == 0 && table[newPage].lock == 0){
			printf("\tMemory Allocated\n");
			ram[i] = 1;
			address = i;
			table[newPage].valid = 1;
			table[newPage].location= address;
			table[newPage].counter++;
			table[newPage].timeAccessed = difftime(time(0), clk_start);
			return newPage;
		}
	}
	
	//no space is available, must run eviction algorithm to make space
	printf("\tRAM FULL. Evicting Page...\n");
	if (i==RAM_SIZE){
		
	}
	
	if(setEviction == 1){
		address = evictRAM1(1);
		table[newPage].valid = 1;
		table[newPage].location = address;
		table[newPage].counter++;
		table[newPage].timeAccessed = difftime(time(0), clk_start);
		printf("\t\tMemory Allocated\n");
	}
	else if(setEviction == 2){
		address = evictRAM2();
		//TODO
		printf("\t\tMemory Allocated\n");
	}
	
	return newPage;
}

//initializes all array values to zero
void init_arrays(){
	int i;
	
	//TODO: what's this funciton callaed?
	clk_start = current_time();
	
	for(i=0; i<RAM_SIZE; i++){
		ram[i] = 0;
	}
	
	for(i=0; i<SSD_SIZE; i++){
		ssd[i] = 0;
	}
	
	for(i=0; i<DISK_SIZE; i++){
		disk[i] = 0;
	}
	
	for(i=0; i<SIZE_PAGE_TABLE; i++){
		table[i].page_number = i;
		table[i].lock = 0;
		table[i].valid = 0;
		table[i].location = -1; 
		table[i].counter = 0; 
	}
	
	return;
}

//grabs next unvalid page
vAddr findPage(){
	int i;
	for(i=0; i<SIZE_PAGE_TABLE; i++){
		if(table[i].valid == 0){
			return i;
		}
	}
	
	
	return -1;
	
}

void printPage(struct Page this_page){
	printf("=============\nPage Index: %d\nAllocated: %d\nLocation: %d\nTime Accessed: %f\n=============\n", 
		this_page.page_number, this_page.valid, this_page.location, this_page.timeAccessed);
}


//first eviction algorithm - Least Recently Used (LRU)
vAddr evictRAM1(int memory){
	int i;
	int min = -1;
	//finds page with LRU access
	for(i=0; i<SIZE_PAGE_TABLE; i++){
		if(((-1) < table[i].location && table[i].location < RAM_SIZE) && table[i].lock == 0){
			if(min == -1){
				min = i;
			}
			else{
				if(table[i].timeAccessed < table[min].timeAccessed){
					min = i;
				}
			}
		}
	}
	//NEED to handle shifting the evicted page down
	int loc = table[min].location;
	
	//do i need a lock here
	copy_to_SSD1(&(table[min]));
	
	return loc;
	
}

void copy_to_SSD1(struct Page *page){
	int i;
	for(i=0; i<SSD_SIZE; i++){
		if(ssd[i] == 0){
			ssd[i] = 1;
			page->location = i + RAM_SIZE;
			return;
		}
	}
	printf("NO ROOM IN SSD\n");
	return;
}

void copy_to_HDD(struct Page *page){
	int i;
	for(i=0; i<HDD_SIZE; i++){
		if(hdd[i] == 0){
			ssd[i] = 1;
			page->location = i + RAM_SIZE + SSD_SIZE;
			return;
		}
	}
	printf("NO ROOM IN HDD\n");
	return;
}

//2nd eviction algorithm
vAddr evictRAM2(){
	//TODO
	return 0;
	
}


//Allocate, access, update, unlock, and free memory
//While calling allocateNewInt, needs to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < SIZE_SIZE_PAGE_TABLE; ++index) {
		indexes[index] = allocateNewInt();
		int *value = accessIntPtr(indexes[index]);
		*value = (index * 3);
		//unlockMemory(indexes[index]);
	}

	/*for (index = 0; index < SIZE_SIZE_PAGE_TABLE; ++index) {
		//freeMemory(indexes[index]);
	}*/
}


//Run the actual memory management tool
int main(){
	init_arrays();
	
	memoryMaxer();
}