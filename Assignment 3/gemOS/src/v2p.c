#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>

#define PAGE_SIZE 4096
#define _2MB (2 * 1024 * 1024)



enum {
	PGD_LEVEL,
	PUD_LEVEL,
	PMD_LEVEL,
	PTE_LEVEL
};



void flush_TLB() {
	u64 cr3_val;

	asm volatile (
		"mov %%cr3, %0"
		: "=r" (cr3_val)
	);

	asm volatile (
		"mov %0, %%rax\n\t"
		"mov %%rax, %%cr3"
		:
		: "r" (cr3_val)
		: "eax"
	);
}



u64 align_size(int length) {
	u64 size = ((length + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
}



u8 check_flags(u32 flag1, u32 flag2) {
	return (flag1 | 4) == (flag2 | 4);
}



void count_vm_areas(struct exec_context *ctx) {
	struct vm_area *vm = ctx->vm_area;
	u32 count = 0;

	while(vm) {
		count++;
		vm = vm->vm_next;
	}

	stats->num_vm_area = count;
}



u8 merge_vm_areas(struct exec_context *ctx) {
	struct vm_area *vm = ctx->vm_area->vm_next;
	struct vm_area *prev = ctx->vm_area;

	while(vm) {
		if(vm->vm_end > MMAP_AREA_END) {
			return -EINVAL;
		}
		if(prev->vm_start != MMAP_AREA_START) {
			if(prev->vm_end == vm->vm_start && check_flags(prev->access_flags, vm->access_flags)) {
				prev->vm_end = vm->vm_end;
				prev->vm_next = vm->vm_next;
				struct vm_area *to_delete = vm;
				vm = vm->vm_next;
				os_free(to_delete, sizeof(struct vm_area));
				continue;
			}
		}
		prev = vm;
		vm = vm->vm_next;
	}

	return 0;
}



void delete_extra_vm_areas(struct exec_context *ctx) {
	struct vm_area *vm = ctx->vm_area->vm_next;
	struct vm_area *to_delete = NULL, *prev = ctx->vm_area;

	while(vm) {
		if(vm->vm_start == vm->vm_end) {
			prev->vm_next = vm->vm_next;
			to_delete = vm;
		}
		prev = vm;
		vm = vm->vm_next;
		if(to_delete) {
			os_free(to_delete, sizeof(struct vm_area));
			to_delete = NULL;
		}
	}
}



u64 find_parent_pte(struct exec_context *current, u64 addr) {
	u64 *pgd_virtual = (u64 *)(osmap(current->pgd));

	u64 pte_offset = ((addr >> 12) & 0x1FF);
	u64 pmd_offset = ((addr >> 21) & 0x1FF);
	u64 pud_offset = ((addr >> 30) & 0x1FF);
	u64 pgd_offset = ((addr >> 39) & 0x1FF);

	u64 *pgd_t = pgd_virtual + pgd_offset;
	if (!((*pgd_t) & 1)) {
		return PGD_LEVEL;
	}

	u64 *pud_t = (u64 *)osmap(((*pgd_t) >> 12)) + pud_offset;
	if (!((*pud_t) & 1)) {
		return PUD_LEVEL;
	}

	u64 *pmd_t = (u64 *)osmap(((*pud_t) >> 12)) + pmd_offset;
	if (!((*pmd_t) & 1)) {
		return PMD_LEVEL;
	}

	u64 *pte_t = (u64 *)osmap(((*pmd_t) >> 12)) + pte_offset;
	if (!((*pte_t) & 1)) {
		return PTE_LEVEL;
	}

	u64 oneull = 1;
	u64 mask = ~(oneull << 3);
	*(pte_t) = (*pte_t) & mask;

	flush_TLB();
	
	return (*pte_t);
}



u64 *create_page_tables(struct exec_context *current, u64 addr, int permission, int flag) {
	u64 *pgd_virtual = (u64 *)(osmap(current->pgd));
	
	u64 pte_offset = ((addr >> 12) & 0x1FF);
	u64 pmd_offset = ((addr >> 21) & 0x1FF);
	u64 pud_offset = ((addr >> 30) & 0x1FF);
	u64 pgd_offset = ((addr >> 39) & 0x1FF);

	u64 write_bit = ((permission & PROT_WRITE) ? 0x8 : 0x0);

	u64 *pgd_t = pgd_virtual + pgd_offset;
	if(flag == PGD_LEVEL) {
		*pgd_t = 0x0;
		return NULL;
	}
	if ((*pgd_t) & 1) {
		if ((permission & PROT_WRITE) != 0) {
			*(pgd_t) = (*pgd_t) | 0x8;
		}
	} else {
		u64 pud_allocated = (u64)(osmap(os_pfn_alloc(OS_PT_REG)));
		*(pgd_t) = ((pud_allocated >> 12) << 12) | write_bit | 0x11;
	}

	u64 *pud_t = (u64 *)osmap(((*pgd_t) >> 12)) + pud_offset;
	if(flag == PUD_LEVEL) {
		*pud_t = 0x0;
		return NULL;
	}
	if ((*pud_t) & 1) {
		if ((permission & PROT_WRITE) != 0) {
			*(pud_t) = (*pud_t) | 0x8;
		}
	} else {
		u64 pmd_allocated = (u64)(osmap(os_pfn_alloc(OS_PT_REG)));
		*(pud_t) = ((pmd_allocated >> 12) << 12) | write_bit | 0x11;
	}

	u64 *pmd_t = (u64 *)osmap(((*pud_t) >> 12)) + pmd_offset;
	if(flag == PMD_LEVEL) {
		*pmd_t = 0x0;
		return NULL;
	}
	if ((*pmd_t) & 1) {
		if ((permission & PROT_WRITE) != 0) {
			*(pmd_t) = (*pmd_t) | 0x8;
		}
	} else {
		u64 pte_allocated = (u64)(osmap(os_pfn_alloc(OS_PT_REG)));
		*(pmd_t) = ((pte_allocated >> 12) << 12) | write_bit | 0x11;
	}

	u64 *pte_t = (u64 *)osmap(((*pmd_t) >> 12)) + pte_offset;
	if(flag == PTE_LEVEL) {
		*pte_t = 0x0;
		return NULL;
	}

	flush_TLB();

	return pte_t;
}



long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags) {
	if(!current || length < 0 || length > _2MB || !(prot == PROT_READ || prot == (PROT_READ | PROT_WRITE))) {
		return -EINVAL;
	}

	u64 size = align_size(length);

	if(addr && (addr < MMAP_AREA_START + PAGE_SIZE || addr + size >= MMAP_AREA_END)) {
		return -EINVAL;
	}

	if(!current->vm_area) {
		// printk("dummy inserted\n");
		struct vm_area *dummy_node = (struct vm_area *)os_alloc(sizeof(struct vm_area));
		dummy_node->vm_start = MMAP_AREA_START;
		dummy_node->vm_end = MMAP_AREA_START + PAGE_SIZE;
		dummy_node->access_flags = 0;
		dummy_node->vm_next = NULL;
		current->vm_area = dummy_node;
		stats->num_vm_area = 1;
	}

	struct vm_area *vm = current->vm_area;
	struct vm_area *prev = NULL;
	struct vm_area *next = NULL;
	u8 flag = 0;
	u64 start_addr;

	struct vm_area *new_vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
	new_vm_area->access_flags = prot;
	
	if(flags == MAP_FIXED) {
		// printk("MAP_FIXED\n");
		if(!addr) {
			return -EINVAL;
		}

		new_vm_area->vm_start = addr;
		new_vm_area->vm_end = addr + size;

		while(vm) {
			next = vm->vm_next;
			if((vm->vm_start <= addr && vm->vm_end > addr) || (vm->vm_start < addr + size && vm->vm_end >= addr + size)) {
				os_free(new_vm_area, sizeof(struct vm_area));
				return -EINVAL;
			} else if(vm->vm_end <= addr && (!next || next->vm_start >= addr + size)) {
				vm->vm_next = new_vm_area;
				new_vm_area->vm_next = next;
				flag = 1;
				break;
			}
			prev = vm;
			vm = next;
		}

		start_addr = addr;
	} else if(flags == 0) {
		// printk("flag 0\n");
		if(!addr) {
			// printk("Hint address not passed\n");
			while(vm) {
				next = vm->vm_next;
				if(!next) {
					if(vm->vm_end + size > MMAP_AREA_END) {
						return -EINVAL;
					}
					start_addr = vm->vm_end;
					new_vm_area->vm_start = vm->vm_end;
					new_vm_area->vm_end = vm->vm_end + size;
					new_vm_area->vm_next = NULL;
					vm->vm_next = new_vm_area;
					flag = 1;
					break;
				} else if(next->vm_start - vm->vm_end >= size) {
					start_addr = vm->vm_end;
					new_vm_area->vm_start = vm->vm_end;
					new_vm_area->vm_end = vm->vm_end + size;
					new_vm_area->vm_next = next;
					vm->vm_next = new_vm_area;
					flag = 1;
					break;
				}
				prev = vm;
				vm = next;
			}

			// printk("FLAG %d\n", flag);
		} else {
			// printk("Hint address given\n");
			while(vm) {
				next = vm->vm_next;
				if(vm->vm_end <= addr && (!next || next->vm_start >= addr + size)) {
					start_addr = addr;
					new_vm_area->vm_start = addr;
					new_vm_area->vm_end = addr + size;
					new_vm_area->vm_next = next;
					vm->vm_next = new_vm_area;
					// printk("Hint taken\n");
					flag = 1;
					break;
				}
				prev = vm;
				vm = next;
			}

			if(!flag) {
				// printk("Hint not taken\n");
				vm = current->vm_area;
				prev = next = NULL;
				while(vm) {
					next = vm->vm_next;
					if(!next) {
						if(vm->vm_end + size > MMAP_AREA_END) {
							return -EINVAL;
						}
						start_addr = vm->vm_end;
						new_vm_area->vm_start = vm->vm_end;
						new_vm_area->vm_end = vm->vm_end + size;
						new_vm_area->vm_next = NULL;
						vm->vm_next = new_vm_area;
						break;
					} else if(next->vm_start - vm->vm_end >= size) {
						start_addr = vm->vm_end;
						new_vm_area->vm_start = vm->vm_end;
						new_vm_area->vm_end = vm->vm_end + size;
						new_vm_area->vm_next = next;
						vm->vm_next = new_vm_area;
						break;
					}
					prev = vm;
					vm = next;
				}
			}
		}
	} else {
		return -EINVAL;
	}

	delete_extra_vm_areas(current);
	u8 merge = merge_vm_areas(current);
	if(merge < 0) {
		return -EINVAL;
	}

	count_vm_areas(current);

	return start_addr;
}



long vm_area_unmap(struct exec_context *current, u64 addr, int length) {
	if(!current || !current->vm_area || length < 0 || length > _2MB) {
		return -EINVAL;
	}

	u64 size = align_size(length);

	if(addr && (addr + size >= MMAP_AREA_END || addr < MMAP_AREA_START + PAGE_SIZE)) {
		return -EINVAL;
	}

	struct vm_area *vm = current->vm_area->vm_next;
	struct vm_area *prev = current->vm_area;
	struct vm_area *to_delete = NULL;

	while(vm) {
		if(vm->vm_start == addr && vm->vm_end <= addr + size) {
			to_delete = vm;
			prev->vm_next = vm->vm_next;
			os_free(to_delete, sizeof(struct vm_area));
		} else if(vm->vm_start == addr) {
			vm->vm_start = addr + size;
			prev = vm;
		} else if(vm->vm_start < addr && vm->vm_end > addr) {
			if(vm->vm_end > addr + size) {
				struct vm_area *new_vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
				new_vm_area->vm_start = addr + size;
				new_vm_area->vm_end = vm->vm_end;
				new_vm_area->vm_next = vm->vm_next;
				new_vm_area->access_flags = vm->access_flags;
				vm->vm_next = new_vm_area;
			}
			vm->vm_end = addr;
			prev = vm;
		} else if(vm->vm_start >= addr && vm->vm_end < addr + size) {
			to_delete = vm;
			prev->vm_next = vm->vm_next;
			os_free(to_delete, sizeof(struct vm_area));
		} else if(vm->vm_start < addr + size && vm->vm_end >= addr + size) {
			vm->vm_start = addr + size;
			prev = vm;
		} else {
			prev = vm;
		}

		vm = vm->vm_next;
	}

	delete_extra_vm_areas(current);
	count_vm_areas(current);

	u64 *pgd = (u64 *)osmap(current->pgd);
	for(u64 address = addr; address < addr + size; address += PAGE_SIZE) {
		u64 pgd_offset = ((address >> 39) & 0x1FF);
		u64 pud_offset = ((address >> 30) & 0x1FF);
		u64 pmd_offset = ((address >> 21) & 0x1FF);
		u64 pte_offset = ((address >> 12) & 0x1FF);

		u64 *pgd_t = pgd + pgd_offset;
		if(!((*pgd_t) & 0x1)) {
			continue;
		}

		u64 *pud = (u64 *)osmap((*pgd_t) >> 12);
		u64 *pud_t = pud + pud_offset;
		if(!((*pud_t) & 0x1)) {
			continue;
		}

		u64 *pmd = (u64 *)osmap((*pud_t) >> 12);
		u64 *pmd_t = pmd + pmd_offset;
		if(!((*pmd_t) & 0x1)) {
			continue;
		}

		u64 *pte = (u64 *)osmap((*pmd_t) >> 12);
		u64 *pte_t = pte + pte_offset;
		if((*pte_t) & 0x1) {
			u64 alloc_pte = (u64)osmap((*pte_t) >> 12);
			put_pfn((*pte_t) >> 12);
			if(get_pfn_refcount((*pte_t) >> 12) == 0) {
				os_pfn_free(USER_REG, (*pte_t) >> 12);
			}
			*pte_t = 0x0;
		} else {
			continue;
		}
	}

	flush_TLB();

	return 0;
}



long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot) {
	if(!current || !current->vm_area || length < 0 || length > _2MB || !(prot == PROT_READ || prot == (PROT_READ | PROT_WRITE))) {
		return -EINVAL;
	}

	u64 size = align_size(length);

	if(addr && (addr + size >= MMAP_AREA_END || addr < MMAP_AREA_START + PAGE_SIZE)) {
		return -EINVAL;
	}

	struct vm_area *vm = current->vm_area->vm_next;

	while(vm) {
		if(vm->vm_start == addr && vm->vm_end <= addr + size) {
			vm->access_flags = prot;
		} else if(vm->vm_start == addr) {
			if(vm->access_flags != prot) {
				struct vm_area *new_vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
				new_vm_area->vm_start = addr + size;
				new_vm_area->vm_end = vm->vm_end;
				new_vm_area->access_flags = vm->access_flags;
				new_vm_area->vm_next = vm->vm_next;
				vm->vm_end = addr + size;
				vm->access_flags = prot;
				vm->vm_next = new_vm_area;
			}
		} else if(vm->vm_start < addr && vm->vm_end > addr) {
			if(vm->access_flags != prot) {
				struct vm_area *new_vm1 = (struct vm_area *)os_alloc(sizeof(struct vm_area));
				new_vm1->vm_start = addr;
				new_vm1->vm_end = vm->vm_end;
				new_vm1->access_flags = prot;
				new_vm1->vm_next = vm->vm_next;
				vm->vm_next = new_vm1;

				if(vm->vm_end > addr + size) {
					struct vm_area *new_vm2 = (struct vm_area *)os_alloc(sizeof(struct vm_area));
					new_vm2->vm_start = addr + size;
					new_vm2->vm_end = vm->vm_end;
					new_vm2->vm_next = new_vm1->vm_next;
					new_vm2->access_flags = vm->access_flags;
					new_vm1->vm_next = new_vm2;
					new_vm1->vm_end = addr + size;
				}
				vm->vm_end = addr;
			} 
		} else if(vm->vm_start >= addr && vm->vm_end < addr + size) {
			vm->access_flags = prot;
		} else if(vm->vm_start < addr + size && vm->vm_end >= addr + size) {
			if(vm->access_flags != prot) {
				struct vm_area *new_vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
				new_vm_area->vm_start = addr + size;
				new_vm_area->vm_end = vm->vm_end;
				new_vm_area->access_flags = vm->access_flags;
				new_vm_area->vm_next = vm->vm_next;
				vm->access_flags = prot;
				vm->vm_next = new_vm_area;
				vm->vm_end = addr + size;
			}
		}

		vm = vm->vm_next;
	}

	delete_extra_vm_areas(current);
	u8 merge = merge_vm_areas(current);
	if(merge < 0) {
		return -EINVAL;
	}

	count_vm_areas(current);

	u64 *pgd = (u64 *)osmap(current->pgd);
	u8 write_bit = (prot & PROT_WRITE) ? 0x8 : 0;
	for(u64 address = addr; address < addr + size; address += PAGE_SIZE) {
		u8 flag = 0;
		vm = current->vm_area->vm_next;

		while(vm) {
			if(vm->vm_start <= address && vm->vm_end >= address + PAGE_SIZE) {
				flag = 1;
				break;
			}
			vm = vm->vm_next;
		}

		if(!flag) {
			continue;
		}

		u64 pgd_offset = ((address >> 39) & 0x1FF);
		u64 pud_offset = ((address >> 30) & 0x1FF);
		u64 pmd_offset = ((address >> 21) & 0x1FF);
		u64 pte_offset = ((address >> 12) & 0x1FF);

		u64 *pgd_t = pgd + pgd_offset;
		if(!((*pgd_t) & 0x1)) {
			// printk("pgd not valid\n");
			continue;
		} else if(vm->access_flags & PROT_WRITE) {
			*pgd_t |= write_bit;
		}

		u64 *pud = (u64 *)osmap((*pgd_t) >> 12);
		u64 *pud_t = pud + pud_offset;
		if(!((*pud_t) & 0x1)) {
			continue;
		} else if(vm->access_flags & PROT_WRITE) {
			*pud_t |= write_bit;
		}

		u64 *pmd = (u64 *)osmap((*pud_t) >> 12);
		u64 *pmd_t = pmd + pmd_offset;
		if(!((*pmd_t) & 0x1)) {
			continue;
		} else if(vm->access_flags & PROT_WRITE) {
			*pmd_t |= write_bit;
		}

		u64 *pte = (u64 *)osmap((*pmd_t) >> 12);
		u64 *pte_t = pte + pte_offset;
		if((*pte_t) & 0x1) {
			if(!(vm->access_flags & PROT_WRITE) && (*pte_t & 0x8)) {
				*pte_t ^= 0x8;
			} else {
				if(get_pfn_refcount((*pte_t) >> 12) == 1) {
					*pte_t |= write_bit;
				}
			}
		} else {
			continue;
		}
	}

	flush_TLB();

	return 0;
}



long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code) {
	if(!current || !current->vm_area) {
		return -EINVAL;
	}

	struct vm_area *vm = current->vm_area;
	u8 flag = 0;

	while(vm) {
		if(vm->vm_start <= addr && vm->vm_end > addr) {
			flag = 1;
			break;
		}

		vm = vm->vm_next;
	}

	if(!flag) {
		return -EINVAL;
	}

	if((error_code == 0x6 || error_code == 0x7) && !(vm->access_flags & PROT_WRITE)) {
		return -EINVAL;
	} else if(error_code == 0x7) {
		return handle_cow_fault(current, addr, vm->access_flags);
	}

	u64 *pgd = (u64 *)osmap(current->pgd);
	u64 pgd_offset = ((addr >> 39) & 0x1FF);
	u64 pud_offset = ((addr >> 30) & 0x1FF);
	u64 pmd_offset = ((addr >> 21) & 0x1FF);
	u64 pte_offset = ((addr >> 12) & 0x1FF);

	u64 *pgd_t = pgd + pgd_offset;
	u8 write_bit = (vm->access_flags & PROT_WRITE) ? 0x8 : 0;
	if((*pgd_t) & 0x1) {
		*pgd_t |= write_bit;
	} else {
		u64 alloc_pgd = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pgd = (u64)osmap(alloc_pgd);
		alloc_pgd = ((alloc_pgd >> 12) << 12) | 0x11 | write_bit;
		*pgd_t = alloc_pgd;
	}

	u64 *pud = (u64 *)osmap((*pgd_t) >> 12);
	u64 *pud_t = pud + pud_offset;
	if((*pud_t) & 0x1) {
		*pud_t |= write_bit;
	} else {
		u64 alloc_pud = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pud = (u64)osmap(alloc_pud);
		alloc_pud = ((alloc_pud >> 12) << 12) | 0x11 | write_bit;
		*pud_t = alloc_pud;
	}

	u64 *pmd = (u64 *)osmap((*pud_t) >> 12);
	u64 *pmd_t = pmd + pmd_offset;
	if((*pmd_t) & 0x1) {
		*pmd_t |= write_bit;
	} else {
		u64 alloc_pmd = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pmd = (u64)osmap(alloc_pmd);
		alloc_pmd = ((alloc_pmd >> 12) << 12) | 0x11 | write_bit;
		*pmd_t = alloc_pmd;
	}

	u64 *pte = (u64 *)osmap((*pmd_t) >> 12);
	u64 *pte_t = pte + pte_offset;
	if((*pte_t) & 0x1) {
		*pte_t |= write_bit;
	} else {
		u64 alloc_pte = (u64)os_pfn_alloc(USER_REG);
		alloc_pte = (u64)osmap(alloc_pte);
		alloc_pte = ((alloc_pte >> 12) << 12) | 0x11 | write_bit;
		*pte_t = alloc_pte;
	}

	flush_TLB();

	return 1;
}



long do_cfork() {
	u32 pid;
	struct exec_context *new_ctx = get_new_ctx();
	struct exec_context *ctx = get_current_ctx();
	
	pid = new_ctx->pid;
	new_ctx->ppid = ctx->pid;
	new_ctx->type = ctx->type;
	new_ctx->state = ctx->state;
	new_ctx->used_mem = ctx->used_mem;
	new_ctx->pgd = (u32)os_pfn_alloc(OS_PT_REG);
	
	u64 start, end;
	for(int seg = 0; seg < MAX_MM_SEGS; seg++) {
		new_ctx->mms[seg] = ctx->mms[seg];

		start = new_ctx->mms[seg].start;
		if (seg == MM_SEG_STACK) {
			end = new_ctx->mms[seg].end;
		} else {
			end = new_ctx->mms[seg].next_free;
		}

		for (; start < end; start += PAGE_SIZE) {
			u64 pfn_virtual_old = find_parent_pte(ctx, start);
			u64 *pte_loc_new_table = create_page_tables(new_ctx, start, new_ctx->mms[seg].access_flags, pfn_virtual_old);
			if (pte_loc_new_table) {
				*(pte_loc_new_table) = pfn_virtual_old;
				get_pfn(pfn_virtual_old >> 12);
			}
		}
	}

	if(ctx->vm_area) {
		new_ctx->vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
		new_ctx->vm_area->vm_start = MMAP_AREA_START;
		new_ctx->vm_area->vm_end = MMAP_AREA_START + PAGE_SIZE;
		new_ctx->vm_area->access_flags = 0x0;
		new_ctx->vm_area->vm_next = NULL;

		struct vm_area *child_vm = new_ctx->vm_area;
		struct vm_area *parent_vm = ctx->vm_area->vm_next;
		while(parent_vm) {
			struct vm_area *new_vm_area = (struct vm_area *)os_alloc(sizeof(struct vm_area));
			new_vm_area->vm_start = parent_vm->vm_start;
			new_vm_area->vm_end = parent_vm->vm_end;
			new_vm_area->access_flags = parent_vm->access_flags;
			new_vm_area->vm_next = NULL;
			child_vm->vm_next = new_vm_area;

			start = parent_vm->vm_start;
			end = parent_vm->vm_end;
			for (; start < end; start += PAGE_SIZE) {
				u64 pfn_virtual_old = find_parent_pte(ctx, start);
				u64 *pte_loc_new_table = create_page_tables(new_ctx, start, parent_vm->access_flags, pfn_virtual_old);
				if (pte_loc_new_table) {
					*(pte_loc_new_table) = pfn_virtual_old;
					get_pfn(pfn_virtual_old >> 12);
				}
			}

			child_vm = child_vm->vm_next;
			parent_vm = parent_vm->vm_next;
		}
	}

	for(int i = 0; i < CNAME_MAX; i++) {
		new_ctx->name[i] = ctx->name[i];
	}

	new_ctx->regs = ctx->regs;
	new_ctx->pending_signal_bitmap = ctx->pending_signal_bitmap;

	for(int i = 0; i < MAX_SIGNALS; i++) {
		new_ctx->sighandlers[i] = ctx->sighandlers[i];
	}

	new_ctx->ticks_to_sleep = ctx->ticks_to_sleep;
	new_ctx->alarm_config_time = ctx->alarm_config_time;
	new_ctx->ticks_to_alarm = ctx->ticks_to_alarm;

	for(int i = 0; i < MAX_OPEN_FILES; i++){
		new_ctx->files[i] = ctx->files[i];
	}

	copy_os_pts(ctx->pgd, new_ctx->pgd);
	do_file_fork(new_ctx);
	setup_child_context(new_ctx);

	return pid;
}



long handle_cow_fault(struct exec_context *current, u64 vaddr, int access_flags) {

	if(!current || vaddr < 0) {
		return -EINVAL;
	}

	u64 *pgd = (u64 *)osmap(current->pgd);
	u64 pgd_offset = ((vaddr >> 39) & 0x1FF);
	u64 pud_offset = ((vaddr >> 30) & 0x1FF);
	u64 pmd_offset = ((vaddr >> 21) & 0x1FF);
	u64 pte_offset = ((vaddr >> 12) & 0x1FF);

	u64 *pgd_t = pgd + pgd_offset;
	u8 write_bit = (access_flags & PROT_WRITE) ? 0x8 : 0;
	if(((*pgd_t) & 0x1) == 0) {
		u64 alloc_pgd = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pgd = (u64)osmap(alloc_pgd);
		alloc_pgd = ((alloc_pgd >> 12) << 12) | 0x11 | write_bit;
		*pgd_t = alloc_pgd;
	} else {
		*pgd_t |= write_bit;
	}

	u64 *pud = (u64 *)osmap((*pgd_t) >> 12);
	u64 *pud_t = pud + pud_offset;
	if(((*pud_t) & 0x1) == 0) {
		u64 alloc_pud = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pud = (u64)osmap(alloc_pud);
		alloc_pud = ((alloc_pud >> 12) << 12) | 0x11 | write_bit;
		*pud_t = alloc_pud;
	} else {
		*pud_t |= write_bit;
	}

	u64 *pmd = (u64 *)osmap((*pud_t) >> 12);
	u64 *pmd_t = pmd + pmd_offset;
	if(((*pmd_t) & 0x1) == 0) {
		u64 alloc_pmd = (u64)os_pfn_alloc(OS_PT_REG);
		alloc_pmd = (u64)osmap(alloc_pmd);
		alloc_pmd = ((alloc_pmd >> 12) << 12) | 0x11 | write_bit;
		*pmd_t = alloc_pmd;
	} else {
		*pmd_t |= write_bit;
	}

	u64 *pte = (u64 *)osmap((*pmd_t) >> 12);
	u64 *pte_t = pte + pte_offset;
	if((*pte_t) & 0x1) {
		if(get_pfn_refcount((*pte_t) >> 12) == 1) {
			*pte_t |= 0x8;
		} else if(get_pfn_refcount((*pte_t) >> 12) > 1) {
			u64 shared_pte = *pte_t;
			put_pfn((*pte_t) >> 12);
			u64 alloc_pte = (u64)os_pfn_alloc(USER_REG);
			alloc_pte = (u64)osmap(alloc_pte);
			alloc_pte = ((alloc_pte >> 12) << 12) | 0x19;
			*pte_t = alloc_pte;
			memcpy(osmap(alloc_pte >> 12), osmap(shared_pte >> 12), PAGE_SIZE);
		}
	} else {
		u64 alloc_pte = (u64)os_pfn_alloc(USER_REG);
		alloc_pte = (u64)osmap(alloc_pte);
		alloc_pte = ((alloc_pte >> 12) << 12) | 0x19;
		*pte_t = alloc_pte;
	}

	flush_TLB();
	
	return 1;
}
