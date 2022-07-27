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

//int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
//              unsigned int dst_stride, unsigned int src_stride, unsigned int num_reps);
int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int stride, unsigned int length, unsigned int ext2loc);

int main() {

  if (rt_cluster_id() != 0) return bench_cluster_forward(0);

  int k = 1;
  int error_count = 0;
  int core_id = get_core_id();
  
  // if (get_core_id() == 0) {

  /* for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
      error_count += test_idma(EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                               TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i);
    }
    for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
      error_count += test_idma(TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                               EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i);
    }*/
  // }

   for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
     for(int j = i; j < MAX_BUFFER_SIZE; j = 2 * j) {
         error_count += test_idma(EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                                  TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i, j, k, 1);
     }
   }
  
   
   for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
     for(int j = i; j < MAX_BUFFER_SIZE; j = 2 * j) {
         error_count += test_idma(TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                                  EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i, j, k, 1);
     }
   }
  return error_count;
}

//int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
//              unsigned int dst_stride, unsigned int src_stride, unsigned int num_reps){
int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int stride, unsigned int length, unsigned int ext2loc){

#ifdef VERBOSE
  // printf("STARTING TEST FOR %d TRANSFERS FROM %p TO %p\n", num_bytes, src_addr,
        //  dst_addr);
#endif

  /*for (int i = 0; i < num_bytes; i++) {
    *(unsigned char*)(src_addr + i) = i & 0xFF;
  }

  for (int i = 0; i < num_bytes + 16; i++) {
    *(unsigned char*)(dst_addr + i) = 0;
    }*/

  for (int i = 0;  i < length/num_bytes; i++){
    for (int j = 0; j < num_bytes; j++){
      *(unsigned char*)(src_addr + stride*i + j) = (stride*i + j) & 0xFF;
    }
  }
    
        
  for (int i = 0; i < length/(num_bytes+16); i++){
    for (int j = 0; j < num_bytes + 16; j++){
      *(unsigned char*)(dst_addr + stride*i + j) = 0;
      }
    }

    
  synch_barrier();

  unsigned int dma_tx_id = plp_dma_memcpy_2d(dst_addr, src_addr, num_bytes, stride, length, 1);

  // printf("After launch, dma_id: %x, %x, %x\n", dma_tx_id, (unsigned int
  // *)(dma_ptr + CLUSTER_DMA_FRONTEND_DONE_REG_OFFSET), (unsigned int
  // *)(dma_ptr + CLUSTER_DMA_FRONTEND_DONE_REG_OFFSET + ((dma_tx_id &
  // 0xf0000000) >> 26)));

  // while((dma_tx_id & 0x0fffffff) > *(unsigned int *)(dma_ptr +
  // CLUSTER_DMA_FRONTEND_DONE_REG_OFFSET + ((dma_tx_id & 0xf0000000) >> 26))) {
  //   asm volatile("nop");
  // }
  plp_dma_wait(dma_tx_id);

  unsigned int test, read;
  unsigned int error = 0;

  for (int i = 0; i < length/num_bytes; i++) {
    for(int j = 0; j < num_bytes; j++){
      test = (stride*i + j) & 0xFF;
      read = *(unsigned char*)(dst_addr + stride*i + j);
      if (test != read) {
        printf("Error!!! Read: %x, Test:%x, Index: %d, %d \n ", read, test, i, j);
        error++;
      }
    }
  }

  for (int i = num_bytes; i < length/(num_bytes+16); i++) {
   for(int j = num_bytes; j < num_bytes + 16; j++){
 
    test = 0;
    read = *(unsigned char*)(dst_addr + stride*i + j);

    if (test != read) {
      printf("Error!!! Read: %x, Test:%x, Index: %d, %d \n ", read, test, i, j);
      error++;
    }
  }
}
  
  if (get_core_id() == 0) {
    if (error == 0)
      printf("OOOOOOK!!!!!!\n");
    else
      printf("NOT OK!!!!!\n");
  }

  return error;
}
