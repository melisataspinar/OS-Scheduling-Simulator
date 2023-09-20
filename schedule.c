// CS 342 - Operating Systems
// Project 2
// Melisa Taşpınar, 21803668

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// ****************************************************** BURST STRUCT *****************************************************
struct CPU_Burst
{
    int thread_index;
    int burst_index;
    float length;
    long wall_clock_time; 
    struct CPU_Burst *next;
};

// **************************************************** GLOBAL VARIABLES ***************************************************
struct CPU_Burst *head = NULL;
struct CPU_Burst *heads_array[10]; // to be used if the command type is the one with the filename

int N, Bcount, minB, avgB, minA, avgA;
char* ALG;

char* filename;
bool command_type_file;

float virtual_runtimes[10];

int number_of_terminated_w_threads;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

float total_wait;
float wait_times[10];
int burst_counts[10];
int total_burst_count;

// ********************************************* RANDOM GENERATION FUNCTION ************************************************
float exponential_random( float minimum, float average )
{
    float random_value;
    
    do
    {
        random_value = -log( 1- ( rand() / ( 1.0 + RAND_MAX ) ) ) * average;
    } while ( random_value < minimum );

    return random_value;
}

// **************************************************** BURST FUNCTIONS ****************************************************
void output_burst( struct CPU_Burst *burst )
{
    printf( "CPU BURST ****************************\n" );
    printf( "Thread index:\t%d\n", burst->thread_index );
    printf( "Burst index:\t%d\n", burst->burst_index );
    printf( "Length:\t\t%.2f ms\n", burst->length );
    printf( "Wall clock time:%ld ms\n", burst->wall_clock_time );
    printf( "**************************************" );
    printf( "\n\n" );
}

struct CPU_Burst *create_burst( char *input_row, int thread_index, int burst_index ) 
{
    struct CPU_Burst *ptr = malloc( sizeof ( struct CPU_Burst ) );
    ptr->thread_index = thread_index;
    ptr->burst_index = burst_index;
    ptr->next = NULL;

    // the filename-command type
    if ( command_type_file )
    {
        char *c1 = strtok( input_row, " " );
        char *c2 = strtok( NULL, " " );

        //printf("CREATING BURST: c1: %s c2: %s\n", c1, c2);
        ptr->length = (float) atoi( c2 );
        usleep( (float) atoi( c1 ) * 1000 );

        struct timeval  tv;
        gettimeofday(&tv, NULL);
        ptr->wall_clock_time = (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
        return ptr;
    }

    // the regular command type
    else
    {
        ptr->length = exponential_random( minB, avgB );
        struct timeval  tv;
        gettimeofday(&tv, NULL);
        ptr->wall_clock_time = (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
        return ptr;
    }
}

// ************************************************* LINKED LIST FUNCTIONS *************************************************
void output_list() 
{
    struct CPU_Burst *curr = head;

    printf( "\nrunqueue = {\n" ); 
    while( curr != NULL ) 
    {
        printf( "\tthread_index: %d,\tburst_index: %d,\t\tlength: %f,\twall_clock_time: %ld;\n", 
                    curr->thread_index, curr->burst_index, curr->length, curr->wall_clock_time );
        curr = curr->next;
    } 
    printf( "}\n\n" );
}

// inserts a given value at the end of the list
void insert( int thread_index, int burst_index, float length, long wall_clock_time )
{
    if ( head == NULL )
    {
        head = ( struct CPU_Burst* ) malloc( sizeof( struct CPU_Burst ) );
        head->thread_index = thread_index; 
        head->burst_index = burst_index; 
        head->length = length; 
        head->wall_clock_time = wall_clock_time; 
        head->next = NULL; 
        return;
    }

    struct CPU_Burst *new;
    new = ( struct CPU_Burst* ) malloc( sizeof( struct CPU_Burst ) );
    if ( new == NULL )
    {
        printf( "WARNING: Memory allocation unsuccessful.\n" );
        return;
    }
    new->thread_index = thread_index; 
    new->burst_index = burst_index; 
    new->length = length; 
    new->wall_clock_time = wall_clock_time; 
    new->next = NULL; 
    
    struct CPU_Burst *curr = head;
    struct CPU_Burst *prev = head;
    while ( curr != NULL )
    {   
        prev = curr;
        curr = curr->next;
    }
    prev->next = new;
}

// retrieves the first node of the list
struct CPU_Burst* retrieve() 
{
    if ( head == NULL )
    {
        return NULL;
    }

    struct CPU_Burst *temp = head;
    head = head->next;
    return temp;
}

bool is_empty() 
{
   return head == NULL;
}

// ************************************************* SCHEDULING FUNCTION ***************************************************
struct CPU_Burst* scheduling_function() 
{
    if ( !strcmp( ALG, "FCFS" ) )
    {
        struct CPU_Burst *temp = head;

        if ( is_empty() )
        {
            return NULL;
        }
        
        head = head->next;
        return temp;
    }
    else if ( !strcmp( ALG, "SJF" ) )
    {
        //printf("SJF TIME -------------------------------\n");
        struct CPU_Burst *curr = head;
        struct CPU_Burst *min = head;

        bool seen_ids[10];
        int i;
        for ( i = 0; i < 10; i++ )
        {
            seen_ids[i] = false;
        }
        seen_ids[curr->thread_index-1] = true;

        // empty list
        if ( curr == NULL )
        {
            return NULL;
        }

        
        // list with 1 element
        if ( curr->next == NULL )
        {
            min = curr;
            head = NULL; // pop
            return min;
        }
        
        // list with multiple elements
        while ( curr != NULL )
        {
            //printf("SJF LOOP -------------------------------\n");
            if ( curr->length < min->length && !seen_ids[ curr->thread_index-1 ] )
            {
                min = curr;
            }

            seen_ids[ curr->thread_index-1 ] = true;
            curr = curr->next;
        }

        // output_burst(min);
        // pop
        curr = head; 
        if ( curr == min )
        {
            head = head->next;
            return min;
        }
        while ( curr != NULL )
        {
            if ( curr->next == min )
            {
                curr->next = curr->next->next;
                return min;
            }
            curr = curr->next;
        }
        return min;
    }
    else if ( !strcmp( ALG, "PRIO" ) )
    {
        struct CPU_Burst *curr = head;
        struct CPU_Burst *prio = head;

        // empty list
        if ( curr == NULL )
        {
            return NULL;
        }

        bool seen_ids[10];
        int i;
        for ( i = 0; i < 10; i++ )
        {
            seen_ids[i] = false;
        }
        seen_ids[curr->thread_index-1] = true;

        // list with 1 element
        if ( curr->next == NULL )
        {
            prio = curr;
            head = NULL; // pop
            return prio;
        }

        // list with multiple elements
        while ( curr != NULL )
        {
            
            if ( curr->thread_index < prio->thread_index && !seen_ids[ curr->thread_index-1 ] )
            {
                prio = curr;
            }
            seen_ids[ curr->thread_index-1 ] = true;
            curr = curr->next;
        }

        // pop
        curr = head; 
        if ( curr == prio )
        {
            head = head->next;
            return prio;
        }
        while ( curr != NULL )
        {
            if ( curr->next == prio )
            {
                curr->next = curr->next->next;
            }
            curr = curr->next;
        }
        return prio;
    }
    else if ( !strcmp( ALG, "VRUNTIME" ) )
    {
        struct CPU_Burst *curr = head;
        struct CPU_Burst *chosen = head;

        bool seen_ids[10];
        int i;
        for ( i = 0; i < 10; i++ )
        {
            seen_ids[i] = false;
        }
        seen_ids[curr->thread_index-1] = true;

        // empty list
        if ( curr == NULL )
        {
            return NULL;
        }

        // list with 1 element
        if ( curr->next == NULL )
        {
            chosen = curr;
            virtual_runtimes[ chosen->thread_index-1 ] += chosen->length * ( 0.7 + 0.3 * chosen->thread_index ); // increment vruntime
            head = NULL; // pop
            return chosen;
        }

        // list with multiple elements
        while ( curr != NULL )
        {
            
            if ( virtual_runtimes[ curr->thread_index-1 ] < virtual_runtimes[ chosen->thread_index-1 ] && !seen_ids[ curr->thread_index-1 ] )
            {
                chosen = curr;
            }  
            seen_ids[ curr->thread_index-1 ] = true;
            curr = curr->next;
        }

        virtual_runtimes[ chosen->thread_index-1 ] += chosen->length * ( 0.7 + 0.3 * chosen->thread_index ); // increment vruntime

        // pop
        curr = head; 
        if ( curr == chosen )
        {
            head = head->next;
            return chosen;
        }
        while ( curr != NULL )
        {
            if ( curr->next == chosen )
            {
                curr->next = curr->next->next;
            }
            curr = curr->next;
        }
        return chosen;
    }
}

// *************************************************** THREAD FUNCTIONS ****************************************************
void *wthread( void *args ) 
{
    int tid = *( (int *) args );
    //printf("********************%d", tid);
    // regular command
    if ( !command_type_file ) 
    {
        int index;
        for ( index = 1; index <= Bcount; index++ ) 
        {
            usleep( exponential_random( minA, avgA ) * 1000 );
            struct CPU_Burst *burst = create_burst( NULL, tid, index );

            pthread_mutex_lock( &lock ); 

            //printf( "Inserting into thread W%d the following burst:\n", tid );
            //output_burst( burst );
            insert( burst->thread_index, burst->burst_index, burst->length, burst->wall_clock_time );

            pthread_cond_signal( &cond ); 
            pthread_mutex_unlock( &lock ); 
            
        }
    } 

    // command with file name
    else 
    {
        FILE * file;

        unsigned long length = 0;
        int burst_index = 0;

        char name[50] = "";
        char c1[5];
        char c2[5];
        char *row;

        sprintf( c1, "%d", tid );
        strcat( strcat( strcat( name, "./" ), filename ), "-" );
        file = fopen( strcat( strcat( name, strcpy( c2, c1 ) ), ".txt" ), "r" );

        if ( file != NULL ) 
        {
            while ( getline( &row, &length, file ) != -1 ) 
            {
                ++burst_index; 
                struct CPU_Burst *burst = create_burst( row, tid, burst_index );

                

                //printf( "Inserting into thread W%d the following burst:\n", tid );
                //output_burst( burst );

                pthread_mutex_lock( &lock ); 
                insert( burst->thread_index, burst->burst_index, burst->length, burst->wall_clock_time );

                pthread_cond_signal( &cond ); 
                pthread_mutex_unlock( &lock ); 
                
            }

            fclose( file );

            if ( row )
            {
                free( row );
            }
        }

        else // file was NULL
        {
            printf( "WARNING: Could not open file %s.\n", name );
            exit(-1);
        }
    }
    
    pthread_cond_signal( &cond ); 
    ++number_of_terminated_w_threads;
    return NULL;
}

void *sthread( void *args ) 
{
    int remaining_w_threads;
    struct CPU_Burst *burst;
    do
    {
        pthread_mutex_lock( &lock );
        if ( N <= number_of_terminated_w_threads && head == NULL ) 
        {
            pthread_mutex_unlock( &lock ); 
            break;
        }

        while ( head == NULL ) 
        {
            //printf("\n\nS IS WAITING \n\n");
            pthread_mutex_unlock( &lock ); 
            pthread_cond_wait( &cond, &lock2 );
            break;
        } 

         // --------------------

        struct timeval  tv;
        gettimeofday(&tv, NULL);
        //printf("The runqueue at time %ld is as follows:", (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000) );
        //output_list();
        burst = scheduling_function();
        float wait_time = (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000) - burst->wall_clock_time;
        //printf("adding wait %.2f", wait_time);
        total_wait += wait_time;
        total_burst_count++;
        burst_counts[burst->burst_index-1]++;
        wait_times[burst->burst_index-1] += wait_time;

        pthread_mutex_unlock( &lock ); // ------------------
        
        //printf( "Processing the burst:\n" );
        // output_burst( burst );

        gettimeofday(&tv, NULL);
        // printf("before wait: %ld\n", (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000));
        usleep( burst->length * 1000 );
        gettimeofday(&tv, NULL);
        // printf("after wait: %ld\n", (long) ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000));
        free( burst );

        remaining_w_threads = N - number_of_terminated_w_threads;
        // printf( "Number of W threads remaining: %d\n\n", remaining_w_threads ) ;

    } while ( true ); 

    return NULL;
} 

// ****************************************************** MAIN FUNCTION ****************************************************
int main( int argc, char **argv ) 
{ 
    total_wait = 0;
    total_burst_count = 0;
    // check if the user entered a correct number of arguments
    if ( argc != 5 && argc != 8 )
    {
        printf( "ERROR:\tUser did not enter a correct number of arguments.\n\tPlease enter either 4 or 7 arguments.\n" );
        return -1;
    }

    // the file command type is being used
    if ( argc == 5 )
    {
        command_type_file = true;

        // check the -f
        if ( strcmp( argv[3], "-f" ) )
        {
            printf( "ERROR:\tUser did not enter \"-f\" as the third argument.\n\tPlease enter \"-f\" after the scheduling algorithm and before the file name.\n" );
            return -1;
        }

        // check if user entered valid N
        if ( atoi( argv[1] ) < 1 || atoi( argv[1] ) > 10 ) 
        {
            printf( "ERROR:\tUser entered an invalid value for <N>.\n\tN must be between 1 and 10.\n" );
            return -1;
        }

        N = atoi( argv[1] );
        ALG = argv[2];
        filename = argv[4];
    }

    // the regular command type is being used
    else
    {
        command_type_file = false;

        // check if user entered valid N
        if ( atoi( argv[1] ) < 1 || atoi( argv[1] ) > 10 ) 
        {
            printf( "ERROR:\tUser entered an invalid value for <N>.\n\tN must be between 1 and 10.\n" );
            return -1;
        }

        // check if user entered valid ALG
        if ( strcmp( argv[7], "FCFS" ) && strcmp( argv[7], "SJF" ) && strcmp( argv[7], "PRIO" ) && strcmp( argv[7], "VRUNTIME" ) )
        {
            printf( "ERROR:\tUser entered an invalid value for <ALG>.\n\tALG can be one of the following: FCFS, SJF, PRIO, VRUNTIME.\n" );
            return -1;
        }

        // check if minB is smaller than 100
        if ( atoi( argv[3] ) < 100 ) 
        {
            printf( "ERROR:\tUser entered an invalid value for <minB>.\n\tminB must not be smaller than 100.\n" );
            return -1;
        }

        // check if minA is smaller than 100
        if ( atoi( argv[5] ) < 100 ) 
        {
            printf( "ERROR:\tUser entered an invalid value for <minA>.\n\tminA must not be smaller than 100.\n" );
            return -1;
        }

        // read the entered arguments into global variables
        N = atoi( argv[1] );
        Bcount = atoi( argv[2] );
        minB = atoi( argv[3] );
        avgB = atoi( argv[4] );
        minA = atoi( argv[5] );
        avgA = atoi( argv[6] );
        ALG = argv[7];
    }
    
    // initialize the virtual runtimes array
    int i;
    for ( i = 0; i < 10; i++ )
    {
        virtual_runtimes[i] = 0;
        wait_times[i] = 0;
        burst_counts[i] = 0;
    }
    
    number_of_terminated_w_threads = 0;

    if ( pthread_mutex_init( &lock, NULL ) != 0 ) 
    {
        printf("ERROR: Mutex initialization unsuccessful.\n\n");
        return -1;
    }

    if ( pthread_mutex_init( &lock2, NULL ) != 0 ) 
    {
        printf("ERROR: Mutex initialization unsuccessful.\n\n");
        return -1;
    }

    // thread parameters
	pthread_t tid_array[N+1];
	pthread_attr_t t;
	pthread_attr_init( &t );
	
	// create an array for the id's
    int ids[N];
    int index;
    for ( index = 0; index < N; index++ )
    {
        ids[index] = index+1;
    }

    // create the threads
    pthread_create( &tid_array[0], &t, sthread, NULL ); // scheduler thread
    for ( index = 0; index < N; index++ )
    {
        pthread_create( &tid_array[index+1], &t, wthread, &ids[index] ); // workload threads
    }

	// join the threads
    for ( index = 0; index < N + 1; index++ )
    {
        pthread_join( tid_array[index], NULL );
    }

    pthread_mutex_destroy( &lock );
    pthread_mutex_destroy( &lock2 );

    if ( !strcmp( ALG, "VRUNTIME" ) ) {
        int k;
        for (k = 0; k < N; k++) {
            float avg_wait = wait_times[k] / burst_counts[k];
            printf("\naverage waiting time for thread %d: %.2f\n", k+1, avg_wait);
        }
    } else {
        printf("\naverage waiting time: %.2f\n", total_wait / total_burst_count);
    }
    
    // printf("%d", total_burst_count);

    return 0;
}