#include <math.h>
#include "pulp.h"
#include <stdio.h>
// #include "pulp_idma.h"
#include <hal/dma/idma_v1.h>

#define VERBOSE
#define MAX_BUFFER_SIZE 0x1000

L2_DATA static unsigned char ext[MAX_BUFFER_SIZE*8];
L1_DATA static unsigned char loc[MAX_BUFFER_SIZE*8];

#define EXT_DATA_ADDR ((unsigned int)ext)
#define TCDM_DATA_ADDR ((unsigned int)loc)

#define DMA_3D_SUPPORT

int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int *stride, unsigned int length, unsigned int *num_reps, unsigned int ext2loc);

int main() {

  if (rt_cluster_id() != 0) return bench_cluster_forward(0);

  int error_count = 0;
  int core_id = get_core_id();

  int len = 8;
  int str[2] = {8, 1024};
  int reps[2] = {0, 1};
  int bytes = 0;

  //printf("%d\n",core_id);

  //if(core_id==0) {
  //for (int i = 4; i < 32 + 1; i = 2 * i) {
    // for (int j = 1; j < 33; j = 2 * j) {
      reps[0] = 256;
      bytes = len*(reps[0]*reps[1]);
      //    printf(" src : %x , dst %x \n", EXT_DATA_ADDR, TCDM_DATA_ADDR);
      error_count += test_idma(EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                               TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), bytes, str, len, reps, 1);
      //}
    //}
    //}
  
  return error_count;
}

int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int *stride, unsigned int length, unsigned int *num_reps, unsigned int ext2loc){
  
#ifdef VERBOSE
  // printf("STARTING TEST FOR %d TRANSFERS FROM %p TO %p\n", num_bytes, src_addr,
        //  dst_addr);
#endif

  float time = 0;
  unsigned int i,j,k;

  /* if(get_core_id() == 0){
    printf("stride0 = %d, stride1 = %d, num_reps0 = %d, num_reps1 = %d, length = %d, num_bytes = %d\n",
           stride[0], stride[1], num_reps[0], num_reps[1], length, num_bytes);
           }*/
  
  for (i = 0;  i < num_reps[1]; i++){
    for (j = 0; j < num_reps[0]; j++){
      for (k = 0; k < length; k++){
        *(unsigned char*)(src_addr + stride[1]*i + stride[0]*j + k) =
                                    (stride[1]*i + stride[0]*j + k) & 0xFF;
      }
    }
  }

  /*if(get_core_id() == 0){
  for (i = 0;  i < num_reps[1]; i++){
    for (j = 0; j < num_reps[0]; j++){
      for (k = 0; k < length; k++){
        printf(" *(src_addr + %d*%d + %d*%d + %d) = %p\n",
               stride[1], i, stride[0], j, k, *(unsigned char*)(src_addr + stride[1]*i + stride[0]*j + k));
      }
    }
  }
  }*/
  
  for (i = 0; i < num_reps[1]; i++){
    for (j = 0; j < num_reps[0]; j++){
      for (k = 0; k < length; k++){
        *(unsigned char*)(dst_addr + stride[1]*i + stride[0]*j + k) = 0;
      }
    }
  }
    
  synch_barrier();

  reset_timer();
  
  start_timer();
  
  #ifdef DMA_3D_SUPPORT
  unsigned int dma_tx_id = plp_dma_memcpy_3d(src_addr, dst_addr, num_bytes, stride, length, num_reps, 1);
  plp_dma_wait(dma_tx_id);
  #else
  for(i = 0; i < num_reps[1]; i++){
    unsigned int dma_tx_id = plp_dma_memcpy_2d(src_addr+stride[1]*i, dst_addr+stride[1]*i,
                                               num_bytes/num_reps[1], stride[0], length, 1);
    plp_dma_wait(dma_tx_id);
  }
  #endif

  time = get_time();
  
  stop_timer();
  
  //  plp_dma_wait(dma_tx_id);

  unsigned int test, read;
  unsigned int error = 0;

  for (i = 0; i < num_reps[1]; i++) {
    for(j = 0; j < num_reps[0]; j++){
      for (k = 0; k < length; k++){
        test = (stride[1]*i + stride[0]*j + k) & 0xFF;
        read = *(unsigned char*)(dst_addr + stride[1]*i + stride[0]*j + k);
        if(get_core_id() == 0){
          //printf("test = %p, read = %p, i = %d, j = %d, k = %d\n", test, read, i, j, k);
          if (test != read) {
            //  printf("Error!!! Read: %x, Test:%x, Index: %d, %d, %d\n ", read, test, i, j, k);
            error++;
          } else {
            //printf("OK!!! Read: %x, Test:%x, Index: %d, %d, %d \n ", read, test, i, j, k);
          }
        }
      }
    }
  }

  /*
  if (get_core_id() == 0) {
    for(i=0; i<500; i++){
      test = i & 0xFF;
      read = *(unsigned char*)(dst_addr + i);
      printf("test = %p, read = %p, i = %d\n", test, read, i);
    }
  }*/
  
  if (get_core_id() == 0) {
    if (error == 0){
      printf("OOOOOOK!!!!!!\n");
      printf("TIME: %f, NUM_BYTES: %d, STRIDE[0]: %d, stride[1]: %d, NUM_REPS[0]: %d, NUM_REPS[1]: %d, LENGTH: %d\n",
             time, num_bytes, stride[0], stride[1],  num_reps[0], num_reps[1], length);
    }
    else {
      printf("NOT OK!!!!!\n");
      printf("TIME: %f, NUM_BYTES: %d, STRIDE[0]: %d, stride[1]: %d, NUM_REPS[0]: %d, NUM_REPS[1]: %d, LENGTH: %d\n",
             time, num_bytes, stride[0], stride[1],  num_reps[0], num_reps[1], length);
    }
  }

  // printf("TIME: %f, SIZE: %d\n", time, length);
  
  return error;
}
