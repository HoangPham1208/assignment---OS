//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*init pthread*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  // check if the new region is valid
  if (rg_elmt.rg_start >= rg_elmt.rg_end)
    return -1;
  pthread_mutex_lock(&mutex);
  // malloc the new region
  struct vm_rg_struct *rg = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
  rg->rg_start = rg_elmt.rg_start;
  rg->rg_end = rg_elmt.rg_end;
  rg->rg_next = NULL;
  // check if the freerg_list is empty
  if (rg_node != NULL) // pushing to the first element
  {
    rg->rg_next = rg_node;
  }

  /* Enlist the new region */
  // update the new region
  mm->mmap->vm_freerg_list = rg;
  pthread_mutex_unlock(&mutex);

#ifdef VMDBG
    printf("----------------------------------------------\n");
    printf("Detect area size: \n");
    printf("Start = %lu, End = %lu\n",mm->mmap->vm_start, mm->mmap->vm_end);
    printf("The enlist has been called\n");
    struct vm_rg_struct *it = mm->mmap->vm_freerg_list;
    while (it != NULL)
    {
      printf("range: start = %lu, end = %lu\n", it->rg_start, it->rg_end);
      it = it->rg_next;
    }
    printf("----------------------------------------------\n");

#endif

  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;
    #ifdef VMDBG 
    printf("----------------------------------------------\n");
    printf("Alloc has been called when process can get free region\n");
    printf("Region range: start = %lu, end = %lu\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
    printf("VM area range: start = %lu, end = %lu\n", caller->mm->mmap->vm_start, caller->mm->mmap->vm_end);
    // Checking the free region list
    printf("Free region list: \n");
    struct vm_rg_struct *it = caller->mm->mmap->vm_freerg_list;
    while (it != NULL)
    {
      printf("Region range: start = %lu, end = %lu\n", it->rg_start, it->rg_end);
      it = it->rg_next;
    }
    printf("----------------------------------------------\n");
    #endif
    #ifdef TEST
    printf("-------------------------\n");
    printf("ALLOC - size = %d\n", size);
    printf("-------------------------\n");
    printf("\n");
    // print the vma list
    print_list_vma(caller->mm->mmap);
    // print the region are being used
    printf("Regions are being used: \n");
    for(int it = 0 ; it < PAGING_MAX_SYMTBL_SZ; it++){
      if(caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0){
        continue;
      }
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
    }
    printf("\n");
    // print the free region list
    printf("Free region list: \n");
    print_list_rg(caller->mm->mmap->vm_freerg_list);
    printf("-------------------------\n");
    #endif
    return 0;
  }
  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
  if( caller->mm->mmap->sbrk + size < caller->mm->mmap->vm_end){
    caller->mm->symrgtbl[rgid].rg_start = caller->mm->mmap->sbrk;
    caller->mm->symrgtbl[rgid].rg_end = caller->mm->mmap->sbrk + size;
    caller->mm->mmap->sbrk += size;
    *alloc_addr = caller->mm->symrgtbl[rgid].rg_start;
    #ifdef VMDBG 
    printf("----------------------------------------------\n");
    printf("Alloc has been called when process can lift up sbrk\n");
    printf("Region range: start = %lu, end = %lu\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
    printf("VM area range: start = %lu, end = %lu\n", caller->mm->mmap->vm_start, caller->mm->mmap->vm_end);
    // Checking the free region list
    printf("Free region list: \n");
    struct vm_rg_struct *it = caller->mm->mmap->vm_freerg_list;
    while (it != NULL)
    {
      printf("Region range: start = %lu, end = %lu\n", it->rg_start, it->rg_end);
      it = it->rg_next;
    }
    printf("----------------------------------------------\n");
    #endif
    #ifdef TEST
    printf("-------------------------\n");
    printf("ALLOC - size = %d\n", size);
    printf("-------------------------\n");
    printf("\n");
    // print the vma list
    print_list_vma(caller->mm->mmap);
    // print the region are being used
    printf("Regions are being used:\n");
    for(int it = 0 ; it < PAGING_MAX_SYMTBL_SZ; it++){
      if(caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0){
        continue;
      }
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
    }
    printf("\n");
    // print the free region list
    printf("Free region list: \n");
    print_list_rg(caller->mm->mmap->vm_freerg_list);
    printf("-------------------------\n");
    #endif
    return 0;
  }

  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  //int inc_limit_ret
  int old_sbrk ;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  inc_vma_limit(caller, vmaid, inc_sz);

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
  caller->mm->mmap->sbrk = old_sbrk + size;
  *alloc_addr = old_sbrk;

  // Handle the last free region in the vma
  // Case 1: last_free_rg is NULL mean that first allocated region
  // if(cur_vma->vm_freerg_list == NULL){
  //   cur_vma->vm_freerg_list = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
  //   cur_vma->vm_freerg_list->rg_start = caller->mm->symrgtbl[rgid].rg_end;
  //   cur_vma->vm_freerg_list->rg_end = cur_vma->sbrk ;
  //   cur_vma->vm_freerg_list->rg_next = NULL;
  // }
  // // Case 2: it happen when the remain has been not 
  // else{
  //   struct vm_rg_struct *temp = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
  //   temp->rg_start = caller->mm->symrgtbl[rgid].rg_end;
  //   temp->rg_end = cur_vma->sbrk;
  //   temp->rg_next = cur_vma->vm_freerg_list;
  //   cur_vma->vm_freerg_list = temp;
  // }
  #ifdef VMDBG
    printf("----------------------------------------------\n");

  printf("Alloc has been called when process must increase area limit\n");
  printf("Region range: start = %lu, end = %lu\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
  printf("VM area range: start = %lu, end = %lu\n", cur_vma->vm_start, cur_vma->vm_end);
  // Checking the free region list
  printf("Free region list: \n");
  struct vm_rg_struct *it = cur_vma->vm_freerg_list;
  while (it != NULL)
  {
    printf("Region range: start = %lu, end = %lu\n", it->rg_start, it->rg_end);
    it = it->rg_next;
  }
    printf("----------------------------------------------\n");
  
  #endif
  #ifdef TEST
    printf("-------------------------\n");
    printf("ALLOC - size = %d\n", size);
    printf("-------------------------\n");
    printf("\n");
    // print the vma list
    print_list_vma(caller->mm->mmap);
    // print the region are being used
    printf("Regions are being used:\n");
    for(int it = 0 ; it < PAGING_MAX_SYMTBL_SZ; it++){
      if(caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0){
        continue;
      }
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
    }
    printf("\n");
    // print the free region list
    printf("List of free region:\n");
    print_list_rg(caller->mm->mmap->vm_freerg_list);
    printf("-------------------------\n");
    #endif

  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  struct vm_rg_struct rgnode;
  #ifdef VMDBG
  printf("free is called\n");
  #endif
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ){
    return -1;
  }
  /* TODO: Manage the collect freed region to freerg_list */
  rgnode.rg_start = caller->mm->symrgtbl[rgid].rg_start;
  rgnode.rg_end = caller->mm->symrgtbl[rgid].rg_end;
  
  // reset the symrgtbl entry to 0 
  caller->mm->symrgtbl[rgid].rg_start = 0;
  caller->mm->symrgtbl[rgid].rg_end = 0;
  
  /*enlist the obsoleted memory region */
  
  enlist_vm_freerg_list(caller->mm, rgnode);
  #ifdef TEST
    printf("-------------------------\n");
    printf("FREE - Region id %d:[%lu,%lu]\n", rgid, rgnode.rg_start, rgnode.rg_end);
    printf("-------------------------\n");
    printf("\n");
    // print the vma list
    print_list_vma(caller->mm->mmap);
    // print the region are being used
    printf("List of region are being used:\n");
    for(int it = 0 ; it < PAGING_MAX_SYMTBL_SZ; it++){
      if(caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0){
        continue;
      }
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
    }
    printf("\n");
    // print the free region list
    printf("Free region list:\n");
    print_list_rg(caller->mm->mmap->vm_freerg_list);
    printf("-------------------------\n");
  #endif
  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
   return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn; 
    int vicfpn;
    uint32_t vicpte;

    int tgtfpn = PAGING_SWP(pte);//the target frame storing our variable

    /* TODO: Play with your paging theory here */
    /* Find victim page */
    find_victim_page(caller->mm, &vicpgn);
    #ifdef VMDBG
    printf("this is vicpgn: %d\n", vicpgn);
    #endif
    /* Get victim frame from victim page*/
    vicpte = caller->mm->pgd[vicpgn];
    vicfpn = PAGING_FPN(vicpte);
    /* Get free frame in MEMSWP */
    MEMPHY_get_freefp(caller->active_mswp, &swpfpn);

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
    /* Update page table */
    //pte_set_swap() &mm->pgd;  
    pte_set_swap(&vicpte,0,swpfpn);
    /* Update its online status of the target page */
    //pte_set_fpn() & mm->pgd[pgn];
    // target has been swapped to memram
    pte_set_fpn(&pte,vicfpn);

    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }

  *fpn = PAGING_FPN(pte);
  #ifdef VMDBG
  printf("This is FPN: %d\n", *fpn);
  #endif

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;
  #ifdef VMDBG
  printf("This is page number: %d\n", pgn);
  #endif
  /* Get the page` to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  #ifdef TEST
  printf("----------------------------------------\n");
  printf("READ CONTENT\n");
  printf("----------------------------------------\n");
  printf("VM address = %d, fpn = %d, phyaddr = %d\n", addr,fpn, phyaddr);
  #endif

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  #ifdef TEST
  printf("----------------------------------------\n");
  printf("WRITE CONTENT\n");
  printf("----------------------------------------\n");
  printf("VM addr = %d, fpn = %d, phyaddr = %d\n", addr, fpn, phyaddr);
  #endif

  MEMPHY_write(caller->mram,phyaddr, value);

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);
  
  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;

  int val = __read(proc, 0, source, offset, &data);
  #ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
#endif
  __write(proc, 0, destination, 0, data);

#ifdef TEST
  printf("------------------\n");
  printf("AFTER READ\n");
  printf("------------------\n");
  printf("List of used frame:\n");
  print_list_fp(proc->mram->used_fp_list);
#endif
  MEMPHY_dump(proc->mram);

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
#ifdef TEST
  printf("------------------\n");
  printf("AFTER WRITE\n");
  printf("------------------\n");
  printf("used_fp_list:\n");
  print_list_fp(proc->mram->used_fp_list);
#endif
  MEMPHY_dump(proc->mram);
#endif
  return val;
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  while(vma != NULL)
  {
    if(vmaid != vma->vm_id) /* skip current vma */
    {
      vma = vma->vm_next;
      continue;
    }
    if(vma->vm_start <= vmastart || vma->vm_end >= vmaend)
      return -1;

    vma = vma->vm_next;
  }

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only) 
   * now will be alloc real ram region */
  // check if start = end
  if(cur_vma->vm_start == cur_vma->vm_end)
  {
    cur_vma->vm_freerg_list = NULL;
  }
  cur_vma->vm_end += inc_sz;
  if (vm_map_ram(caller, cur_vma->vm_start, cur_vma->vm_end, 
                    old_end, incnumpage , area) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;

}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theorical mechanism to find the victim page */
  // using FIFO algorithm
  if(pg == NULL) {
    // no page in fifo list
    // so retpgn must be 0
    *retpgn = 0;
    return 0;
  }
  while(pg->pg_next != NULL) pg = pg->pg_next;
  *retpgn = pg->pgn;
  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        { /*End of free list */
          rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    }
    else
    {
      rgit = rgit->rg_next;	// Traverse next rg
    }
  }

 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

//#endif
