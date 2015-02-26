// API.c - defines user functions for accessing memory 
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h> // usleep
#include <time.h>
#include "api.h"
#include "Error.h"

time_t clk_start; // begins clock


// FUNCTION HEADERS

// setup, debug
void init_arrays();
void printPage(vAddr page_index);

// Different page eviction algorithms
vAddr evict_LRU(int memory);
vAddr evict_Clock(int memory);

// Eviction and swapping helper functions
data_address get_next_unallocated_pageframe_in_level(Level l);
data_address evict_page_from_level(Level level_to_evict_from);
void setPage(vAddr page, Level level, data_address address)
void load_page_to_level(vAddr page,Level l);

// Returns a unallocated page from the table, or -1 if there are no unallocated pages.
vAddr get_unallocated_page(){
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
		if(table[i].allocated == False){
			return i;
		}
	}
	return -1;
}


// When the requested page is not in RAM, it is retrieved it from the appropriate backing store.
// If it is on the hard disk, it is put it onto SSD.
// it is then transferred from the SSD into main memory.
// The page fault handler must evict pages to make room when swapping in.
// The page fault handler is responsible for inserting the appropriate delays based on the memory type.
//void page_fault_handler(vAddr page){
//}

// When a page fault occurs, but there is no place to store the faulted page, a page is evicted a page to make room.
// A locked page cannot be evicted.
// Two different eviction algorithms may be used to choose an unlocked page to evict
vAddr (*get_page_to_evict)(Level l) = evict_LRU; // Pointer to the eviction function/algorithm to use

// returns the physical address for the next unallocated space in the given level of memory, or -1 of that memory level is full
data_address get_next_unallocated_pageframe_in_level(Level l){
	for(vAddr i=0; i<memory_sizes[l]); i++){
		if(memory_bitmaps[l][i] == False){
			return i;
		}
	}
	return -1;
}

// Helper function to evict the given page from the given memory level (recursive on memory levels.)
data_address evict_page_from_level(Level level_to_evict_from){
	Level level_above = level_to_evict_from + 1;
	if (level_above == NONE){
		printf("ERROR: Cannot evict from hard drive!");
	}else{
		vAddr page_to_evict = get_page_to_evict(level); // get the page to evict from the given memory level using the current page eviction algorithm
		data_address evicted_address = table[page_to_evict].location;	// save the data address of the page we're about to evict
		load_page_to_level(page_to_evict, level_above); // copy the evicted page to the next memory level (recursivly evicting pages from that level if needed)
		return evicted_address;
	}
}

// Sets the given page to the given physical address for the given level
void setPage(vAddr page, Level level, data_address address){	
	printf("\tMemory Allocated\n");
	memory_bitmaps[level][address] = True;	// record that this memory is being used in the appropriate memory bitmap
	table[page].allocated = True;			// record that this page frame is being used
	table[page].location= level;			// record what level the data is being stored in
	table[page].address = address;			// record where in that level the data is being stored
	table[page].counter++;					// increment the counter every time we access a page
	table[page].timeAccessed = difftime(time(0), clk_start);	// record the time of access
}

// loads the given page to the given memory level, evicting pages from that level to higher levels to make space if needed (recursive)
void load_page_to_level(vAddr page,Level l){
	data_address address = get_next_unallocated_pageframe_in_level(l); // gets free memory space at the given memory level
	if (address == -1){						// if this memory level is full
		address = evict_page_from_level(l);	// evict a page using the page eviction algorithm and use the memory it was taking.
	}
	set_page(page, level, address);			// set the page to use this memory at this level.
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	
	//finds an unallocated page in the table
	vAddr page = get_unallocated_page();
	
	// Checks if memory is available (there was at-least one unallocated page)
	if(page == -1){
		printf("NO MEMORY: Returning -1...\n");
		return -1;
	}
	
	load_page_to_level(page, RAM);
	
	return page;
}

vAddr evict_Clock(Level level){
	// TODO
	return -1;
}

// First eviction algorithm - Least Recently Used (LRU)
vAddr evict_LRU(Level level){
	vAddr min = -1;
	
	//finds page with LRU access
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
		if((table[i].location == level && table[i].lock == 0){	// if the table is in the given level and is unlocked
			if(min == -1){	// if min hasn't been set yet
				min = i;
			}else{
				// compare to the previous min
				if(table[i].timeAccessed < table[min].timeAccessed){
					min = i;
				}
			}
		}
	}
	

	return min;
}

// Prints page info.
void printPage(vAddr page_index){
	printf("=============\nPage Index: %d\nAllocated: %d\nLocation: %d\nTime Accessed: %f\n=============\n", 
		page_index, table[page_index].valid, table[page_index].location, table[page_index].timeAccessed);
}

#define FOREVER 187.5 // forever, in seconds.
// Stress test function, should take FOREVER
// Allocate, access, update, unlock, and free memory
// While calling allocateNewInt, needs to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_SIZE_PAGE_TABLE];
	vAddr index = 0;
	for (index = 0; index < SIZE_SIZE_PAGE_TABLE; ++index) {
		indexes[index] = allocateNewInt();
		data *value = accessIntPtr(indexes[index]);
		*value = (index * 3);
		unlockMemory(indexes[index]);
	}

	for (index = 0; index < SIZE_SIZE_PAGE_TABLE; ++index) {
		freeMemory(indexes[index]);
	}
}

// Initializes all array values to zero
void init_arrays(){

	// TODO: what's this function called?
	//clk_start = current_time();
	
	// Clear all memory bitmaps
	for (Level l = 0;l<3;l++){
		for (vAddr i=0;i<memory_sizes[l];i++){
			memory_bitmaps[l][i]=False;
		}
	}
	
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
		table[i].page_number = i;
		table[i].lock = False;
		table[i].allocated = False;
		table[i].location = NONE; 
		table[i].counter = 0; 
	}
}

//Run the actual memory management tool
int main(){
	init_arrays();			// setup
	clk_start = time(0);	// program start time
	memoryMaxer();			// test
}