#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define META_SIZE sizeof(struct blk_meta_data)
#define MIN_BLK_SIZE 8
#define SBRK_NULL ((void*) -1)

struct blk_meta_data* ptr_blk_list = NULL;

//storing meta information
struct blk_meta_data {
    size_t size;
    int is_free;
    void* ptr;
    struct blk_meta_data* next;
};

int find_free_blk(struct blk_meta_data** last ,size_t size){
    struct blk_meta_data* current = *last;

    while(current){
        if(current->is_free==1 && current->size >= size){//valid block
            *last = current;
            return 1;
        }
        *last = current;
        current = current->next;
    }
    return 0;
}

struct blk_meta_data* request_space(size_t size){

    struct blk_meta_data* block_mdata;
    void* ptr  = sbrk(0);
    block_mdata=ptr;
    size_t blk_size = size + META_SIZE;

    //align to 8 bits
    blk_size = (blk_size+7) & ~7;

    void* request = sbrk(blk_size);

    if(request == SBRK_NULL) return NULL;
    assert((void*)block_mdata == request);
    
    block_mdata->ptr = ptr+META_SIZE;
    block_mdata->is_free=0;
    block_mdata->size = size;
    block_mdata->next=NULL;

    return block_mdata;

}

struct blk_meta_data* get_mdata_ptr(void* ptr){
    return (struct blk_meta_data*)ptr-1;
}

void _free(void* ptr){
    if(!ptr) return;

    struct blk_meta_data* mdata = get_mdata_ptr(ptr);
    assert(mdata->is_free==0);
    mdata->is_free=1;

}

void* _malloc(size_t size){
    struct blk_meta_data* allocating_block=NULL;
    if(size<=0) return NULL;

    if(!ptr_blk_list){

        allocating_block = request_space(size);
        ptr_blk_list = allocating_block;
        if(!allocating_block) return NULL;

    }else{

        struct blk_meta_data* last = ptr_blk_list;
        if(!find_free_blk(&last, size)){

            allocating_block = request_space(size);
            if(!allocating_block) return NULL;

            last->next = allocating_block;

        }else{

            allocating_block = last;
            // struct blk_meta_data* curr = last;
            size_t tot_size =allocating_block->size;
            size_t min_size = META_SIZE+MIN_BLK_SIZE;

            if((tot_size - size - META_SIZE) >= min_size){

                allocating_block->size = size;
                allocating_block->is_free = 0;

                struct blk_meta_data* new_block;
                new_block = (struct blk_meta_data*)((uint8_t*)allocating_block->ptr + size);

                //Creating split block
                new_block->ptr = (uint8_t*)allocating_block->ptr + size + META_SIZE;
                new_block->is_free=1;
                new_block->size = tot_size - size - META_SIZE;
                new_block->next = allocating_block->next;
                
                //Including the split block to blk_meta_data
                allocating_block->next = new_block;

            }else{

                allocating_block->is_free = 0;

            }
        }
    }

    printf("%zu bytes allocated\n", allocating_block->size);

    return allocating_block->ptr;
}

void* _calloc(size_t n, size_t size){
    void* blk_ptr = _malloc(n*size);
    memset(blk_ptr, 0, n*size);

    return blk_ptr;
}

void* _realloc(void* ptr, size_t size){
    if(!ptr) return _malloc(size);

    struct blk_meta_data* mdata = get_mdata_ptr(ptr);
    if(mdata->size >= size) return ptr;

    void* new_ptr = _malloc(size);
    if(!new_ptr) return NULL;
    memcpy(new_ptr, ptr, mdata->size);
    if(new_ptr) _free(ptr);

    printf("realloc succesful, size = %zu\n", get_mdata_ptr(new_ptr)->size);

    return new_ptr;
}

int main(){
    printf("%lu\n", META_SIZE);
    int *x = _malloc(256);
    if(x!=NULL) printf("Successful\n"); 
    int *y = _calloc(256, sizeof(int));
    printf("if calloc works zero, y[0]=%d\n", y[0]);
    _free(y);

    x = _realloc(x, 300);
    _free(x);

    return 0;
}
