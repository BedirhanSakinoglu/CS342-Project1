#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dma.h" // include the library interface

#define MIN_M_SIZE 14
#define MAX_M_SIZE 22

int* addr;

int segment_size;
int bitmap_size;
int allocation_size;
int reserved_size = 256;
int total_frag = 0;

pthread_mutex_t mutex;

//----------------------------------------
//Bit Array Functions
void  set_bit(int A[],  int bit_index, int value)
{
    if(value == 1){
        A[bit_index/32] |= 1 << (bit_index%32);
    }
    else if(value == 0){
        A[bit_index/32] &= ~(1 << (bit_index%32));
    }
}
//----------------------------------------

int dma_init (int m){

    if(m < MIN_M_SIZE || m > MAX_M_SIZE){
        printf("\nERROR! Value of m must be between min and max value");
        return -1;
    }

    /*
    int segment_size;
    int bitmap_size;
    int allocation_size;
    int reserved_size = 256;
    */

    //Find 2^m
    segment_size = 1;
    for(int i = 0 ; i < m ; i++){
        segment_size = segment_size*2;
    }

    bitmap_size = segment_size/64;
    allocation_size = segment_size-(reserved_size+bitmap_size);

    int bitmap[bitmap_size];
    int reserved[reserved_size];
    int allocation[allocation_size];
    int segment[segment_size];

    addr = mmap(0, segment_size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    
    for(int i = 0 ; i < bitmap_size*8 ; i++){
        if(i < 32){
            set_bit(bitmap, i, 0);
        }
        else{
            set_bit(bitmap, i, 1);
        }
    }
    set_bit(bitmap, 1, 1);

    for(int i = 0 ; i < segment_size ; i++){
        if(i<bitmap_size){
            segment[i] = bitmap[i];
        }
        else if(i<bitmap_size+reserved_size){
            segment[i] = reserved[i];
        }
        else if(i>=bitmap_size+reserved_size){
            segment[i] = allocation[i];
        }
        else{
            printf("\n Error: %d",i);
        }
    }

    addr = segment;

    //return addr;
    return 0;
}

void *dma_alloc (int size){

    pthread_mutex_lock(&mutex);

    int allocation_size = 1;
    while (allocation_size < size){
        allocation_size = allocation_size + 16 ;
    }

    total_frag = total_frag + (allocation_size-size);

    int counter = 0;
    for(int i = 0 ; i < bitmap_size*8 ; i++){
        if((addr[i/32]>>(i%32))&1){
            counter = counter + 1;
        }
        else{
            counter = -1;
        }

        if(counter == allocation_size/8){
            //Allocate
            for(int t = i-(allocation_size/8) ; t < i; t++){
                if(t == i-(allocation_size/8)+1){
                    set_bit(addr, t, 1);
                }
                else{
                    set_bit(addr, t, 0);
                }
            }
            int allocated_space_index = bitmap_size + reserved_size + i-(allocation_size/8);
            int* ptr = &addr[allocated_space_index];

            pthread_mutex_unlock(&mutex);
            return ptr;
        }
    }
    printf("\nNo suitable location found\n");
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void dma_free (void *p){
    pthread_mutex_lock(&mutex);

    int index_addr = ((int*)p)-&addr[0];
    int index_bitmap = index_addr - bitmap_size - reserved_size;
    int is_freed = 0;

    for(int i=index_bitmap ; i < bitmap_size ; i=i+2){
        if((addr[i/32]>>(i%32))&1){
            if((addr[(i+1)/32]>>((i+1)%32))&1){
                break;
            }
        }
        else{
            if((addr[(i+1)/32]>>((i+1)%32))&1 & i!=index_bitmap){
                break;
            }
            else{
                set_bit(addr, i, 1);
                set_bit(addr, i+1, 1);
            }
        }
    }

    pthread_mutex_unlock(&mutex);
}

void dma_print_page(int pno){
    int start_index;
    int page_size;
    
    start_index = 0;
    page_size = 1;

    for(int t = 0 ; t < 12 ; t++){
        page_size = page_size*2;
    }

    start_index = page_size*pno;

    int value;
    int counter=0;
    char hex[page_size];

    printf("\nPage: %d\n", pno);

    for(int i = start_index ; i < start_index+page_size ; i++){
        sprintf(&hex, "%x", addr[i]);
        if(counter + strlen(hex) >= 64){
            int remaining_count = 64-counter;
            for(int t = 0; t < remaining_count ; t++){
                printf("%c", hex[t]);
            }
            printf("\n");
            counter = 0;
            for(int t = remaining_count; t < strlen(hex) ; t++){
                printf("%c", hex[t]);
                counter = counter + 1;
            }
        }
        else{
            printf("%s", hex);
            counter = counter + strlen(hex);
        }
    }

    printf("\n\n");
    
}

void dma_print_bitmap(){
    printf("\n\n--------------------------------------------------------");
    printf("\nBitMap\n");
    for(int i = 0 ; i < bitmap_size*8 ; i++){
        if(i%32 == 0){
            printf("\n");
        }
        else if(i%8 == 0){
            printf(" ");
        }

        if((addr[i/32]>>(i%32))&1) {
            printf("1");
        } 
        else {
            printf("0");
        } 
    }
    printf("\n\n--------------------------------------------------------");
}

void dma_print_blocks(){

    char* is_occupied = "";
    int address;
    int occupied_space;
    int index;
    int counter = 0;

    int empty_flag = 0;
    int empty_count = 0;
    int alloc_flag = 0;
    int alloc_count = 0;
    int finish_flag = 0;
    
    while(finish_flag == 0){
        
        for(int i = counter ; i < bitmap_size*8 ; i=i+2){
            if((addr[i/32]>>(i%32))&1){
                if(alloc_flag == 1){
                    alloc_flag = 0;
                    occupied_space = alloc_count*8;
                    is_occupied = "A";
                    counter = i;
                    break;
                }
                else if((addr[(i+1)/32]>>((i+1)%32))&1){
                    empty_flag = 1;
                    empty_count = empty_count + 2;
                }
            }
            else{
                if((addr[(i+1)/32]>>((i+1)%32))&1){
                    if(empty_flag == 1){
                        empty_flag = 0;
                        occupied_space = empty_count*8;
                        is_occupied = "F";
                        counter = i;
                        break;
                    }
                    else if(alloc_flag == 1){
                        alloc_flag = 0;
                        occupied_space = alloc_count*8;
                        is_occupied = "A";
                        counter = i;
                        
                        break;
                    }
                    else{
                        alloc_flag = 1;
                        alloc_count = alloc_count + 2;
                    } 
                }
                else{
                    alloc_count = alloc_count + 2;
                }
            }

            if(i == bitmap_size*8-2){
                finish_flag = 1;
                if(empty_flag == 1){
                    empty_flag = 0;
                    occupied_space = empty_count*8;
                    is_occupied = "F";
                    counter = i;
                    break;
                }
                else if(alloc_flag == 1){
                    alloc_flag = 0;
                    occupied_space = alloc_count*8;
                    is_occupied = "A";
                    counter = i;
                    break;
                }
            }
        }
        index = counter-empty_count;
        address = &addr[8*index + bitmap_size];
        printf("\n%s ,0x%018x , 0x%05x, (%d) \n", is_occupied, address, occupied_space, occupied_space);
        empty_flag = 0;
        alloc_flag = 0;
    } 

}

int dma_give_intfrag(){
    return total_frag;
}

int main (int argc, char ** argv)
{
    void *p1;
    void *p2;
    void *p3;
    void *p4;
    int ret;

    ret = dma_init (15); // create a segment
    if (ret != 0) {
        printf ("something was wrong\n");
        exit(1);
    }
    
    p1 = dma_alloc (100); // allocate space for 100 bytes
    p2 = dma_alloc (1024);
    p3 = dma_alloc (48); //always check the return value
    p4 = dma_alloc (220);

    dma_print_bitmap();
    printf("\n************************************");
    printf("\nTotal Frag: %d\n", dma_give_intfrag());
    printf("************************************\n");
    
    dma_free (p3);
    //dma_print_page(0);
    
    p3 = dma_alloc (2048);

    //dma_print_bitmap();
    dma_print_blocks();
    
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);

    dma_print_bitmap();
    
}