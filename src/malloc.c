#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1) //Converts header pointer to data pointer
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1) //Converts data pointer to header pointer

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allocated memory  */
   bool   free;          /* Is this _block free?                            */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                   */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *lastFreeBlockFound = NULL;

void printStatistics( void )
{
  struct _block *end = heapList;
  while(end != NULL)
  {
     end = end->next;
     num_blocks++;
  }
  
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   /** \TODO Implement best fit here */

   int leftover_size; //get size of the current free node and subtract it from the size wanting to allocate
   int min_leftover = INT_MAX; //varibale to keep track of min leftover space from (size - block_size). (Might need to change later)
   struct _block *blockReturn = NULL; //Used to return the memeory address of least wasted space

   while(curr)
   {
      /*
        Checks if the current node is free and fits the size the user is wanting to allocating.
        Then compares the remaining size to the past remaning size(s).
      */
      if(curr->free && (size <= curr->size))
      {
         leftover_size = curr->size - size;

         if(leftover_size < min_leftover)
         {
            min_leftover = leftover_size;
            blockReturn = curr;
         }
      }

      *last = curr;
      curr = curr->next;
   }
   
   if(blockReturn != NULL)
      return blockReturn;

#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   /** \TODO Implement worst fit here */

   int leftover_size; //get size of the current free node and subtract it from the size wanting to allocate
   int min_leftover = 0; //varibale to keep track of min leftover space from (size - block_size). (Might need to change later)
   struct _block *blockReturn = NULL; //Used to return the memeory address of least wasted space

   while(curr)
   {
      /*
        Checks if the current node is free and fits the size the user is wanting to allocating.
        Then compares the remaining size to the past remaning size(s).
      */
      if(curr->free && (size <= curr->size))
      {
         leftover_size = curr->size - size;

         if(leftover_size > min_leftover)
         {
            min_leftover = leftover_size;
            blockReturn = curr;
         }
      }

      *last = curr;
      curr = curr->next;
   }
   
   if(blockReturn != NULL)
      return blockReturn;

#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   /** \TODO Implement next fit here */

   if(lastFreeBlockFound == NULL)
   {
      while (curr && !(curr->free && curr->size >= size)) 
      {
         *last = curr;
         curr  = curr->next;
      }

      if(curr && curr->free)
         lastFreeBlockFound = curr;
   }
   else
   {
      curr = lastFreeBlockFound; 
      while (curr && !(curr->free && curr->size >= size)) 
      {
         if(curr->next == NULL) //Check if curr is about to hit the end of the heapList. If it is point it to the "top".
         {
            *last = curr;
            curr = heapList;
         }
         else if(curr->next == lastFreeBlockFound) //No free blocks in the list point last to the "bottom" of the heapList to grow.
         {
            lastFreeBlockFound = NULL;
            while (curr)
            {
               *last = curr;
               curr  = curr->next;
            }
         }
         else //Normal continue
         {
            *last = curr;
            curr  = curr->next;
         }
         
      }

      if(curr && curr->free)
         lastFreeBlockFound = curr;
   }

#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   num_grows++;

   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);


   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }
   else
   {
      if((next->size - size) > (sizeof(struct _block) + 4))
      {
         /*
            Take the block findFreeBlock returned and split it
            copy information needed into a new block to remember next information
         */
         struct _block *new; 
         size_t old_size = next->size;

         /*
            Set the found free block size to the user requested size
            Change the free blocks pointer to the header of the new "split" block we are going to created
            set the free block found to false
         */
         next->size = size;
         new = (struct _block *)((long long)next + sizeof(struct _block) + size);
         new->next = next->next;
         next->next = new;
         next->free = false;

         //Change header of the new block
         new->free = true;
         new->size = old_size - (size + sizeof(struct _block));

         num_reuses++;
         num_splits++;
      }
      else
      {
         num_reuses++;
      }
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;
   
   num_mallocs++;
   num_requested = num_requested + size;

   //Checks the maximum size of the heap during runtime then returns that side at the end of the program.
   struct _block *getSize = heapList;
   size_t heapSize = 0;
   while( getSize )
   {
      heapSize = heapSize + getSize->size;
      getSize = getSize->next;
   }

   if(heapSize > max_heap)
      max_heap = heapSize;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */
   struct _block *loop = heapList;
   while( loop )
   {
      if(loop->free && (loop->next && loop->next->free))
      {
         loop->size = loop->next->size + sizeof(struct _block) + loop->size;
         loop->next = loop->next->next;
         num_coalesces++;
         loop = heapList; //Itterates through linked list again to make sure to get all free blocks
      }
      else
      {
         loop = loop->next;
      }
   }

   num_frees++;
}

void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc
   if(nmemb == 0 || size == 0) //case of when nmemb or size is equal to 0
      return NULL;

   struct _block *callocBlock = malloc(nmemb * size);
   memset(callocBlock,0,nmemb * size);

   if(callocBlock != NULL)
      return callocBlock;

   return NULL; //Case for error
}

void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc
   if(ptr == NULL) //Case for when ptr is equal to NULL
   {
      return malloc(size);
   }

   if(size == 0) //Case for when size is equal to 0
   {
      free(ptr);
      return NULL;
   }

   //Allocate new size into a new block and copy what was inside the ptr into the block.
   struct _block *newMemory = malloc(size);
   memcpy(newMemory,ptr,size);
   free(ptr);

   if(newMemory != NULL)
      return newMemory;


   return NULL; //Case for error
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwMjM= -----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
