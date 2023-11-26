#define _DEFAULT_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>


#define _4MB (4 * 1024 * 1024)
#define size_ul unsigned long
#define CHUNK_HEADER_SIZE sizeof(size_ul)  // 8
#define CHUNK_METADATA_SIZE (CHUNK_HEADER_SIZE + 2 * sizeof(size_ul *))  // 24


size_ul *head = NULL;

size_ul GET_ALIGNED_SIZE(size_ul size) {
	return ((size + CHUNK_HEADER_SIZE - 1) / CHUNK_HEADER_SIZE) * CHUNK_HEADER_SIZE;
}

size_ul GET_CHUNK_SIZE(size_ul size) {
	return ((size + CHUNK_HEADER_SIZE + _4MB - 1) / _4MB) * _4MB;
}

void initialize_memory(size_ul size, size_ul **next_free) {
	*next_free = (size_ul *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (*next_free == MAP_FAILED) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	
	**next_free = size;
	*(*next_free + 2) = (size_ul) NULL;
	*(*next_free + 1) = (size_ul) NULL;
}



void *memalloc(size_ul requested_size) {

	if(requested_size == 0) {
		return NULL;
	}

	if(!head) {
		initialize_memory(GET_CHUNK_SIZE(requested_size), &head);
	}

	size_ul *curr = head;
	size_ul *prev = NULL;
	size_ul allocated_size = GET_ALIGNED_SIZE(requested_size) + CHUNK_HEADER_SIZE;
	allocated_size = allocated_size >= 24 ? allocated_size : 24;
	
	while (curr) {
		size_ul chunk_size = *curr;
		if (chunk_size >= allocated_size) {
			if (chunk_size >= allocated_size + CHUNK_METADATA_SIZE) {
				size_ul remaining_size = chunk_size - allocated_size;
				
				*curr = allocated_size;
				
				size_ul *next_free_chunk = curr + allocated_size / 8;
				*next_free_chunk = remaining_size;

				if (prev) {
					size_ul *next = (size_ul *) *(curr + 1);
					*(prev + 1) = (size_ul) next;
					if(next) {
						*(next + 2) = (size_ul) prev;
					}
					*(next_free_chunk + 1) = (size_ul) head;
					*(next_free_chunk + 2) = (size_ul) NULL;
					*(head + 2) = (size_ul) next_free_chunk;
					head = next_free_chunk;
				} else {
					size_ul *next = (size_ul *) *(curr + 1);
					*(next_free_chunk + 1) = (size_ul) next;
					*(next_free_chunk + 2) = (size_ul) NULL;
					if(next) {
						*(next + 2) = (size_ul) next_free_chunk;
					}
					head = next_free_chunk;
				}
			} else {
				*curr = chunk_size;
				size_ul *next = (size_ul *) *(curr + 1);
				if(prev) {
					if (next) {
						if(prev) *(prev + 1) = (size_ul) next;
						*(next + 2) = (size_ul) prev;
					}
				} else {
					head = next;
				}
			}

			return (void *)(curr + 1);
		}
		prev = curr;
		curr = (size_ul *) *(curr + 1);
	}
	
	initialize_memory(GET_CHUNK_SIZE(requested_size), &curr);
	*(prev + 1) = (size_ul) curr;
	*(curr + 2) = (size_ul) prev;
	size_ul chunk_size = *curr;
	if (chunk_size >= allocated_size + CHUNK_METADATA_SIZE) {
		size_ul remaining_size = chunk_size - allocated_size;
		
		*curr = allocated_size;
		
		size_ul *next_free_chunk = curr + allocated_size / 8;
		*next_free_chunk = remaining_size;

		size_ul *next = (size_ul *) *(curr + 1);
		*(prev + 1) = (size_ul) next;
		if(next) {
			*(next + 2) = (size_ul) prev;
		}
		*(next_free_chunk + 1) = (size_ul) head;
		*(next_free_chunk + 2) = (size_ul) NULL;
		*(head + 2) = (size_ul) next_free_chunk;
		head = next_free_chunk;
	} else {
		*curr = chunk_size;
		if(prev) {
			*(prev + 1) = (size_ul) NULL;
		}
	}

	return (void *)(curr + 1);
}



int memfree(void *ptr) {

	if(!ptr) {
		return -1;
	}

	size_ul *todelete = (size_ul *)ptr;
	todelete--;

	size_ul *curr = head;
	size_ul *prev = (size_ul *)NULL, *next = (size_ul *)NULL;

	while(curr) {
		if(curr + *curr / 8 == todelete) {
			prev = curr;
			break;
		}
		curr = (size_ul *) *(curr + 1);
	}

	curr = head;
	while(curr) {
		if(todelete + *todelete / 8 == curr) {
			next = curr;
			break;
		}
		curr = (size_ul *) *(curr + 1);
	}

	if(prev) {
		size_ul *before = (size_ul *) *(prev + 2);
		size_ul *after = (size_ul *) *(prev + 1);
		if(prev == head) {
			head = after;
		}
		if(before) {
			*(before + 1) = (size_ul) after;
		}
		if(after) {
			*(after + 2) = (size_ul) before;
		}
	}

	if(next) {
		size_ul *before = (size_ul *) *(next + 2);
		size_ul *after = (size_ul *) *(next + 1);
		if(next == head) {
			head = after;
		}
		if(before) {
			*(before + 1) = (size_ul) after;
		}
		if(after) {
			*(after + 2) = (size_ul) before;
		}
	}

	if(prev) {
		*prev += *todelete;
		todelete = prev;
		if(next) {
			*todelete += *next;
			*(todelete + 1) = *(next + 1);
		}
	} else if(next) {
		*todelete += *next;
		*(todelete + 1) = *(next + 1);
		*(todelete + 2) = *(next + 2);
	}

	if(!head) {
		head = todelete;
		*(todelete + 2) = (size_ul) NULL;
		*(todelete + 1) = (size_ul) NULL;
	} else {
		*(todelete + 1) = (size_ul) head;
		*(todelete + 2) = (size_ul) NULL;
		*(head + 2) = (size_ul) todelete;
		head = todelete;
	}

	return 0;
}