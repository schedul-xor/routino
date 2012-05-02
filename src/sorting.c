/***************************************
 Merge sort functions.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2009-2012 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(USE_PTHREADS) && USE_PTHREADS
#include <pthread.h>
#include <unistd.h>
#endif

#include "types.h"

#include "files.h"
#include "sorting.h"


/* Global variables */

/*+ The command line '--tmpdir' option or its default value. +*/
extern char *option_tmpdirname;

/*+ The amount of RAM to use for filesorting. +*/
extern size_t option_filesort_ramsize;

/*+ The number of filesorting threads allowed. +*/
extern int option_filesort_threads;


/* Thread data type definitions */

/*+ A data type for holding data for a thread. +*/
typedef struct _thread_data
 {
  pthread_t thread;             /*+ The thread identifier. +*/

  int       running;            /*+ A flag indicating the current state of the thread. +*/

  void     *data;               /*+ The main data array. +*/
  void    **datap;              /*+ An array of pointers to the data objects. +*/
  size_t    n;                  /*+ The number of pointers. +*/

  char    *filename;            /*+ The name of the file to write the results to. +*/

  size_t   itemsize;            /*+ The size of each item. +*/
  int    (*compare)(const void*,const void*); /*+ The comparison function. +*/
 }
 thread_data;


/* Thread helper functions */

static void *filesort_fixed_heapsort_thread(thread_data *thread);
static void *filesort_vary_heapsort_thread(thread_data *thread);


/*++++++++++++++++++++++++++++++++++++++
  A function to sort the contents of a file of fixed length objects using a
  limited amount of RAM.

  The data is sorted using a "Merge sort" http://en.wikipedia.org/wiki/Merge_sort
  and in particular an "external sort" http://en.wikipedia.org/wiki/External_sorting.
  The individual sort steps and the merge step both use a "Heap sort"
  http://en.wikipedia.org/wiki/Heapsort.  The combination of the two should work well
  if the data is already partially sorted.

  index_t filesort_fixed Returns the number of objects kept.

  int fd_in The file descriptor of the input file (opened for reading and at the beginning).

  int fd_out The file descriptor of the output file (opened for writing and empty).

  size_t itemsize The size of each item in the file that needs sorting.

  int (*compare)(const void*, const void*) The comparison function (identical to qsort if the
                                           data to be sorted is an array of things not pointers).

  int (*keep)(void *,index_t) If non-NULL then this function is called for each item, if it
                              returns 1 then the object is kept and written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

index_t filesort_fixed(int fd_in,int fd_out,size_t itemsize,int (*compare)(const void*,const void*),int (*keep)(void*,index_t))
{
 int *fds=NULL,*heap=NULL;
 int nfiles=0,ndata=0;
 index_t count=0,total=0;
 size_t nitems=option_filesort_ramsize/(option_filesort_threads*(itemsize+sizeof(void*)));
 void *data,**datap;
 thread_data *threads;
 int i,more=1;
#if defined(USE_PTHREADS) && USE_PTHREADS
 int nthreads=0;
#endif

 /* Allocate the RAM buffer and other bits */

 threads=(thread_data*)malloc(option_filesort_threads*sizeof(thread_data));

 for(i=0;i<option_filesort_threads;i++)
   {
    threads[i].running=0;

    threads[i].data=malloc(nitems*itemsize);
    threads[i].datap=malloc(nitems*sizeof(void*));

    threads[i].filename=(char*)malloc(strlen(option_tmpdirname)+24);

    threads[i].itemsize=itemsize;
    threads[i].compare=compare;
   }

 /* Loop around, fill the buffer, sort the data and write a temporary file */

 do
   {
    int thread=0;

#if defined(USE_PTHREADS) && USE_PTHREADS

    /* Find a spare slot (one *must* be unused at all times) */

    for(thread=0;thread<option_filesort_threads;thread++)
       if(!threads[thread].running)
          break;

#endif

    /* Read in the data and create pointers */

    for(i=0;i<nitems;i++)
      {
       threads[thread].datap[i]=threads[thread].data+i*itemsize;

       if(ReadFile(fd_in,threads[thread].datap[i],itemsize))
         {
          more=0;
          break;
         }

       total++;
      }

    threads[thread].n=i;

    /* Shortcut if there is no data and no previous files (i.e. no data at all) */

    if(nfiles==0 && threads[thread].n==0)
       goto tidy_and_exit;

    /* No new data read in this time round */

    if(threads[thread].n==0)
       break;

    /* Sort the data pointers using a heap sort (potentially in a thread) */

    sprintf(threads[thread].filename,"%s/filesort.%d.tmp",option_tmpdirname,nfiles);

#if defined(USE_PTHREADS) && USE_PTHREADS

    if(option_filesort_threads==1)
      {
       threads[thread].running=1;

       filesort_fixed_heapsort_thread(&threads[thread]);

       threads[thread].running=0;
      }
    else
      {
       while(nthreads==(option_filesort_threads-1))
         {
          struct timespec millisecond={0,1000000};

          for(i=0;i<option_filesort_threads;i++)
             if(threads[i].running==2)
               {
                pthread_join(threads[i].thread,NULL);
                threads[i].running=0;
                nthreads--;
                printf("thread stopped nthreads=%d\n",nthreads);
               }

          if(nthreads==(option_filesort_threads-1))
             nanosleep(&millisecond,NULL);
         }

       threads[thread].running=1;

       pthread_create(&threads[thread].thread,NULL,(void* (*)(void*))filesort_fixed_heapsort_thread,&threads[thread]);

       nthreads++;
       printf("thread started nthreads=%d\n",nthreads);
      }

#else

    filesort_fixed_heapsort_thread(&threads[thread]);

#endif

    nfiles++;
   }
 while(more);

 /* Wait for all of the threads to finish */

#if defined(USE_PTHREADS) && USE_PTHREADS

 while(nthreads && option_filesort_threads>1)
   {
    struct timespec millisecond={0,1000000};

    for(i=0;i<option_filesort_threads;i++)
       if(threads[i].running==2)
         {
          pthread_join(threads[i].thread,NULL);
          threads[i].running=0;
          nthreads--;
          printf("thread stopped nthreads=%d\n",nthreads);
         }

    if(nthreads)
       nanosleep(&millisecond,NULL);
   }

#endif

 /* Shortcut if only one file, lucky for us we still have the data in RAM) */

 if(nfiles==1)
   {
    for(i=0;i<threads[0].n;i++)
      {
       if(!keep || keep(threads[0].datap[i],count))
         {
          WriteFile(fd_out,threads[0].datap[i],itemsize);
          count++;
         }
      }

    DeleteFile(threads[0].filename);

    goto tidy_and_exit;
   }

 /* Check that number of files is less than file size */

 assert(nfiles<nitems);

 /* Open all of the temporary files */

 fds=(int*)malloc(nfiles*sizeof(int));

 for(i=0;i<nfiles;i++)
   {
    char *filename=threads[0].filename;

    sprintf(filename,"%s/filesort.%d.tmp",option_tmpdirname,i);

    fds[i]=ReOpenFile(filename);

    DeleteFile(filename);
   }

 /* Perform an n-way merge using a binary heap */

 heap=(int*)malloc((1+nfiles)*sizeof(int));

 data =threads[0].data;
 datap=threads[0].datap;

 /* Fill the heap to start with */

 for(i=0;i<nfiles;i++)
   {
    int index;

    datap[i]=data+i*itemsize;

    ReadFile(fds[i],datap[i],itemsize);

    index=i+1;

    heap[index]=i;

    /* Bubble up the new value */

    while(index>1)
      {
       int newindex;
       int temp;

       newindex=index/2;

       if(compare(datap[heap[index]],datap[heap[newindex]])>=0)
          break;

       temp=heap[index];
       heap[index]=heap[newindex];
       heap[newindex]=temp;

       index=newindex;
      }
   }

 /* Repeatedly pull out the root of the heap and refill from the same file */

 ndata=nfiles;

 do
   {
    int index=1;

    if(!keep || keep(datap[heap[index]],count))
      {
       WriteFile(fd_out,datap[heap[index]],itemsize);
       count++;
      }

    if(ReadFile(fds[heap[index]],datap[heap[index]],itemsize))
      {
       heap[index]=heap[ndata];
       ndata--;
      }

    /* Bubble down the new value */

    while((2*index)<ndata)
      {
       int newindex;
       int temp;

       newindex=2*index;

       if(compare(datap[heap[newindex]],datap[heap[newindex+1]])>=0)
          newindex=newindex+1;

       if(compare(datap[heap[index]],datap[heap[newindex]])<=0)
          break;

       temp=heap[newindex];
       heap[newindex]=heap[index];
       heap[index]=temp;

       index=newindex;
      }

    if((2*index)==ndata)
      {
       int newindex;
       int temp;

       newindex=2*index;

       if(compare(datap[heap[index]],datap[heap[newindex]])<=0)
          ; /* break */
       else
         {
          temp=heap[newindex];
          heap[newindex]=heap[index];
          heap[index]=temp;
         }
      }
   }
 while(ndata>0);

 /* Tidy up */

 tidy_and_exit:

 if(fds)
   {
    for(i=0;i<nfiles;i++)
       CloseFile(fds[i]);
    free(fds);
   }

 if(heap)
    free(heap);

 for(i=0;i<option_filesort_threads;i++)
   {
    free(threads[i].data);
    free(threads[i].datap);

    free(threads[i].filename);
   }

 return(count);
}


/*++++++++++++++++++++++++++++++++++++++
  A function to sort the contents of a file of variable length objects (each
  preceded by its length in FILESORT_VARSIZE bytes) using a limited amount of RAM.

  The data is sorted using a "Merge sort" http://en.wikipedia.org/wiki/Merge_sort
  and in particular an "external sort" http://en.wikipedia.org/wiki/External_sorting.
  The individual sort steps and the merge step both use a "Heap sort"
  http://en.wikipedia.org/wiki/Heapsort.  The combination of the two should work well
  if the data is already partially sorted.

  index_t filesort_vary Returns the number of objects kept.

  int fd_in The file descriptor of the input file (opened for reading and at the beginning).

  int fd_out The file descriptor of the output file (opened for writing and empty).

  int (*compare)(const void*, const void*) The comparison function (identical to qsort if the
                                           data to be sorted is an array of things not pointers).

  int (*keep)(void *,index_t) If non-NULL then this function is called for each item, if it
                              returns 1 then the object is kept and written to the output file.
  ++++++++++++++++++++++++++++++++++++++*/

index_t filesort_vary(int fd_in,int fd_out,int (*compare)(const void*,const void*),int (*keep)(void*,index_t))
{
 int *fds=NULL,*heap=NULL;
 int nfiles=0,ndata=0;
 index_t count=0,total=0;
 size_t datasize=option_filesort_ramsize/option_filesort_threads;
 FILESORT_VARINT nextitemsize,largestitemsize=0;
 void *data,**datap;
 thread_data *threads;
 int i,more=1;
#if defined(USE_PTHREADS) && USE_PTHREADS
 int nthreads=0;
#endif

 /* Allocate the RAM buffer and other bits */

 threads=(thread_data*)malloc(option_filesort_threads*sizeof(thread_data));

 for(i=0;i<option_filesort_threads;i++)
   {
    threads[i].running=0;

    threads[i].data=malloc(datasize);
    threads[i].datap=NULL;

    threads[i].filename=(char*)malloc(strlen(option_tmpdirname)+24);

    threads[i].compare=compare;
   }

 /* Loop around, fill the buffer, sort the data and write a temporary file */

 if(ReadFile(fd_in,&nextitemsize,FILESORT_VARSIZE))    /* Always have the next item size known in advance */
    goto tidy_and_exit;

 do
   {
    size_t ramused=FILESORT_VARALIGN-FILESORT_VARSIZE;
    int thread=0;

#if defined(USE_PTHREADS) && USE_PTHREADS

    /* Find a spare slot (one *must* be unused at all times) */

    for(thread=0;thread<option_filesort_threads;thread++)
       if(!threads[thread].running)
          break;

#endif

    threads[thread].datap=threads[thread].data+datasize;

    threads[thread].n=0;

    /* Read in the data and create pointers */

    while((ramused+FILESORT_VARSIZE+nextitemsize)<=((void*)threads[thread].datap-sizeof(void*)-threads[thread].data))
      {
       FILESORT_VARINT itemsize=nextitemsize;

       if(itemsize>largestitemsize)
          largestitemsize=itemsize;

       *(FILESORT_VARINT*)(threads[thread].data+ramused)=itemsize;

       ramused+=FILESORT_VARSIZE;

       ReadFile(fd_in,threads[thread].data+ramused,itemsize);

       *--threads[thread].datap=threads[thread].data+ramused; /* points to real data */

       ramused+=itemsize;

       ramused =FILESORT_VARALIGN*((ramused+FILESORT_VARSIZE-1)/FILESORT_VARALIGN);
       ramused+=FILESORT_VARALIGN-FILESORT_VARSIZE;

       total++;
       threads[thread].n++;

       if(ReadFile(fd_in,&nextitemsize,FILESORT_VARSIZE))
         {
          more=0;
          break;
         }
      }

    /* No new data read in this time round */

    if(threads[thread].n==0)
       break;

    /* Sort the data pointers using a heap sort (potentially in a thread) */

    sprintf(threads[thread].filename,"%s/filesort.%d.tmp",option_tmpdirname,nfiles);

#if defined(USE_PTHREADS) && USE_PTHREADS

    if(option_filesort_threads==1)
      {
       threads[thread].running=1;

       filesort_vary_heapsort_thread(&threads[thread]);

       threads[thread].running=0;
      }
    else
      {
       while(nthreads==(option_filesort_threads-1))
         {
          struct timespec millisecond={0,1000000};

          for(i=0;i<option_filesort_threads;i++)
             if(threads[i].running==2)
               {
                pthread_join(threads[i].thread,NULL);
                threads[i].running=0;
                nthreads--;
                printf("thread stopped nthreads=%d\n",nthreads);
               }

          if(nthreads==(option_filesort_threads-1))
             nanosleep(&millisecond,NULL);
         }

       threads[thread].running=1;

       pthread_create(&threads[thread].thread,NULL,(void* (*)(void*))filesort_vary_heapsort_thread,&threads[thread]);

       nthreads++;
       printf("thread started nthreads=%d\n",nthreads);
      }

#else

    filesort_vary_heapsort_thread(&threads[thread]);

#endif

    nfiles++;
   }
 while(more);

 /* Wait for all of the threads to finish */

#if defined(USE_PTHREADS) && USE_PTHREADS

 while(nthreads && option_filesort_threads>1)
   {
    struct timespec millisecond={0,1000000};

    for(i=0;i<option_filesort_threads;i++)
       if(threads[i].running==2)
         {
          pthread_join(threads[i].thread,NULL);
          threads[i].running=0;
          nthreads--;
          printf("thread stopped nthreads=%d\n",nthreads);
         }

    if(nthreads)
       nanosleep(&millisecond,NULL);
   }

#endif

 /* Shortcut if only one file, lucky for us we still have the data in RAM) */

 if(nfiles==1)
   {
    for(i=0;i<threads[0].n;i++)
      {
       if(!keep || keep(threads[0].datap[i],count))
         {
          FILESORT_VARINT itemsize=*(FILESORT_VARINT*)(threads[0].datap[i]-FILESORT_VARSIZE);

          WriteFile(fd_out,threads[0].datap[i]-FILESORT_VARSIZE,itemsize+FILESORT_VARSIZE);
          count++;
         }
      }

    DeleteFile(threads[0].filename);

    goto tidy_and_exit;
   }

 /* Check that number of files is less than file size */

 largestitemsize=FILESORT_VARALIGN*(1+(largestitemsize+FILESORT_VARALIGN-FILESORT_VARSIZE)/FILESORT_VARALIGN);

 assert(nfiles<((datasize-nfiles*sizeof(void*))/largestitemsize));

 /* Open all of the temporary files */

 fds=(int*)malloc(nfiles*sizeof(int));

 for(i=0;i<nfiles;i++)
   {
    char *filename=threads[0].filename;

    sprintf(filename,"%s/filesort.%d.tmp",option_tmpdirname,i);

    fds[i]=ReOpenFile(filename);

    DeleteFile(filename);
   }

 /* Perform an n-way merge using a binary heap */

 heap=(int*)malloc((1+nfiles)*sizeof(int));

 data=threads[0].data;
 datap=data+datasize-nfiles*sizeof(void*);

 /* Fill the heap to start with */

 for(i=0;i<nfiles;i++)
   {
    int index;
    FILESORT_VARINT itemsize;

    datap[i]=data+FILESORT_VARALIGN-FILESORT_VARSIZE+i*largestitemsize;

    ReadFile(fds[i],&itemsize,FILESORT_VARSIZE);

    *(FILESORT_VARINT*)(datap[i]-FILESORT_VARSIZE)=itemsize;

    ReadFile(fds[i],datap[i],itemsize);

    index=i+1;

    heap[index]=i;

    /* Bubble up the new value */

    while(index>1)
      {
       int newindex;
       int temp;

       newindex=index/2;

       if(compare(datap[heap[index]],datap[heap[newindex]])>=0)
          break;

       temp=heap[index];
       heap[index]=heap[newindex];
       heap[newindex]=temp;

       index=newindex;
      }
   }

 /* Repeatedly pull out the root of the heap and refill from the same file */

 ndata=nfiles;

 do
   {
    int index=1;
    FILESORT_VARINT itemsize;

    if(!keep || keep(datap[heap[index]],count))
      {
       itemsize=*(FILESORT_VARINT*)(datap[heap[index]]-FILESORT_VARSIZE);

       WriteFile(fd_out,datap[heap[index]]-FILESORT_VARSIZE,itemsize+FILESORT_VARSIZE);
       count++;
      }

    if(ReadFile(fds[heap[index]],&itemsize,FILESORT_VARSIZE))
      {
       heap[index]=heap[ndata];
       ndata--;
      }
    else
      {
       *(FILESORT_VARINT*)(datap[heap[index]]-FILESORT_VARSIZE)=itemsize;

       ReadFile(fds[heap[index]],datap[heap[index]],itemsize);
      }

    /* Bubble down the new value */

    while((2*index)<ndata)
      {
       int newindex;
       int temp;

       newindex=2*index;

       if(compare(datap[heap[newindex]],datap[heap[newindex+1]])>=0)
          newindex=newindex+1;

       if(compare(datap[heap[index]],datap[heap[newindex]])<=0)
          break;

       temp=heap[newindex];
       heap[newindex]=heap[index];
       heap[index]=temp;

       index=newindex;
      }

    if((2*index)==ndata)
      {
       int newindex;
       int temp;

       newindex=2*index;

       if(compare(datap[heap[index]],datap[heap[newindex]])<=0)
          ; /* break */
       else
         {
          temp=heap[newindex];
          heap[newindex]=heap[index];
          heap[index]=temp;
         }
      }
   }
 while(ndata>0);

 /* Tidy up */

 tidy_and_exit:

 if(fds)
   {
    for(i=0;i<nfiles;i++)
       CloseFile(fds[i]);
    free(fds);
   }

 if(heap)
    free(heap);

 for(i=0;i<option_filesort_threads;i++)
   {
    free(threads[i].data);

    free(threads[i].filename);
   }

 return(count);
}


/*++++++++++++++++++++++++++++++++++++++
  A wrapper function that can be run in a thread for fixed data.

  void *filesort_fixed_heapsort_thread Returns NULL (required to return void*).

  thread_data *thread The data to be processed in this thread.
  ++++++++++++++++++++++++++++++++++++++*/

static void *filesort_fixed_heapsort_thread(thread_data *thread)
{
 int fd,i;

 /* Sort the data pointers using a heap sort */

 filesort_heapsort(thread->datap,thread->n,thread->compare);

 /* Create a temporary file and write the result */

 fd=OpenFileNew(thread->filename);

 for(i=0;i<thread->n;i++)
    WriteFile(fd,thread->datap[i],thread->itemsize);

 CloseFile(fd);

 thread->running=2;

 return(NULL);
}


/*++++++++++++++++++++++++++++++++++++++
  A wrapper function that can be run in a thread for variable data.

  void *filesort_vary_heapsort_thread Returns NULL (required to return void*).

  thread_data *thread The data to be processed in this thread.
  ++++++++++++++++++++++++++++++++++++++*/

static void *filesort_vary_heapsort_thread(thread_data *thread)
{
 int fd,i;

 /* Sort the data pointers using a heap sort */

 filesort_heapsort(thread->datap,thread->n,thread->compare);

 /* Create a temporary file and write the result */

 fd=OpenFileNew(thread->filename);

 for(i=0;i<thread->n;i++)
   {
    FILESORT_VARINT itemsize=*(FILESORT_VARINT*)(thread->datap[i]-FILESORT_VARSIZE);

    WriteFile(fd,thread->datap[i]-FILESORT_VARSIZE,itemsize+FILESORT_VARSIZE);
   }

 CloseFile(fd);

 thread->running=2;

 return(NULL);
}


/*++++++++++++++++++++++++++++++++++++++
  A function to sort an array of pointers efficiently.

  The data is sorted using a "Heap sort" http://en.wikipedia.org/wiki/Heapsort,
  in particular, this is good because it can operate in-place and doesn't
  allocate more memory like using qsort() does.

  void **datap A pointer to the array of pointers to sort.

  size_t nitems The number of items of data to sort.

  int(*compare)(const void *, const void *) The comparison function (identical to qsort if the
                                            data to be sorted was an array of things not pointers).
  ++++++++++++++++++++++++++++++++++++++*/

void filesort_heapsort(void **datap,size_t nitems,int(*compare)(const void*, const void*))
{
 void **datap1=&datap[-1];
 int i;

 /* Fill the heap by pretending to insert the data that is already there */

 for(i=2;i<=nitems;i++)
   {
    int index=i;

    /* Bubble up the new value (upside-down, put largest at top) */

    while(index>1)
      {
       int newindex;
       void *temp;

       newindex=index/2;

       if(compare(datap1[index],datap1[newindex])<=0) /* reversed compared to filesort_fixed() above */
          break;

       temp=datap1[index];
       datap1[index]=datap1[newindex];
       datap1[newindex]=temp;

       index=newindex;
      }
   }

 /* Repeatedly pull out the root of the heap and swap with the bottom item */

 for(i=nitems;i>1;i--)
   {
    int index=1;
    void *temp;

    temp=datap1[index];
    datap1[index]=datap1[i];
    datap1[i]=temp;

    /* Bubble down the new value (upside-down, put largest at top) */

    while((2*index)<(i-1))
      {
       int newindex;
       void *temp;

       newindex=2*index;

       if(compare(datap1[newindex],datap1[newindex+1])<=0) /* reversed compared to filesort_fixed() above */
          newindex=newindex+1;

       if(compare(datap1[index],datap1[newindex])>=0) /* reversed compared to filesort_fixed() above */
          break;

       temp=datap1[newindex];
       datap1[newindex]=datap1[index];
       datap1[index]=temp;

       index=newindex;
      }

    if((2*index)==(i-1))
      {
       int newindex;
       void *temp;

       newindex=2*index;

       if(compare(datap1[index],datap1[newindex])>=0) /* reversed compared to filesort_fixed() above */
          ; /* break */
       else
         {
          temp=datap1[newindex];
          datap1[newindex]=datap1[index];
          datap1[index]=temp;
         }
      }
   }
}
