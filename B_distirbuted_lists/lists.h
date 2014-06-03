
/* Here we define the structure we will use as the
 * process list in the kernel.
 */
typedef struct List{
	Task * task;
	struct List *next;
}lnode;

/*for the distributed lists version */
lnode *lists_heads[W_THREADS], *lists_tails[W_THREADS]; 
int circ_pos; //circular counter used to push and pop sequentually among the lists

/* If pos = 0 new process becomes the new head of the list
 * If pos =-1 new process becomes the new tail of the list
 */
int add_list(lnode **heads, lnode **tails, Task *task, int pos){
	
	lnode *new;
	
	/* We check if the pos value is a valid one */
	//assert(pos==-1 || pos==0);

	/* Allocate memory for the new process node and check if succeded */
	new = (lnode*)malloc(sizeof(lnode));
	if(new == NULL){
		printf("\nError allocating memory for new Process list node.\n");
		return -1;
	}
	new->task = task;		//we store the new process in it's node

    //if we reach here it means it will be pushed in the list successfully
    //next task will be put in the next list (circular position counter)
    circ_pos = (circ_pos+1) % W_THREADS;  //W_THREADS == number_of_lists

	/* If the list is empty ignore pos and initialize the list storing the
	 * node as head and tail (one node on the list).
	 */
	if(heads[circ_pos] == NULL){		
		heads[circ_pos] = new;
		tails[circ_pos] = heads[circ_pos];
		heads[circ_pos]->next = tails[circ_pos];
		tails[circ_pos]->next = NULL;
       // printf("added successfully\n");
		return 0;
	}
	
	/* Place the node to the correct place depending pos value */
	if(pos == 0){
		new->next = heads[circ_pos];
		heads[circ_pos] = new;
	}else if(pos == -1){	//tail
		tails[circ_pos]->next = new;
		tails[circ_pos] = new;
		tails[circ_pos]->next = NULL;
	}
	return 0;		
}



/* The function get_firstProc returns the task stored in head node and extracts
 * the node from the list. Indexing is handled by create_workerthreads() itself! */
Task *get_first_task(lnode **head){
	Task *ret;
	lnode *tmp;
	int i;
    int flag = 0;
    
	if(*head == NULL){
        /* WORK STEALING !!!! - a bit Bakeristiko  */
        #pragma omp critical
        {
            for(i=0; i<W_THREADS; i++){
                if(lists_heads[i] != NULL){
                    ret = lists_heads[i]->task;
                    tmp = lists_heads[i];
                    lists_heads[i] = lists_heads[i]->next;
                    free(tmp);
                    flag = 1;
                    break;
                }                    
            }
        } 
        if(flag == 0)	//all lists are empty ... so null
                return NULL;
        else           //we found a task inside a list so return it
            return ret;
	}

	ret = (*head)->task;	//the process to return is stored in head
	tmp = *head;	//We store the head to a tmp in order to free the memory released
	*head = (*head)->next;	//Before memory release we update head location
	free(tmp);	//We free the memory allocated for the previous list head
    
	return ret;		//and we return the process
}

