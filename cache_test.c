#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>


#define CACHE_ITERS 4 // Number of iters that should be necessary fot 
                      //the prefetching of the data


#define ARRAY_SIZE 64*1024*1024
#define CACHE_SIZE 8*1024*1024

static uint64_t b1[ARRAY_SIZE]; // b1 is the buffer to evaluate (cold/warm)
static uint64_t b2[ARRAY_SIZE]; // b2 is the destination/source of the operation (always warm)
																// R = b2 <- b1
																// W = b1 <- b2
static uint64_t b3[ARRAY_SIZE];

enum OPERATION {
  NONE,  
  R,    // Read
  W,    // Write
  RW,   // Read follow by a write
  WR    // Write follow by a read
};


struct arguments {
  char     op;
  uint64_t size;
};



void printUsage(int argc, char **argv) {
  printf("Usage mode\n"
       "\t%s [R|W|RW|WR] size\n", argv[0]);
}



static uint64_t string2bytes(char *s) {
  int rmember;
  char unit;
  uint64_t size;

  rmember = sscanf (s, "%ld%c", &size, &unit);

  if (rmember == 2) {
    if (toupper (unit) == 'G') {
      size *= 1024 * 1024 * 1024;
    } else if (toupper (unit) == 'M') {
      size *= 1024 * 1024;
    } else if (toupper (unit) == 'K') {
      size *= 1024;
    }
  }  
  return size;
}


int parseArguments(int argc, char **argv, struct arguments *a) {
  if(argc!=3) {
    return -1;
  }

  if(!strcmp(argv[1], "r") || !strcmp(argv[1], "R")) {
    a->op=R;
  } else if(!strcmp(argv[1], "w") || !strcmp(argv[1], "W")) {
    a->op=W;
  } else if(!strcmp(argv[1], "rw") || !strcmp(argv[1], "RW")) {
    a->op=RW;
  } else if(!strcmp(argv[1], "wr") || !strcmp(argv[1], "WR")) {
    a->op=WR;
  } else {
    return -1;
  }

  a->size=string2bytes(argv[2]);
  if(!a->size || a->size % 8) {
    printf("Size must be a multiple of 64 bits\n");
    return -1;
  }

  return 0;
}


/* Code adapt from mfp-pciebench-helper.c
 * Before starting a test we aim thrash the cache by randomly
 * writing to elements in a 64MB large array.
 */
static void thrash_cache(uint64_t *pmem, uint64_t total_size) {
  uint64_t i, r;
  for (i = 0; i < CACHE_ITERS*CACHE_SIZE; i++) {
    r = rand();
    pmem[r % (CACHE_SIZE)/sizeof(uint64_t)] = (uint64_t)i * r;
  }
}


/* Warm the host buffers for a given window size. The window size is
 * rounded up to the nearest full page. */
static void warm_cache(uint64_t *pmem, uint64_t total_size) {
  uint64_t i;
  for (i = 0; i < CACHE_ITERS*total_size; i++) {
    pmem[i % (total_size/sizeof(uint64_t))] = (uint64_t)i;
  }
}

#define TIC(a) gettimeofday(&(a),NULL)
#define TOC(b) gettimeofday(&(b),NULL)
#define PT(a,b,c) timersub(&(b),&(a),&(c)); printf("%d s %d us", c.tv_sec, c.tv_usec)

int main(int argc, char **argv) {
  struct arguments args;
  struct timeval f,l,d;

  if(parseArguments(argc, argv, &args)){
    printUsage(argc,argv);
    return -1;
  }

  printf("Cold\tWarm\n");
  switch(args.op) {
    case W:
      thrash_cache(b3, args.size);
      warm_cache(b2, args.size);
      TIC(f);
      memcpy(b1,b2,args.size);
      TOC(l); 
      PT(f,l,d); printf("\t");
      warm_cache(b2, args.size);
      warm_cache(b1, args.size);
      TIC(f);      
      memcpy(b1,b2,args.size);
      TOC(l);
      PT(f,l,d); printf("\n");
      break;
    case R: 
      thrash_cache(b3, args.size);
      warm_cache(b2, args.size);
      TIC(f);      
      memcpy(b2,b1,args.size);
      TOC(l); 
      PT(f,l,d);  printf("\t");     
      warm_cache(b2, args.size);
      warm_cache(b1, args.size);
      TIC(f);
      memcpy(b2,b1,args.size);    
      TOC(l); 
      PT(f,l,d); printf("\n");   
      break;
    case WR:      
      thrash_cache(b3, args.size);
      warm_cache(b2, args.size);
      TIC(f);
      memcpy(b1,b2,args.size);
      memcpy(b2,b1,args.size);
      TOC(l); 
      PT(f,l,d);  printf("\t");      
      warm_cache(b2, args.size);
      warm_cache(b1, args.size);
      TIC(f);
      memcpy(b1,b2,args.size);
      memcpy(b2,b1,args.size);
      TOC(l); 
      PT(f,l,d);  printf("\n");        
      break;
    case RW:      
      thrash_cache(b3, args.size);
      warm_cache(b2, args.size);
      TIC(f);
      memcpy(b2,b1,args.size);
      memcpy(b1,b2,args.size);
      TOC(l); 
      PT(f,l,d);  printf("\t");        
      warm_cache(b2, args.size);
      warm_cache(b1, args.size);
      TIC(f);
      memcpy(b2,b1,args.size);
      memcpy(b1,b2,args.size);      
      TOC(l); 
      PT(f,l,d);  printf("\n");        
      break;
  }            


  return 0;
}