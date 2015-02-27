// API.c - defines user functions for accessing memory 
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h> // usleep
#include <time.h>
#include "api.h"
#include "Error.h"
#include "pageQueues.h"
// FUNCTION HEADERS

// setup, debug, helper
program_time get_current_time();
void init_arrays();
void printPage(vAddr page_index);

// Different page eviction algorithms
vAddr evict_LRU(Level level);
vAddr evict_Clock(Level level);

// Eviction and swapping helper functions
data_address get_next_unallocated_pageframe_in_level(Level l);
data_address evict_page_from_level(Level level_to_evict_from);
void set_page(vAddr page, Level level, data_address address);
void reset_page(vAddr page);
void load_page_to_level(vAddr page,Level l);

// helper funciton - returns the current time since the program started, in ms
program_time get_current_time(){
	// TODO
	return 0;
	//difftime(time(0), clk_start);	// record the time of access
}

// Returns a unallocated page from the table, or -1 if there are no unallocated pages.
vAddr get_unallocated_page(){
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
		if(table[i].allocated == False){
			return i;
		}
	}
	return -1;
}

// When a page fault occurs, but there is no place to store the faulted page, a page is evicted a page to make room.
// A locked page cannot be evicted.
// Two different eviction algorithms may be used to choose an unlocked page to evict
vAddr evict_Clock(Level level){
	// TODO
	return -1;
}

// First eviction algorithm - Least Recently Used (LRU)
vAddr evict_LRU(Level level){
	vAddr min = -1;
	
	//finds page with LRU access
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
		if(table[i].addresses[level] != -1 && table[i].lock == 0){	// if the table is in the given level and is unlocked
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

vAddr (*get_page_to_evict)(Level); // Pointer to the eviction function/algorithm to use

// returns the physical address for the next unallocated space in the given level of memory, or -1 of that memory level is full
data_address get_next_unallocated_pageframe_in_level(Level l){
	printf("\t\tGetting next unallocated memory in level %d...\n", l);
	for(vAddr i=0; i<memory_sizes[l]; i++){
		if(memory_bitmaps[l][i] == False){
			printf("\t\t...memory %d is free in level %d.\n", i, l);
			return i;
		}
	}
	printf("\t\t...no unallocated memory found in level %d.\n", l);
	return -1;
}

// copies the page data copy in one level to another
void update_page_data(vAddr page, Level from, Level to){
	printf("\t\t\tCopying page data for page %d from level %d to level %d.\n", page, from, to);
	physical_memories[to][table[page].addresses[to]] = physical_memories[from][table[page].addresses[from]]; // copy data to a higher level.
}

// Helper function to evict the given page from the given memory level (recursive on memory levels.)
data_address evict_page_from_level(Level level_to_evict_from){
	printf("\t\tEvicting page from level %d.\n", level_to_evict_from);
	Level level_above = level_to_evict_from + 1;
	if (level_above == NONE){
		printf("ERROR: Cannot evict from hard drive!\n");
	}else{
		vAddr page_to_evict = (*get_page_to_evict)(level_to_evict_from); // get the page to evict from the given memory level using the current page eviction algorithm
		data_address evicted_address = table[page_to_evict].addresses[level_to_evict_from];	 // save the data address on this level of the page we're about to evict
		if (table[page_to_evict].addresses[level_above] = -1){			 // if the page we're evicting has a copy in the higher level, then we don't need to copy it, we can just overwrite it
			printf("\t\tPage has no copy in level above %d. Copying Page to level...\n", level_above);
			load_page_to_level(page_to_evict, level_above); 			 // copy the evicted page to the next memory level (recursively evicting pages from that level if needed)
		}else{
			if (table[page_to_evict].modified == True){
				printf("\t\tPage had a copy in the level above %d but was dirty. Updating higher level copy before overwriting this copy...\n", level_above);
				update_page_data(page_to_evict, level_to_evict_from, level_above);
			}else{
				printf("\t\tPage had a copy in the level above %d but was not dirtyOverwriting this copy...\n", level_above);
			}
		}
		memory_bitmaps[level_to_evict_from][evicted_address] = False; // Clear the memory bitmap for the memory we just evicted to a highewr level.
		return evicted_address;
	}
	printf("\t\t...evicted page from level %d.\n", level_to_evict_from);
}

// Sets the given page to the given physical address for the given level
void set_page(vAddr page, Level level, data_address address){	
	printf("\t\tSetting page %d in level %d with address %d...\n", page, level, address);
	
	memory_bitmaps[level][address] = True;	// record that this memory is being used in the appropriate memory bitmap
	table[page].allocated = True;			// record that this page frame is being used
//	table[page].location= level;			// record what level the data is being stored in
	table[page].addresses[level] = address;	// record where in that level the data is being stored
	for (Level l = 1;l<3;l++){
		if (table[page].addresses[l] != -1){	// if we found a copy of this page in some higher level of memory
			printf("\t\t\tCopying from higher level %d.\n", l);
			update_page_data(page, level, l);	// copy from that level.
			break;
		}
	}
	
	table[page].counter++;					// increment the counter every time we access a page
	table[page].timeAccessed = get_current_time();
	
	printf("\t\t...set page %d in level %d with address %d.\n", page, level, address);
}

// When the requested page is not in RAM, it is retrieved it from the appropriate backing store.
// If it is on the hard disk, it is put it onto SSD.
// it is then transferred from the SSD into main memory.
// The page fault handler must evict pages to make room when swapping in.
// The page fault handler is responsible for inserting the appropriate delays based on the memory type.
void load_page_to_level(vAddr page,Level level){ // loads the given page to the given memory level, evicting pages from that level to higher levels to make space if needed (recursive)
	printf("\tLoading page %d to level %d...\n", page, level);
	data_address address = get_next_unallocated_pageframe_in_level(level); // gets free memory space at the given memory level
	if (address == -1){						// if this memory level is full
		printf("\t\tNo free memory in level 5D found. Evicting a page...\n");
		address = evict_page_from_level(level);	// evict a page using the page eviction algorithm and use the memory it was taking.
	}
	set_page(page, level, address);			// set the page to use this memory at this level.
	printf("\t...Loaded page %d to level %d.\n", page, level);
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	printf("Allocating new int...\n");
	
	//finds an unallocated page in the table
	vAddr page = get_unallocated_page();
	
	// Checks if memory is available (there was at-least one unallocated page)
	if(page == -1){
		printf("NO MEMORY: Returning -1...\n");
		return -1;
	}
	
	load_page_to_level(page, RAM);
	
//	table[page].lock=True;

	printf("...New int Allocated.\n");
	return page;
}

// Obtains the indicated memory page, from lower levels of the hierarchy if needed, and returns a pointer to the corresponding data (integer) in RAM.
data * accessIntPtr(vAddr page){
	printf("Accessing in t pointer.\n");
	data_address RAM_address = table[page].addresses[RAM];
	if (RAM_address != -1){
		printf("\tPage to access was already in RAM.\n");
		return &(ram[RAM_address]);
	}else{
		printf("\tNeed to laod page to access into RAM\n");
		// Need to bring the page into ram
		load_page_to_level(page, RAM);
		return &(ram[table[page].addresses[RAM]]);
	}
}

// Allows the user to indicate that the page can be swapped to disk, if needed, invalidating any previous pointers they had to the memory.
void unlockMemory(vAddr page){
	printf("Unlocking page %d.\n", page);
	table[page].lock=False;
}	

// Frees the page and deletes any swapped out copies.
void freeMemory(vAddr page){
	printf("Freeing memory for page %d.\n",page);
	// TODO: clear copies of page in other levels
	for (Level l = 0;l<3;l++){
		if (table[page].addresses[l] != -1){
			memory_bitmaps[l][table[page].addresses[l]] = False; // clear memory for the copy in this level
		}
	}
	reset_page(page);												// reset page
}	

// Prints page info.
void printPage(vAddr page_index){
	printf("=============\nPage Index: %d\nAllocated: %d\nLocation: %d %d %d\nTime Accessed: %lu\n=============\n", 
		page_index, table[page_index].allocated, table[page_index].addresses[0], table[page_index].addresses[1], table[page_index].addresses[2],table[page_index].timeAccessed);
}

#define FOREVER 187.5 // forever, in seconds.
// Stress test function, should take FOREVER
// Allocate, access, update, unlock, and free memory
// While calling allocateNewInt, needs to swap old pages to secondary memory
void memoryMaxer() {
	printf("Memory Maxer...\n");
	vAddr indexes[SIZE_PAGE_TABLE];
	vAddr index = 0;
	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		indexes[index] = allocateNewInt();
		data *value = accessIntPtr(indexes[index]);
		*value = (index * 3);
		unlockMemory(indexes[index]);
	}

	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		freeMemory(indexes[index]);
	}
}

// resets the given page struct (but does not clear the memory botmaps for it's data copies)
void reset_page(vAddr page){
	table[page].lock = False;
	table[page].allocated = False;
	table[page].modified = False;
	table[page].counter = 0; 
	table[page].timeAccessed = 0;
}

// Initializes all array values to zero
void init_arrays(){
	
	// Clear all memory bitmaps
	for (Level l = 0;l<3;l++){
		for (vAddr i=0;i<memory_sizes[l];i++){
			memory_bitmaps[l][i]=False;
		}
	}
	
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){
//		table[i].page_number = i;
		reset_page(i);
	}
}

//Run the actual memory management tool
int main(){
	init_arrays();			// setup
	get_page_to_evict = &evict_LRU; // set the page eviction algorithm
	memoryMaxer();			// test
}