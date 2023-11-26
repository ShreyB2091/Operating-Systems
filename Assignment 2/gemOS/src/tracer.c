#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>



///////////////////////////////////////////////////////////////////////////
//// 		             Start of Trace buffer functionality 	    	      /////
///////////////////////////////////////////////////////////////////////////



int is_valid_mem_range(unsigned long buff, u32 count, int access_bit)  {
	u32 access_flag = 1 << access_bit;
	struct exec_context *ctx = get_current_ctx();

	if(buff >= ctx->mms[MM_SEG_CODE].start && buff + count <= ctx->mms[MM_SEG_CODE].next_free) {
		return (access_flag == 1);
	} else if(buff >= ctx->mms[MM_SEG_RODATA].start & buff + count <= ctx->mms[MM_SEG_RODATA].next_free) {
		return (access_flag == 1);
	} else if(buff >= ctx->mms[MM_SEG_DATA].start && buff + count <= ctx->mms[MM_SEG_DATA].next_free) {
		return (access_flag == 1 || access_flag == 2);
	} else if(buff >= ctx->mms[MM_SEG_STACK].start && buff + count <= ctx->mms[MM_SEG_STACK].end) {
		return (access_flag == 1 || access_flag == 2);
	} else {
		struct vm_area *vm = ctx->vm_area;
		u8 flag = 0;
		while(vm) {
			if(buff >= vm->vm_start && buff + count <= vm->vm_end) {
				flag = 1;
				break;
			}
			vm = vm->vm_next;
		}

		if(flag) {
			u32 ret_val = vm->access_flags & access_flag;
			return (ret_val != 0);
		}
	}

	return 0;
}



long trace_buffer_close(struct file *filep) {
	if(filep->type == REGULAR) {
		return -EINVAL;
	}

	struct exec_context *ctx = get_current_ctx();
	u32 fd = 0;
	while(fd < MAX_OPEN_FILES) {
		if(ctx->files[fd] == filep) {
			break;
		} else {
			fd++;
		}
	}
	
	os_page_free(USER_REG, filep->trace_buffer->buffer);
	os_free(filep->fops, sizeof(struct fileops *));
	os_free(filep->trace_buffer, sizeof(struct trace_buffer_info *));
	os_page_free(USER_REG, filep);

	ctx->files[fd] = NULL;

	return 0;	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count) {
	if((filep->mode != O_READ && filep->mode != O_RDWR) || filep->type == REGULAR) {
		return -EINVAL;
	}

	if(!is_valid_mem_range((unsigned long) buff, count, 1)) {
		return -EBADMEM;
	}

	u32 bytes_read = 0;
	u32 read_position = filep->trace_buffer->readEnd;
	u32 write_position = filep->trace_buffer->writeEnd;
	u32 storage_left = 0;
	if(write_position > read_position) {
		storage_left = write_position - read_position;
	} else if(write_position < read_position) {
		storage_left = TRACE_BUFFER_MAX_SIZE - read_position + write_position;
	} else {
		storage_left = (filep->trace_buffer->isFull ? TRACE_BUFFER_MAX_SIZE : 0);
	}

	bytes_read = storage_left > count ? count : storage_left;
	if(filep->trace_buffer->isFull == 0 && read_position == write_position) {
		return 0;
	}

	for(int i = 0; i < bytes_read; i++) {
		buff[i] = filep->trace_buffer->buffer[(read_position + i) % TRACE_BUFFER_MAX_SIZE];
	}
	filep->trace_buffer->readEnd = (read_position + bytes_read) % TRACE_BUFFER_MAX_SIZE;
	if(bytes_read > 0) {
		filep->trace_buffer->isFull = 0;
	}

	filep->offp = filep->trace_buffer->readEnd;

	return bytes_read;
}



int trace_buffer_write(struct file *filep, char *buff, u32 count) {
	if(!filep || (filep->mode != O_WRITE && filep->mode != O_RDWR) || filep->type == REGULAR) {
		return -EINVAL;
	}

	if(!is_valid_mem_range((unsigned long) buff, count, 0)) {
		return -EBADMEM;
	}

	if(filep->trace_buffer->isFull) {
		return 0;
	}

	u32 bytes_written = 0;
	u32 read_position = filep->trace_buffer->readEnd;
	u32 write_position = filep->trace_buffer->writeEnd;
	u32 storage_left = 0;
	if(read_position > write_position) {
		storage_left = read_position - write_position;
	} else if(read_position < write_position) {
		storage_left = TRACE_BUFFER_MAX_SIZE - write_position + read_position;
	} else {
		storage_left = (filep->trace_buffer->isFull ? 0 : TRACE_BUFFER_MAX_SIZE);
	}

	bytes_written = storage_left > count ? count : storage_left;
	for(int i = 0; i < bytes_written; i++) {
		filep->trace_buffer->buffer[(write_position + i) % TRACE_BUFFER_MAX_SIZE] = buff[i];
	}
	filep->trace_buffer->writeEnd = (write_position + bytes_written) % TRACE_BUFFER_MAX_SIZE;

	if(bytes_written > 0 && filep->trace_buffer->writeEnd == read_position) {
		filep->trace_buffer->isFull = 1;
	}

	filep->offp = filep->trace_buffer->writeEnd;

	return bytes_written;
}



int sys_create_trace_buffer(struct exec_context *current, int mode) {
	if(mode < 1 || mode > 3) {
		return -EINVAL;
	}

	u32 fd = 0;
	while(fd < MAX_OPEN_FILES) {
		if(current->files[fd] == NULL) {
			break;
		} else {
			fd++;
		}
	}

	if(fd == MAX_OPEN_FILES) {
		return -EINVAL;
	}
	struct file *new_trace = (struct file *)os_page_alloc(USER_REG);
	if(new_trace == NULL) {
		return -ENOMEM;
	}
	new_trace->type = TRACE_BUFFER;
	new_trace->mode = mode;
	new_trace->offp = 0;
	new_trace->ref_count = 1;
	new_trace->inode = NULL;
	
	struct trace_buffer_info *buffer_object = (struct trace_buffer_info *)os_alloc(sizeof(struct trace_buffer_info));
	if(buffer_object == NULL) {
		return -ENOMEM;
	}
	buffer_object->isFull = 0;
	buffer_object->readEnd = 0;
	buffer_object->writeEnd = 0;
	buffer_object->buffer = (char *)os_page_alloc(USER_REG);

	new_trace->trace_buffer = buffer_object;

	struct fileops *fops = (struct fileops *)os_alloc(sizeof(struct fileops));
	if(fops == NULL) {
		return -ENOMEM;
	}
	fops->read = trace_buffer_read;
	fops->write = trace_buffer_write;
	fops->close = trace_buffer_close;
	fops->lseek = NULL;

	new_trace->fops = fops;

	current->files[fd] = new_trace;
	return fd;
}



///////////////////////////////////////////////////////////////////////////
//// 		              Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////



int get_arg_count(int syscall_num) {
	u32 arg_count;

	if( syscall_num == SYSCALL_GETPID || 
	syscall_num == SYSCALL_GETPPID || 
	syscall_num == SYSCALL_FORK || 
	syscall_num == SYSCALL_CFORK || 
	syscall_num == SYSCALL_VFORK || 
	syscall_num == SYSCALL_PHYS_INFO || 
	syscall_num == SYSCALL_STATS || 
	syscall_num == SYSCALL_GET_USER_P || 
	syscall_num == SYSCALL_GET_COW_F || 
	syscall_num == SYSCALL_END_STRACE) {
		arg_count = 0;
	} else if(syscall_num == SYSCALL_EXIT || 
	syscall_num == SYSCALL_CONFIGURE || 
	syscall_num == SYSCALL_DUMP_PTT || 
	syscall_num == SYSCALL_SLEEP || 
	syscall_num == SYSCALL_PMAP || 
	syscall_num == SYSCALL_DUP || 
	syscall_num == SYSCALL_CLOSE || 
	syscall_num == SYSCALL_TRACE_BUFFER) {
		arg_count = 1;
	} else if(syscall_num == SYSCALL_SIGNAL || 
	syscall_num == SYSCALL_EXPAND || 
	syscall_num == SYSCALL_CLONE || 
	syscall_num == SYSCALL_MUNMAP || 
	syscall_num == SYSCALL_OPEN || 
	syscall_num == SYSCALL_DUP2 || 
	syscall_num == SYSCALL_START_STRACE || 
	syscall_num == SYSCALL_STRACE) {
		arg_count = 2;
	} else if(syscall_num == SYSCALL_MPROTECT || 
	syscall_num == SYSCALL_READ || 
	syscall_num == SYSCALL_WRITE || 
	syscall_num == SYSCALL_LSEEK || 
	syscall_num == SYSCALL_READ_STRACE || 
	syscall_num == SYSCALL_READ_FTRACE) {
		arg_count = 3;
	} else if(syscall_num == SYSCALL_MMAP ||
	syscall_num == SYSCALL_FTRACE) {
		arg_count = 4;
	}

	return arg_count;
}



void write_to_buffer(struct file *filep, char *buff, u32 count) {
	u32 bytes_written = 0;
	u32 read_position = filep->trace_buffer->readEnd;
	u32 write_position = filep->trace_buffer->writeEnd;
	u32 storage_left = 0;
	if(read_position > write_position) {
		storage_left = read_position - write_position;
	} else {
		storage_left = TRACE_BUFFER_MAX_SIZE - write_position + read_position;
	}

	bytes_written = storage_left > count ? count : storage_left;
	for(int i = 0; i < bytes_written; i++) {
		filep->trace_buffer->buffer[(write_position + i) % TRACE_BUFFER_MAX_SIZE] = buff[i];
	}

	u64 *buf = (u64 *)filep->trace_buffer->buffer;

	filep->trace_buffer->writeEnd = (write_position + bytes_written) % TRACE_BUFFER_MAX_SIZE;
	if(filep->trace_buffer->writeEnd == read_position) {
		filep->trace_buffer->isFull = 1;
	}

	filep->offp = filep->trace_buffer->writeEnd;
}



int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4) {
	if(syscall_num == 1) {
		return 0;
	}

	u32 arg_count = get_arg_count(syscall_num);

	struct exec_context *ctx = get_current_ctx();
	if(!ctx) {
		return 0;
	}

	struct strace_head *trace_head = ctx->st_md_base;
	if(!trace_head) {
		return 0;
	}

	u8 flag = 0;
	if(trace_head->tracing_mode == FULL_TRACING) {
		flag = 1;
	} else {
		struct strace_info *head = trace_head->next;
		while(head) {
			if(head->syscall_num == syscall_num) {
				flag = 1;
				break;
			}
			head = head->next;
		}
	}

	if(!flag || syscall_num == SYSCALL_START_STRACE || syscall_num == SYSCALL_END_STRACE || trace_head->is_traced == 0) {
		return 0;
	}

	int strace_fd = trace_head->strace_fd;
	struct file* filep = ctx->files[strace_fd];
	u64 *buff = (u64 *)os_alloc((arg_count + 1) * sizeof(u64));
	if(buff == NULL) {
		return -EINVAL;
	}

	buff[0] = syscall_num;
	if(arg_count >= 1) {
		buff[1] = param1;
	}
	if(arg_count >= 2) {
		buff[2] = param2;
	}
	if(arg_count >= 3) {
		buff[3] = param3;
	}
	if(arg_count >= 4) {
		buff[4] = param4;
	}

	write_to_buffer(filep, (char *)buff, (arg_count + 1) * sizeof(u64));
	return 0;
}



int sys_strace(struct exec_context *current, int syscall_num, int action) {
	if(!current->st_md_base) {
		current->st_md_base = (struct strace_head *)os_alloc(sizeof(struct strace_head));
		if(current->st_md_base == NULL) {
			return -EINVAL;
		}
		current->st_md_base->count = 0;
		current->st_md_base->next = current->st_md_base->last = NULL;
	}
	struct strace_head *trace_head = current->st_md_base;

	struct strace_info *head = trace_head->next;
	struct strace_info *tmp = head;

	if(action == ADD_STRACE) {
		if(current->st_md_base->count == STRACE_MAX) {
			return -EINVAL;
		}
		struct strace_info *new_syscall = (struct strace_info *)os_alloc(sizeof(struct strace_info));
		if(new_syscall == NULL) {
			return -EINVAL;
		}
		new_syscall->syscall_num = syscall_num;
		new_syscall->next = NULL;

		u8 flag = 0;
		if(!head) {
			current->st_md_base->next = new_syscall;
			current->st_md_base->last = new_syscall;
			current->st_md_base->count = 1;
			return 0;
		}
		while(head) {
			if(head->syscall_num == syscall_num) {
				flag = 1;
				break;
			}
			head = head->next;
		}
		if(!flag) {
			current->st_md_base->last->next = new_syscall;
			current->st_md_base->last = new_syscall;
			current->st_md_base->count++;
		} else {
			return -EINVAL;
		}
	} else if(action == REMOVE_STRACE) {
		u8 flag = 0;
		struct strace_info *prev = NULL;
		while(head && head->next) {
			if(head->syscall_num == syscall_num) {
				flag = 1;
				break;
			}
			prev = head;
			head = head->next;
		}
		if(!flag) {
			return -EINVAL;
		} else {
			if(!prev) {
				struct strace_info *to_delete = tmp;
				current->st_md_base->next = tmp->next;
				os_free(to_delete, sizeof(struct strace_info *));
			} else {
				struct strace_info *to_delete = head;
				prev->next = head->next;
				os_free(to_delete, sizeof(struct strace_info *));
			}
		}
		current->st_md_base->count--;
	} else {
		return -EINVAL;
	}

	return 0;
}



int sys_read_strace(struct file *filep, char *buff, u64 count) {
	u32 bytes_read = 0;
	u32 buff_index = 0;
	for(int i = 0; i < count; i++) {
		struct trace_buffer_info *buffer_info = filep->trace_buffer;
		u32 arg_count = get_arg_count(buffer_info->buffer[buffer_info->readEnd]);
		u32 bytes = trace_buffer_read(filep, buff + buff_index, (arg_count + 1) * sizeof(u64));
		if(bytes > 0) {
			bytes_read += bytes;
		}
		buff_index += bytes;
	}
	return bytes_read;
}



int sys_start_strace(struct exec_context *current, int fd, int tracing_mode) {
	if(!current->st_md_base) {
		current->st_md_base = (struct strace_head *)os_alloc(sizeof(struct strace_head));
		if(current->st_md_base == NULL) {
			return -EINVAL;
		}

		struct strace_head *trace_head = current->st_md_base;
		
		trace_head->next = trace_head->last = NULL;
		trace_head->count = 0;
		trace_head->is_traced = 1;
		trace_head->tracing_mode = tracing_mode;
		trace_head->strace_fd = fd;
	} else {
		struct strace_head *trace_head = current->st_md_base;
		trace_head->is_traced = 1;
		trace_head->tracing_mode = tracing_mode;
		trace_head->strace_fd = fd;
	}

	return 0;
}



int sys_end_strace(struct exec_context *current) {
	struct strace_head *trace_head = current->st_md_base;
	if(!trace_head) {
		return 0;
	}

	struct strace_info *head = trace_head->next;
	while(head) {
		struct strace_info *to_delete = head;
		head = head->next;
		os_free(to_delete, sizeof(struct strace_info));
	}

	trace_head->count = 0;
	trace_head->is_traced = 0;
	current->st_md_base->next = NULL;
	current->st_md_base->last = NULL;

	return 0;
}



///////////////////////////////////////////////////////////////////////////
//// 		              Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////



long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
	if(!ctx) {
		return -EINVAL;
	}

	if(!ctx->ft_md_base) {
		ctx->ft_md_base = (struct ftrace_head *)os_alloc(sizeof(struct ftrace_head));
		if(ctx->ft_md_base == NULL) {
			return -EINVAL;
		}
		ctx->ft_md_base->count = 0;
		ctx->ft_md_base->next = NULL;
		ctx->ft_md_base->last = NULL;
	}
	struct ftrace_head *trace_head = ctx->ft_md_base;
	struct ftrace_info *head = ctx->ft_md_base->next;
	struct ftrace_info *tmp = head;

	if(action == ADD_FTRACE) {
		if(trace_head->count == FTRACE_MAX) {
			return -EINVAL;
		}

		u8 flag = 0;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			head = head->next;
		}

		if(flag) {
			return -EINVAL;
		} else {
			struct ftrace_info *new_trace = (struct ftrace_info *)os_alloc(sizeof(struct ftrace_info));
			if(new_trace == NULL) {
				return -EINVAL;
			}
			new_trace->faddr = faddr;
			new_trace->num_args = nargs;
			new_trace->fd = fd_trace_buffer;
			new_trace->capture_backtrace = 0;
			new_trace->next = NULL;
			if(!trace_head->next) {
				trace_head->next = new_trace;
				trace_head->last = new_trace;
				trace_head->count = 1;
			} else {
				trace_head->last->next = new_trace;
				trace_head->last = new_trace;
				ctx->ft_md_base->count++;
			}
		}
	} else if (action == REMOVE_FTRACE) {
		u8 flag = 0;
		struct ftrace_info *prev = NULL;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			prev = head;
			head = head->next;
		}

		if(!flag) {
			return -EINVAL;
		} else {
			if(!prev) {
				struct ftrace_info *to_delete = tmp;
				ctx->ft_md_base->next = tmp->next;
				if(to_delete->code_backup[0] != -1) {
					u8 *ptr = (u8 *)to_delete->faddr;
					for(int i = 0; i < 4; i++) {
						ptr[i] = to_delete->code_backup[i];
					}
				}
				os_free(to_delete, sizeof(struct ftrace_info));
			} else {
				struct ftrace_info *to_delete = head;
				prev->next = head->next;
				if(to_delete->code_backup[0] != -1) {
					u8 *ptr = (u8 *)to_delete->faddr;
					for(int i = 0; i < 4; i++) {
						ptr[i] = to_delete->code_backup[i];
					}
				}
				os_free(to_delete, sizeof(struct ftrace_info));
			}
		}
		ctx->ft_md_base->count--;
	} else if (action == ENABLE_FTRACE) {
		u8 flag = 0;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			head = head->next;
		}

		u8 *ptr = (u8 *)faddr;
		if(!flag) {
			return -EINVAL;
		} else if(*ptr != INV_OPCODE) {
			for(int i = 0; i < 4; i++) {
				head->code_backup[i] = ptr[i];
				ptr[i] = INV_OPCODE;
			}
		}
	} else if(action == DISABLE_FTRACE) {
		u8 flag = 0;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			head = head->next;
		}

		u8 *ptr = (u8 *)faddr;
		if(!flag) {
			return -EINVAL;
		} else if(*ptr == INV_OPCODE) {
			u8 *ptr = (u8 *)head->faddr;
			for(int i = 0; i < 4; i++) {
				ptr[i] = head->code_backup[i];
			}
		}
	} else if(action == ENABLE_BACKTRACE) {
		u8 flag = 0;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			head = head->next;
		}

		u8 *ptr = (u8 *)faddr;
		if(!flag) {
			return -EINVAL;
		} else if(*ptr != INV_OPCODE) {
			for(int i = 0; i < 4; i++) {
				head->code_backup[i] = ptr[i];
				ptr[i] = INV_OPCODE;
			}
		}
		head->capture_backtrace = 1;
	} else if(action == DISABLE_BACKTRACE) {
		u8 flag = 0;
		while(head) {
			if(head->faddr == faddr) {
				flag = 1;
				break;
			}
			head = head->next;
		}

		u8 *ptr = (u8 *)faddr;
		if(!flag) {
			return -EINVAL;
		} else if(*ptr == INV_OPCODE) {
			u8 *ptr = (u8 *)head->faddr;
			for(int i = 0; i < 4; i++) {
				ptr[i] = head->code_backup[i];
			}
			head->capture_backtrace = 0;
		}
	} else {
		return -EINVAL;
	} 

	return 0;
}



long handle_ftrace_fault(struct user_regs *regs) {
	u64 faddr = regs->entry_rip;

	struct exec_context *ctx = get_current_ctx();
	if(!ctx || !ctx->ft_md_base) {
		return -EINVAL;
	}

	struct ftrace_head *trace_head = ctx->ft_md_base;
	if(!trace_head->next) {
		return -EINVAL;
	}

	struct ftrace_info *head = trace_head->next;
	u8 flag = 0;
	u32 num_args = -1;
	while(head) {
		if(head->faddr == faddr) {
			num_args = head->num_args;
			flag = 1;
			break;
		}
		head = head->next;
	}

	if(!flag) {
		return -EINVAL;
	}

	regs->entry_rsp -= 8;
	u64 *ptr = (u64 *)regs->entry_rsp;
	*((u64 *)regs->entry_rsp) = regs->rbp;
	regs->rbp = regs->entry_rsp;

	u64 bytes_to_write = (num_args + 1) * sizeof(u64);
	if(head->capture_backtrace) {
		u64 *base_ptr = (u64 *)regs->rbp;
		bytes_to_write += sizeof(u64);
		while(*(base_ptr + 1) != END_ADDR) {
			bytes_to_write += sizeof(u64);
			base_ptr = (u64 *)(*base_ptr);
		}
	}
	u64 bytes_count[1] = { bytes_to_write };
	write_to_buffer(ctx->files[head->fd], (char *)bytes_count, sizeof(u64));

	u64 *buff = (u64 *)os_alloc((num_args + 1) * sizeof(u64));
	if(buff == NULL) {
		return -EINVAL;
	}

	buff[0] = faddr;
	if(num_args >= 1) {
		buff[1] = regs->rdi;
	}
	if(num_args >= 2) {
		buff[2] = regs->rsi;
	}
	if(num_args >= 3) {
		buff[3] = regs->rdx;
	}
	if(num_args >= 4) {
		buff[4] = regs->rcx;
	}
	if(num_args >= 5) {
		buff[5] = regs->r8;
	}
	if(num_args == 6) {
		buff[6] = regs->r9;
	}

	write_to_buffer(ctx->files[head->fd], (char *)buff, (num_args + 1) * sizeof(u64));

	u64 *back_trace_buff = (u64 *)os_alloc(sizeof(u64));
	if(head->capture_backtrace) {
		u64 *base_ptr = (u64 *)regs->rbp;
		back_trace_buff[0] = faddr;
		write_to_buffer(ctx->files[head->fd], (char *)back_trace_buff, sizeof(back_trace_buff));

		while(*(base_ptr + 1) != END_ADDR) {
			back_trace_buff[0] = *(base_ptr + 1);
			write_to_buffer(ctx->files[head->fd], (char *)back_trace_buff, sizeof(back_trace_buff));
			base_ptr = (u64 *)(*base_ptr);
		}
	}

	os_free(buff, (num_args + 1) * sizeof(u64));
	os_free(back_trace_buff, sizeof(u64));
	regs->entry_rip = faddr + 4;

	return 0;
}



int sys_read_ftrace(struct file *filep, char *buff, u64 count) {
	u32 bytes_read = 0;
	u32 buff_index = 0;
	for(int i = 0; i < count; i++) {
		u32 bytes = trace_buffer_read(filep, buff + buff_index, 8);
		if(bytes > 0) {
			u32 bytes_info = trace_buffer_read(filep, buff + buff_index, (u32)buff[buff_index]);
			if(bytes_info > 0) {
				bytes = bytes_info;
			} else {
				bytes = 0;
			}
		}
		bytes_read += bytes;
		buff_index += bytes;
	}

	return bytes_read;
}
