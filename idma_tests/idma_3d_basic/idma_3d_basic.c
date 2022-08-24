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
              unsigned int *stride, unsigned int length, unsigned int ext2loc);

int main() {

  if (rt_cluster_id() != 0) return bench_cluster_forward(0);

  int len = 1;
  int str[2] = {0,0};
  int error_count = 0;
  int core_id = get_core_id();
  
  for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
    for (str[0] = i; str[0] < MAX_BUFFER_SIZE; str[0] = 2 * str[0]) {
      for(str[1] = str[0]; str[1] < MAX_BUFFER_SIZE; str[1] = 2 * str[1]) {
        error_count += test_idma(EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                                 TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i, str, len, 1);
      }
    }
  }
  
   
  for (int i = 1; i < MAX_BUFFER_SIZE; i = 2 * i) {
     for (str[0] = i; str[0] < MAX_BUFFER_SIZE; str[0] = 2 * str[0]) {
       for(str[1] = str[0]; str[1] < MAX_BUFFER_SIZE; str[1] = 2 * str[1]) {
         error_count += test_idma(TCDM_DATA_ADDR + (core_id*MAX_BUFFER_SIZE),
                                  EXT_DATA_ADDR + (core_id*MAX_BUFFER_SIZE), i, str, len, 1);
       }
     }
   }
  return error_count;
}

int test_idma(unsigned int src_addr, unsigned int dst_addr, unsigned int num_bytes,
              unsigned int *stride, unsigned int length, unsigned int ext2loc){

#ifdef VERBOSE
  // printf("STARTING TEST FOR %d TRANSFERS FROM %p TO %p\n", num_bytes, src_addr,
        //  dst_addr);
#endif

  for (int i = 0;  i < length/num_bytes; i++){
    for (int j = 0; j < length/num_bytes; j++){
      for (int k = 0; k < num_bytes; k++){
        *(unsigned char*)(src_addr + stride[0]*i + stride[1]*j + k) =
                                    (stride[0]*i + stride[1]*j + k) & 0xFF;
      }
    }
  }
        
  for (int i = 0; i < length/(num_bytes+16); i++){
    for (int j = 0; j < length/(num_bytes+16); j++){
      for (int k = 0; k < num_bytes + 16; k++){
        *(unsigned char*)(dst_addr + stride[0]*i + stride[1]*j + k) = 0;
      }
    }
  }
    
  synch_barrier();

  unsigned int dma_tx_id = plp_dma_memcpy_3d(dst_addr, src_addr, num_bytes, stride, length, 1);

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
    for(int j = 0; j < length/num_bytes; j++){
      for (int k = 0; k < num_bytes; k++){
        test = (stride[0]*i + stride[1]*j + k) & 0xFF;
        read = *(unsigned char*)(dst_addr + stride[0]*i + stride[1]*j + k);
        if (test != read) {
          printf("Error!!! Read: %x, Test:%x, Index: %d, %d, %d\n ", read, test, i, j, k);
          error++;
        }
      }
    }
  }

  for (int i = num_bytes; i < length/(num_bytes+16); i++) {
    for(int j = num_bytes; j < length/(num_bytes+16); j++){
      for (int k = 0; k < num_bytes + 16; k++){
        test = 0;
        read = *(unsigned char*)(stride[0]*i + stride[1]*j + k);
        
        if (test != read) {
          printf("Error!!! Read: %x, Test:%x, Index: %d, %d, %d \n ", read, test, i, j, k);
          error++;
        }
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
