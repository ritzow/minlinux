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

	uint32_t * sq_entries; /* length of sq_array */

	uint32_t * sq_mask;   /* mask field location */
	uint32_t * sq_head;   /* sq_array head index field location */
	uint32_t * sq_tail;   /* sq_array tail index field location */
	uint32_t * sq_array;  /* indices into sqes */

	/* sq_array is used by io_uring and contains indices, this is our own */
	uint32_t sq_list_head; /* head of actual SQ entries */
	uint32_t sq_list_tail; /* tail of actual SQ entries, for our own use */

	uint32_t * cq_head;   /* cqes head index field location */
	uint32_t * cq_tail;   /* cqes tail index field location */
	uint32_t * cq_mask;   /* mask field location */
	uint32_t * cq_entries;

} uring_queue;

uring_queue uring_init(uint32_t size);

struct io_uring_sqe *uring_new_submission(uring_queue *);
void uring_wait(uring_queue *, uint32_t count);

void uring_submit_read(uring_queue *, int, void *, size_t);
void uring_submit_write_linked(uring_queue *, int, void *, size_t);

void uring_close(uring_queue *);

typedef struct {
	struct io_uring_cqe entry;
	bool success;
} cqe;

cqe uring_result(uring_queue *);

#endif