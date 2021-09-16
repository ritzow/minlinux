#ifndef __URING_CTL_H__
#define __URING_CTL_H__

#include "syscall.h"
#include "util.h"

typedef struct {
	int ringfd;
	void * mmap_ring;
	size_t mmap_ring_size;
	void * mmap_sqes;
	size_t mmap_sqes_size;

	struct io_uring_cqe * cqes; /* cqe structs array */
	struct io_uring_sqe * sqes; /* sqe structs array */

	uint32_t * sq_mask;   /* mask field location */
	uint32_t * sq_head;   /* head index field location */
	uint32_t * sq_tail;   /* sq_array tail index field location */
	uint32_t * sq_array;  /* indices into sqes */
	uint32_t * cq_head;   /* cqes head index field location */
	uint32_t * cq_tail;   /* cqes tail index field location */
	uint32_t * cq_mask;   /* mask field location */

} uring_queue;

uring_queue setup_uring();
int read_from_cq(uring_queue *);
void submit_to_sq(uring_queue *, int, int, size_t, void *);
void uring_close(uring_queue *);

#endif