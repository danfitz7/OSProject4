// API.c - defines user functions for accessing memory 
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h> // usleep
#include <time.h>
#include <sys/time.h>
#include "api.h"
#include "Error.h"
#include "pageQueues.h"
// FUNCTION HEADERS

unsigned int recursionLevel = 0;
void printTabs(){
	for (int i=0;i<recursionLevel;i++){
		printf("\t");
	}
}

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
boolean load_page_to_level(vAddr page,Level l);

// simulated delay times
program_time runTime = 0;
program_time startTime = 0;
void sleep_for_level_access(Level level){
	printTabs(); printf("\t\tWaiting on memory device %d for %dus\n", level, memory_delay_times[level]);
	runTime += memory_delay_times[level];
	//sleep(memory_delay_times[level]);
}
program_time curTime = 0;
program_time get_current_time(){
	//struct timeval curTime;
	//gettimeofday(&curTime, NULL);
	//return (curTime.tv_sec*1000 + curTime.tv_usec/1000) - startTime;
	curTime++;
	return curTime;
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
vAddr evict_Random(Level level){
	printTabs(); printf("\tChoosing a random page to evict...\n");
	//TODO: check if there's even anything to evict (ram not full of locked)
	
	vAddr random_page_to_evict = rand()%SIZE_PAGE_TABLE;
	while(table[random_page_to_evict].allocated == True && table[random_page_to_evict].addresses[level] != -1){ // find a random page that is allocated and has a copy on this level or memory
		if (level != RAM || table[random_page_to_evict].lock == False){ // make sure it's not locked in RAM (note short circuit logic)
			random_page_to_evict = rand()%SIZE_PAGE_TABLE;
		}
	}
	printTabs(); printf("\t...randomly evicting page %d from level %d.\n", random_page_to_evict, level);
	return random_page_to_evict;
}

// First eviction algorithm - Least Recently Used (LRU)
vAddr evict_LRU(Level level){
	vAddr min = -1;
	printTabs(); printf("\tFinding the Least Recently Used Page to evict...\n");
	//finds page with LRU access
	for(vAddr i=0; i<SIZE_PAGE_TABLE; i++){	// Loop through page table
		if(table[i].allocated==True && table[i].addresses[level] != -1 && !(level==RAM && table[i].lock == True)){	// if the page is allocated and is in the given level memory and is unlocked if that level is RAM
			if(min == -1){	// if min hasn't been set yet (note we have to use min as an index for the comparison below, so the comparison will crash without this initial set condition)
				min = i;
			}else{
				// compare to the previous min
				if(table[i].timeAccessed < table[min].timeAccessed){
					min = i;
				}
			}
		}
	}
	
	if (min ==-1){
		printf("ERROR: LRU could not find a page to evict on level %d!\n", min);
	}else{
		printTabs(); printf("\t...LRU page on this level %d is page %d.\n", level, min);
	}
	
	return min;
}

vAddr (*get_page_to_evict)(Level); // Pointer to the eviction function/algorithm to use

// returns the physical address for the next unallocated space in the given level of memory, or -1 of that memory level is full
data_address get_next_unallocated_pageframe_in_level(Level l){
	printTabs(); printf("\tGetting next unallocated memory in level %d...\n", l);
	for(vAddr i=0; i<memory_sizes[l]; i++){
		if(memory_bitmaps[l][i] == False){
			printTabs(); printf("\t...memory %d is free in level %d.\n\n", i, l);
			return i;
		}
	}
	printTabs(); printf("\t...no unallocated memory found in level %d.\n\n", l);
	return -1;
}

// if we're swapping between two devices, we can issue commands to both in (nearly) the same time and end up waiting for the longer one.
void sleep_for_swap(Level a, Level b){
	Level longer_level = (memory_delay_times[a]>=memory_delay_times[b])?a :b;
	sleep_for_level_access(longer_level);
}

// copies the page data copy in one level to another
void update_page_data(vAddr page, Level from, Level to){
	printTabs(); printf("\t\tCopying page data for page %d from level %d to level %d.\n", page, from, to);
	if (table[page].addresses[to] <0 || table[page].addresses[from] <0){
		printTabs(); printf("\nERROR: tried to update page data from address %d in level %d to %d in level %d.\n", table[page].addresses[from], from, table[page].addresses[to], to);
	}else if (from == to){
		printf("\nERROR: Copying page data to the same level %d.\n",from);
	}else{
		sleep_for_swap(from, to);
		physical_memories[to][table[page].addresses[to]] = physical_memories[from][table[page].addresses[from]]; // copy data to a higher level.
	}
}

// Helper function to evict the given page from the given memory level (recursive on memory levels.)
data_address evict_page_from_level(Level level_to_evict_from){
	Level level_above = level_to_evict_from + 1;
	if (level_above == NONE){
		printTabs(); printf("ERROR: Cannot evict from hard drive!\n");
	}else{
		vAddr page_to_evict = (*get_page_to_evict)(level_to_evict_from); // get the page to evict from the given memory level using the current page eviction algorithm
		
		// Check that we were able to find something to evict
		if (page_to_evict <0){
			printf("ERROR: Could not find any page on level %d to evict!\n", level_to_evict_from);
			return -1;
		}
		
		printTabs(); printf("\tEvicting page %d from level %d.\n", page_to_evict, level_to_evict_from);
		
		// get the address of the data on this level
		data_address evicted_address = table[page_to_evict].addresses[level_to_evict_from];	 // save the data address on this level of the page we're about to evict
		
		// Copy or update the page to the higher level, if it's not already there
		if (table[page_to_evict].addresses[level_above] = -1){	// no copy in higher level		 
			recursionLevel++;
			printTabs(); printf("\tPage has no copy in next higher level %d. Loading Page to that level...\n", level_above);
			load_page_to_level(page_to_evict, level_above); 			 		// copy the evicted page to the next memory level (recursively evicting pages from that level if needed)
			printTabs(); printf("\tCopying page data to newly loaded frame...\n");
			update_page_data(page_to_evict, level_to_evict_from, level_above);	// copy the page frame
			printTabs(); printf("\t...Page frame copied.\n\n");
			recursionLevel--;
		// if the page we're evicting has a copy in the higher level, then we don't need to copy it, we can just overwrite it
		}else{
			if (table[page_to_evict].modified == True){
				printTabs(); printf("\tPage had a copy in the level above %d but was dirty. Updating higher level copy before overwriting this copy...\n", level_above);
				update_page_data(page_to_evict, level_to_evict_from, level_above);
			}else{
				printTabs(); printf("\tPage had a copy in the level above %d but was not dirty. Overwriting this copy...\n", level_above);
			}
		}
		
		// Clear this table from this level
		table[page_to_evict].addresses[level_to_evict_from] = -1;  	 // Delete this table's reference to the data on this memory level (data will be overwritten when a new page is put in)
		memory_bitmaps[level_to_evict_from][evicted_address] = False; // Clear the memory bitmap for the memory we just evicted to a highewr level.
		
		printTabs(); printf("\t...evicted page %d using address %d from level %d.\n\n", page_to_evict, evicted_address, level_to_evict_from);

		return evicted_address;
	}
	printTabs(); printf("\t...evicted page from level %d.\n", level_to_evict_from);
}

// Sets the given page to the given physical address for the given level
void set_page(vAddr page, Level level, data_address address){	
	printTabs(); printf("\tSetting page %d in level %d with address %d...\n", page, level, address);
	
	memory_bitmaps[level][address] = True;	// record that this memory is being used in the appropriate memory bitmap
	table[page].allocated = True;			// record that this page frame is being used
//	table[page].location= level;			// record what level the data is being stored in
	table[page].addresses[level] = address;	// record where in that level the data is being stored
	
	// search for higher-level copies to copy data from
	for (Level l = level+1;l<3;l++){
		if (table[page].addresses[l] != -1){	// if we found a copy of this page in some higher level of memory
			printTabs(); printf("\t\tCopying data from higher level %d.\n", l);
			update_page_data(page, level, l);	// copy from that level.
			break;
		}
	}
	
	table[page].counter++;					// increment the counter every time we access a page
	table[page].timeAccessed = get_current_time();
	
	printTabs(); printf("\t...set page %d in level %d with address %d.\n\n", page, level, address);
}

// When the requested page is not in RAM, it is retrieved it from the appropriate backing store.
// If it is on the hard disk, it is put it onto SSD.
// it is then transferred from the SSD into main memory.
// The page fault handler must evict pages to make room when swapping in.
// The page fault handler is responsible for inserting the appropriate delays based on the memory type.
boolean load_page_to_level(vAddr page,Level level){ // loads the given page to the given memory level, evicting pages from that level to higher levels to make space if needed (recursive)
	printf("\n"); printTabs(); printf("\tLoading page %d to level %d...\n", page, level);
	recursionLevel++;
	
	data_address address = get_next_unallocated_pageframe_in_level(level); // gets free memory space at the given memory level
	if (address == -1){						// if this memory level is full
		recursionLevel--;
		printTabs(); printf("\t\tNo free memory in level %d found. Evicting a page...\n", level);
		recursionLevel+=2;
		
		address = evict_page_from_level(level);	// evict a page using the page eviction algorithm and use the memory it was taking.
		
		if (address <0){
			printf("ERROR: Could not find a page to evict on the level %d to load a page there!\n", level);
			return False;
		}
		
		recursionLevel--;
		
		// No page could be evicted for some reason
		if (address == -1){
			printf("WARNING: No page could be evicted from memory level %d to load page %d.\n", level, page);
			return False;
		}

	}
	set_page(page, level, address);			// set the page to use this memory at this level.
	
	recursionLevel--;
	printTabs(); printf("\t...Loaded page %d to level %d.\n\n", page, level);
	return True;
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	printf("\nAllocating new int...\n");
	
	//finds an unallocated page in the table
	vAddr page = get_unallocated_page();
	
	// Checks if memory is available (there was at-least one unallocated page)
	if(page == -1){
		printf("ERROR: NO MEMORY: Returning -1...\n");
		return -1;
	}
	
	load_page_to_level(page, RAM);
	
//	table[page].lock=True;

	printf("...New int Allocated.\n");
	return page;
}

// Obtains the indicated memory page, from lower levels of the hierarchy if needed, and returns a pointer to the corresponding data (integer) in RAM.
data * accessIntPtr(vAddr page){
	printf("\nAccessing int pointer.\n");
	data_address RAM_address = table[page].addresses[RAM];
	table[page].modified = True; // Assume the user will change the data with the pointer we're about to give them
	table[page].lock = True;
	// if the page is already in RAM, just given them the RAM pointer
	if (RAM_address != -1){
		printf("\tPage to access was already in RAM.\n");
		return &(ram[RAM_address]);
	
	// if the page is not in RAM, we've hit a page fault. Load it to RAM, evicitng other pages if need be.
	}else{
		printf("\tPage fault! Loading requested page to ram!\n");
		// Need to bring the page into ram
		if (load_page_to_level(page, RAM)){
			return &(ram[table[page].addresses[RAM]]);
		}else{
			return NULL;
		}
	}
}

// Allows the user to indicate that the page can be swapped to disk, if needed, invalidating any previous pointers they had to the memory.
void unlockMemory(vAddr page){
	printf("\nUnlocking page %d.\n", page);
	if (page>=SIZE_PAGE_TABLE || table[page].allocated==False){
		printf("ERROR: trying to unlock non-existant or unallocated page %d.\n", page);
	}else{
		table[page].lock=False;
	}
}	

// Frees the page and deletes any swapped out copies.
void freeMemory(vAddr page){
	printf("\nFreeing memory for page %d.\n",page);
	
	if (page>=SIZE_PAGE_TABLE || table[page].allocated==False){
		printf("ERROR: trying to free non-existant or unallocated page %d.\n", page);
	}else{
		// TODO: clear copies of page in other levels
		for (Level l = 0;l<3;l++){
			if (table[page].addresses[l] != -1){
				memory_bitmaps[l][table[page].addresses[l]] = False; // clear memory for the copy in this level
			}
		}
		reset_page(page);	// reset page
	}		
}	

// Prints page info.
void printPage(vAddr page_index){
	printf("=============\nPage Index: %d\nAllocated: %d\nLocation: %d %d %d\nTime Accessed: %lu\n=============\n", 
		page_index, table[page_index].allocated, table[page_index].addresses[0], table[page_index].addresses[1], table[page_index].addresses[2],table[page_index].timeAccessed);
}

void printMemoryBitmap(Level level){
	printf("\tLevel %d:", level);
	for (int i=0;i<memory_sizes[level];i++){
		printf((memory_bitmaps[level][i]==True)?"1":"0");
	}
	printf("\n");
}

void printMemoryBitmaps(){
	printf("MEMORY BITMAPS:\n");
	for (int i=0;i<3;i++){
		printMemoryBitmap(i);
	}
}

void printPageTable(){
	printf("\nPAGE TABLE:\n");
	for (vAddr page =0;page<SIZE_PAGE_TABLE;page++){
		if (table[page].allocated==True){
			printf("\tP %3d L{%2d,%2d,%2d} T %lu\n", page, table[page].addresses[0], table[page].addresses[1], table[page].addresses[2], table[page].timeAccessed);
		}
	}
}

// Stress test function, should take FOREVER
// Allocate, access, update, unlock, and free memory
// While calling allocateNewInt, needs to swap old pages to secondary memory
void memoryMaxer() {
	printf("Memory Maxer...\n");
	vAddr indexes[SIZE_PAGE_TABLE];
	vAddr index = 0;
	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		printf("\n\n##################### INT %d Runtime %lu ############################\n", index, runTime);
		printMemoryBitmaps();
		printPageTable();
		
		indexes[index] = allocateNewInt();
		data *value = accessIntPtr(indexes[index]);
		*value = (index * 3);
		unlockMemory(indexes[index]);
	}

	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		freeMemory(indexes[index]);
	}
	
	printf("\n\nTotal runtime: %lu which %s forever.", runTime, (runTime==FOREVER)?"is":"is not");
}

// resets the given page struct (NOTE: does NOT clear the memory bitmaps for it's data copies)
void reset_page(vAddr page){
	table[page].lock = False;
	table[page].allocated = False;
	table[page].modified = False;
	table[page].counter = 0; 
	table[page].timeAccessed = 0;
	
	// delete all copies
	for (Level l=0;l<3;l++){
		table[page].addresses[l] = -1; 
	}
}

// Initializes all array values to zero
void init_arrays(){
	
	startTime = get_current_time();
	
	// Clear all memory bitmaps initially
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