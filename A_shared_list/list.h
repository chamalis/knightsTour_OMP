
/* Here we define the structure we will use as the
 * process list in the kernel.
 */
typedef struct List{
	Task * task;
	struct List *next;
}lnode;

/*for the common list version */
lnode *head=NULL, *tail=NULL; 

/* If pos = 0 new process becomes the new head of the list
 * If pos =-1 new process becomes the new tail of the list
 */
int add_list(Task *task, int pos){
	
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
	
	/* If the list is empty ignore pos and initialize the list storing the
	 * node as head and tail (one node on the list).
	 */
	if(head == NULL){		
		head = new;
		tail = head;
		head->next = tail;
		tail->next = NULL;
		return 0;
	}
	
	/* Place the node to the correct place depending pos value */
	if(pos == 0){
		new->next = head;
		head = new;
	}else if(pos == -1){	//tail
		tail->next = new;
		tail = new;
		tail->next = NULL;
	}
        
	return 0;		
}



/* The function get_firstProc returns the task stored in head node and extracts
 * the node from the list */
Task *get_first_task(){
	Task *ret;
	lnode *tmp;
	
    //printf("removing and returning head ... ");
	if(head == NULL){
		//printf("List is empty, there is no process to return. \n"); 
		return NULL;
	}

	ret = head->task;	//the process to return is stored in head
	tmp = head;		//We store the head to a tmp in order to free the memory released
	head = head->next;	//Before memory release we set the head, the next element of the list
	free(tmp);		//We free the memory allocated for the previous list head
	
    //printf("completed succesfully\n");
	return ret;		//and we return the process
}


/* This function just returns the head process in the list
 * without removing it. NOT USED*/
Task *get_head_task(){
	return head->task;
}


/* we delete the entire list by iterating all nodes
 * deleting them until we reach the end of the list
 * NOT USED ! */
void delete_list(){
	lnode *tmp, 
	      *iter = head;

	while(iter != NULL){
		tmp = iter;
		iter = iter->next;
		free(tmp);
	}
}

