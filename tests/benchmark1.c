#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main()
{
    printf("Running benchmark1.c\n");
    char * ptr_array[50000];
    srand(0);
    clock_t start, end;
    double total;

    start = clock();

    /*
        Different block sizes will show the differences between the four heap algorithims and how they might need
        more splits and grows to be able to allocate proper sized memory blocks. 
    */
    int i;
    for ( i = 0; i < 50000; i++ )
    {
        printf("Allocating block %d\n", i);
        //if number is greater than 4 generate a random block size
        int block_odds = (rand() % 10) + 1; 
        
        if(block_odds <= 4)
        {
            //printf("Creating normal block of size 1024\n");
            ptr_array[i] = ( char * ) malloc ( 1024 ); 
        }
        else
        {
            int block_size = (rand() % 5120) + 1024; 
            //printf("Creating normal block of size %d\n",block_size);
            ptr_array[i] = ( char * ) malloc ( block_size ); 
        }
        
        ptr_array[i] = ptr_array[i];
    }

    //Randomly free a block if the random number generated for that block is > 6
    for ( i = 0; i < 50000; i++ )
    {
        printf("Freeing block %d\n",i);
        int free_odds = (rand() % 10) + 1; 
        if( free_odds > 6 )
        {
            free( ptr_array[i] );
        }
    }

    /*
        Mallocing another 50000 blocks to taking into account for search time for algoritihms.
    */
    char * ptr_array2[50000];

    for ( i = 0; i < 50000; i++ )
    {
        printf("Allocating block %d\n", i);
        ptr_array2[i] = ( char * ) malloc ( 1024 ); 
        ptr_array2[i] = ptr_array2[i];
    }

    end = clock();
    total = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Total time: %f\n", total);

    return 0;
}