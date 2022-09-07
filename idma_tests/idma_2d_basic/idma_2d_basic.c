#include <math.h>
#include "pulp.h"
#include <stdio.h>
// #include "pulp_idma.h"
#include <hal/dma/idma_v1.h>

#define VERBOSE
#define MAX_BUFFER_SIZE 0x400

L2_DATA static unsigned char ext[MAX_BUFFER_SIZE*8];
L1_DATA static unsigned char loc[MAX_BUFFER_SIZE*8];

#define EXT_DATA_ADDR ((unsigned int)ext)
#define TCDM_DATA_ADDR ((unsigned int)loc)

int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int stride, unsigned int length, unsigned int ext2loc);

int main() {

  if (rt_cluster_id() != 0) return bench_cluster_forward(0);

  int error_count = 0;
  int core_id = get_core_id();

  for (int i = 64; i < 2049; i = 2 * i) {
    for(int j = 8; j < 33; j = 2 * j) {
      error_count += test_idma(EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                               TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i, 64, j, 1);
    }
  }  
  
      // for(int j = 1; j < 3; j = 2 * j) {
      //for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
      //      error_count += test_idma(TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
      //                       EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), 1, 16, 1, 0);
      // }
      // }
  return error_count;
  
  }

int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int stride, unsigned int length, unsigned int ext2loc){

#ifdef VERBOSE
  // printf("STARTING TEST FOR %d TRANSFERS FROM %p TO %p\n", num_bytes, src_addr,
        //  dst_addr);
#endif

  float time = 0;
  unsigned int reps = num_bytes/length;
  unsigned int i,j;
  
  for (i = 0;  i < reps; i++){
    for (j = 0; j < length; j++){
      *(unsigned char*)(src_addr + stride*i + j) = (stride*i + j) & 0xFF;
    }
  }
  
  for (i = 0; i < reps; i++){
    for (j = 0; j < length; j++){
      *(unsigned char*)(dst_addr + stride*i + j) = 0;
    }
  }
  
    
  synch_barrier();

  reset_timer();
  
  start_timer();
  
  unsigned int dma_tx_id = plp_dma_memcpy_2d(src_addr, dst_addr, num_bytes, stride, length, ext2loc);

  plp_dma_wait(dma_tx_id);

  time = get_time();

  stop_timer();
    
  unsigned int test, read;
  unsigned int error = 0;

  for (i = 0; i < reps; i++) {
    for(j = 0; j < length; j++){
      test = (stride*i + j) & 0xFF;
      read = *(unsigned char*)(dst_addr + stride*i + j);
      if(get_core_id() == 0){
        //printf("test = %p, read = %p, i = %d, j = %d\n", test, read, i, j);
        if (test != read) {
          printf("Error!!! Read: %x, Test:%x, Index: %d, %d \n ", read, test, i, j);
          error++;
        } else {
          //        printf("OK!!! Read: %x, Test:%x, Index: %d, %d \n ", read, test, i, j);
        }
      }
    }
  }
  
  
  /*for (int i = num_bytes/length; i < (num_bytes/length)+16; i++) {
    for(int j = 0; j < length; j++){
      test = 0;
      read = *(unsigned char*)(dst_addr + stride*i + j);
      if (test != read) {
        printf("Error2!!! Read: %x, Test:%x, Index: %d, %d \n ", read, test, i, j);
        error++;
      }
    }
    }*/
  
  if (get_core_id() == 0) {
    if (error == 0){
      printf("OOOOOOK!!!!!!\n");
      printf("TIME: %f, NUM_BYTES: %d, STRIDE: %d, LENGTH: %d\n", time, num_bytes, stride,  length);
    }else{
      printf("NOT OK!!!!!\n");
      printf("TIME: %f, NUM_BYTES: %d, STRIDE: %d, LENGTH: %d\n", time, num_bytes, stride,  length);
    }
  }

  return error;
}
