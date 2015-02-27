//Helper functions

//Created this because not sure about the structure of the main file now

/*
//-------------------------------------------//
//QUEUE OPERATIONS

//Queue node - should be in api.
typedef struct pageNode{
	page *data;
	struct pageNode *next;
}pageNode; // ??

pageNode *head; //??
pageNode *tail; //??

//Queue functions

//Push an item to the end of the queue
void enqueue(page *data){
	if (tail == NULL){
		tail = (pageNode *)malloc(sizeof(pageNode));
		tail-> next = NULL;
		tail-> data = data;
		head = tail;
	} else{
		pageNode *temp = (pageNode *)malloc(sizeof(pageNode));
		temp -> data = data;
		temp -> next = NULL;
		tail-> next = temp;
		tail = temp;
	}
} 

//Pops an item from the front of the queue
void dequeue(){
	pageNode *temp = (pageNode *)malloc(sizeof(pageNode));
	temp = head;
	if(temp == NULL){
		printf("Can't dequeue empty queue\n");
		return;
	} else{
		if( temp -> data != NULL){
			free(head);
			head = temp -> next;
		} else{
			free(head);
			head = NULL;
			tail = NULL;
		}
	}
}

//Print the contents of queue
void print_queue(){
	pageNode *current = (pageNode *)malloc(sizeof(pageNode));
	current = head;
	if(head == NULL && tail == NULL){
		printf("Queue empty\n");
		return;
	}
	while(current -> next != NULL){
		printf("Page at level %d and address %d with referenced %d\n", current->data->location, current->data->address, current->data->referenced);
		current = current -> next;
	}
	if(current -> next == NULL){
		printf("Page at level %d and address %d with referenced %d\nEND\n", current->data->location, current->data->address, current->data->referenced);
	}
}*/

//-----------------------------------------//
//Delay the transfer
	delay(int level){
		if(level == RAM_LEVEL){
			return;
		}
		if(level == SSD_LEVEL){
			usleep(SSD_SLEEP);
		}
		if(level == HDD_LEVEL){
			usleep(HHD_SLEEP);
		}
	}


//Returns -1 if the level is full, 
//otherwise returns the address of the first open position
	vAddr findMemory(int level){
		int counter = 0;
		int size;
		int *memory;
		if(level == 0){
			size = SIZE_RAM;
			memory = &RAM[0];
		} else if(level == 1){
			size = SIZE_SSD;
			memory = &SSD[0];
		} else{
			size = SIZE_HDD;
			memory = &HDD[0];
		}

		for(counter = 0; counter < size; counter++){
			if(*(memory + counter) == 0){
				return counter;
			}
		}
		return -1;
	} 

	void clear_memory_position(page *page_to_clear){
		switch( page_to_clear->location ){
			case(RAM_LEVEL):
			RAM[page_to_clear->address] = 0;
			break;
			case(SSD_LEVEL):
			SSD[page_to_clear->address] = 0;
			break;
			case(HDD_LEVEL):
			HDD[page_to_clear->address] = 0;
			break;
		}
		page_to_clear -> allocated = 0;
	} 

//Adds a page entry to the table and allocates the free spot
//Returns the virtual address of the new added page
	vAddr add_page(int level, int physical_address){
		if(level > 2){
			errorWithContext("Invalid level, exiting\n");
			exit(1);
		}

		if(physical_address == -1){
			evict_page(level);
			physical_address = findMemory(level);
		}

		page new_page;
		new_page.address = physical_address;	
		new_page.locked = 0;					
		new_page.referenced = 0;				
		new_page.allocated = 1;					
		new_page.location = level;
		gettimeofday(&new_page.last_used, NULL);

		//alocate in physical memory
		if(level == RAM_LEVEL)
			RAM[physical_address] = 1;
		else if(level == SSD_LEVEL)
			SSD[physical_address] = 1;
		else
			HDD[physical_address] = 1;

		int index = findPage();
		page_table[index] = new_page;
		enqueue(&page_table[index]);
		return index;
	}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
	vAddr allocateNewInt(){
		int physical_address = findMemory(RAM_LEVEL);
		return add_page(RAM_LEVEL, physical_address);
	}

// Obtains the indicated memory page, from lower levels of the hierarchy if needed, 
//and returns a pointer to the corresponding data (integer) in RAM.
	int * accessIntPtr (vAddr address){
		int counter;
		page *page_to_find = (page *)malloc(sizeof(page));
		page_to_find = &table[address];
		page_to_find->locked = 1;

	//If found in RAM, return a pointer to it
		if(page_to_find->location == RAM_LEVEL){
			printf("Found vAddr %d in RAM\n", address);
			return &RAM[page_to_find->address];
		} else
	{ //If not in RAM, check the lower levels
		int free_memory = -1;
		while(free_memory < 0){
			evict_page(page_to_find -> level -1);
			free_memory = findMemory(page_to_find -> level -1);
		}

		clear_memory_position(page_to_find);
		page_to_find -> address = free_memory;
		page_to_find -> level = page_to_find -> level -1;
		page_to_find -> allocated = 1;
		//printf("vAddr is at level %d and needs to get to level %d. Open memory on level %d is %d\n", page_item->location, page_item->location -1, page_item->location -1, free_memory);
		return accessIntPtr(address);
	}
} 

// Memory must be unlocked when user is done with it
// Any previous pointers in memory are considered invalid and can't be used
void unlockMemory(vAddr address){
	table[address].locked = 0;
	//TODO
} 

// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){
	page *page_to_free = &table[address];
	page_to_free -> allocated = 0;
	page_to_free -> locked = 0;
	page_to_free -> referenced = 0;
	//TODO: find and delete swapped-out copies
}



//----------------------------------------------//
//For main.c

//Run the actual memory management tool
int main(int argc, char * argv[]){
	srand(time(NULL));
	if( argc != 2){
		printf("Please specify proper arguments:\n\t0 - LRU \n\t1 - Clock\n");
		exit(1);
	}

	algorithm = atoi( argv[1] );
	if(algorithm != 0 && algorithm != 1){
		printf("Please specify proper arguments:\n\t0 - LRU \n\t1 - Clock\n");
		exit(1);
	}

	init_arrays();
	//memoryMaxer();
}//NeedThis



