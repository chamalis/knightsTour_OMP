/**************************************
 * Stelios Barberakis 2008030116
 * chefarov@gmail.com
 * Knight's tour using distributed lists 
 * 
 * based on Jure Ziberna's code: 
 * http://www.Planet-Source-Code.com/vb/scripts/ShowCode.asp?txtCodeId=13227&lngWId=3
 * for details.
***************************************/

//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include <assert.h>
#include "omp.h"


/* PARAMETERS. CHANGE THE FOLLOWING THREE VALUES TO AFFECT FUNCTIONALITY *
 *************************************************************************/

#define N 6 //megethos NxN skakieras
#define X 1 //depth limited search - cutoff for the main thread
#define W_THREADS 8   //worker threads to execute the tasks

/* **********************************************************************/


/* This is stored in the lists. Every list node contains a Task * and a List *next
 * see list.h for the implementation */
typedef struct task{
    int id;
    int **board;
    int move_num;
    int x,y;
}Task;

#include "lists.h"

int solutions_counter; //need Locking here
int num_tasks;  //counter parallel to task.id

void move(int **board, int x, int y, int move_num) 
{
    int i,j,k, newx, newy;
    
	board[x][y] = move_num;
	if(move_num==N*N) 
	{
        #pragma omp critical
            ++solutions_counter;
	} 
	else // 3 loops, each loops 2-times -> 2^3=8 moves
	{
		for(i=0; i<2; ++i) // positive-negative for x (i=0->0*2-1= -1 ; i=1->1*2-1= 1 )
			for(j=0; j<2; ++j) // positive-negative for y
				for(k=1; k<3; ++k) // position: x=k, y=k-3 (x=2->y=-1; x=1->y=-2) -> THE 'L' MOVE
				{
                    newx = x+k*(i*2-1);
                    newy = y+(k-3)*(j*2-1);
					if(newx>=0 && newx<N && newy>=0 && newy<N)
					{
						if(board[newx][newy] <= 0) 
						{
							move(board, newx, newy, move_num+1);
							board[newx][newy] = 0;
						}
					}
                }
    }
}


/* W threads run in parallel each extracting tasks from their corresponding lists
 * each extraction is local to each thread (private t). get_first_task handles
 * the work stealing sceanrio
 */
void create_worker_threads(){
    Task *t = NULL;
    int i;
    
    printf("Creating %d worker threads to run the %d tasks \n", W_THREADS, num_tasks); 
    omp_set_num_threads(W_THREADS);  //set number of threads dynamically
	#pragma omp parallel
    {
        #pragma omp for private(t)
        for(i=0; i<W_THREADS; i++){   //W_THREADS threads parallel
            t = get_first_task(&lists_heads[i]);  //i-th thread takes from i-th list
            while(t != NULL){
                #pragma omp task
                {  
                    move((int**)t->board, t->x, t->y, t->move_num);
                }
                t = get_first_task(&lists_heads[i]); //i-th thread continues to take from i-th list
            }
        }
    }
    
}
 

/* Constructor for the task object */    
Task* create_task(int **board, int newx, int newy, int move_num){

    int i,j;
    
    //printf("creating new task\n");
    Task * newTask = (Task*)malloc(sizeof(Task));
    if(newTask == NULL){
        fprintf(stderr, "Memory Full, cant create new task\n");
        return NULL;
    }    
    
    newTask->id = ++num_tasks;
    newTask->move_num = move_num;
    newTask->x = newx;
    newTask->y = newy;
    
    newTask->board = (int**)malloc(N*sizeof(int*));
    for(i=0; i<N; i++)
        newTask->board[i] = (int*)malloc(N*sizeof(int));
    for(i=0; i<N; i++)
        for(j=0; j<N; j++)
            newTask->board[i][j] = board[i][j];
            
    return newTask;
}


/* DFS until X-level, then save the rest job to do from that level and below
 * in tasks, and  push them in a FIFO list
 * add_list function handles the circular placing of the tasks to the lists
 * (aka: sequentually) itself. */
void main_thread(int **board, int x, int y, int move_num, int depth) 
{
    int i,j,k, newx, newy;
    Task *t;

	board[x][y] = move_num;    //fill in the move
	if(move_num==N*N)  /* only for tiny chessboards it will get here :P . Just keep no fault */
		++solutions_counter;   //our case Never gets here...
	else 
	{   // 3 loops, each loops 2-times -> 2^3=8 moves
		for(i=0; i<2; ++i) // positive-negative for x (i=0->0*2-1= -1 ; i=1->1*2-1= 1 )
			for(j=0; j<2; ++j) // positive-negative for y
				for(k=1; k<3; ++k) // position: x=k, y=k-3 (x=2->y=-1; x=1->y=-2) -> THE 'L' MOVE
				{
                    newx = x+k*(i*2-1);
                    newy = y+(k-3)*(j*2-1);
					if(newx>=0 && newx<N && newy>=0 && newy<N){
						if(board[newx][newy] <= 0){
                            if(depth < X){    //didnt reach cutoff yet
                                main_thread(board, newx, newy, move_num+1, depth+1);
                                board[newx][newy] = 0;
                            }else{  //save the task of the X-level to a list
                                t = create_task(board, newx, newy, move_num+1);
                                /* Now put task at the END of the list */
                                add_list(lists_heads ,lists_tails, t, -1);  
                            }
                        }
                    }
                }
    }
}



int main(int argc, char **argv) 
{
	int x, y, i, j; 
    int **board =NULL;
    
    /* Initializations */
    board = (int**)malloc(N*sizeof(int*));
    for(i=0; i<N; i++)
        board[i] = (int*)malloc(N*sizeof(int));
    for(i=0; i<N; i++)
        for(j=0; j<N; j++)
            board[i][j] = 0;
          
    circ_pos = 0;           //global list counter
	num_tasks = 0;          //global tasks counter
	solutions_counter = 0;  //global, solutions counter SHARED (needs locking)
    
    /*Execute the search */
    main_thread((int**)board, 0, 0, 1, 0);	//x,y=0,0 , movenum=1 depth=0
    create_worker_threads();	//here starts parallelism
    	
	printf("There are %d solutions for %dx%d board\n", solutions_counter, N, N);
	return 0;
}
