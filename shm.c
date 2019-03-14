#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
      struct shm_get_page(int);
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

//Gets shared mem page id
struct shm_page * shm_get_page(int id){
    uint i;     //have to declare i outside of loop??
    
    for(i = 0; i < 64; ++i){
        if(shm_table.shm_pages[i].id == id){
            return shm_table.shm_pages + i;
        }
    }
    return 0;
}



int shm_open(int id, char **pointer) {

//you write this
    struct shm_page *pg = 0;            //have to declare stuct??
    struct proc *currproc = myproc();
    
    char *va = (char*)PGROUNDUP(currproc->sz);
    
    acquire(&(shm_table.lock));

    pg = shm_get_page(id);
    
    if(pg != 0){
        if(!mappages(currproc->pgdir, va, PGSIZE, V2P(pg -> frame), PTE_W | PTE_U)){
            currproc -> sz = (uint)va + PGSIZE;
            ++(pg->refcnt);
            *pointer = va;
        }
        else{
            cprintf("Error: Shared page with id %d could not be mapped. \n", id);
            release(&(shm_table.lock));
            return -1;
        }
    }
    else{
        uint i;
        for(i = 0; i < 64; ++i){
            if(!shm_table.shm_pages[i].frame){
                pg = shm_table.shm_pages + i;
                break;
            }
        }
        
        if(!pg){
            cprintf("Error: Shared memory table is full. \n");
            release(&(shm_table.lock));
            return -1;
        }
    
        pg->frame = kalloc();
        memset(pg->frame, 0, PGSIZE);
    
        if(!mappages(currproc->pgdir, va, PGSIZE, V2P(pg -> frame), PTE_W | PTE_U)){
            currproc->sz = (uint)va + PGSIZE;
            *pointer = va;
            pg->id = id;
            ++(pg->refcnt);
        }
        else{
            cprintf("Error: Shared page with id %d could not be mapped. \n", id);
            release(&(shm_table.lock));
            return -1;
        }

    }
    release(&(shm_table.lock));
    return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!
    acquire(&(shm_table.lock));
    struct shm_page *pg = shm_get_page(id);
    
    if(pg == 0 || pg->refcnt == 0){
        cprintf("Error: No shared memory to clsoe. \n");
        release(&(shm_table.lock));
        return -1;
    }
    else{
        --(pg->refcnt);
    }
    if(pg->refcnt == 0){
        pg->id = 0;
        pg->frame = 0;
    }

    
    release(&(shm_table.lock));
    return 0; //added to remove compiler warning -- you should decide what to return
}
