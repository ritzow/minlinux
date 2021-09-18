#include "syscall.h"
#include "uring_ctl.h"
#include "util.h"

/* Example code from liburing https://man.archlinux.org/man/io_uring.7 */

/* For teardown see 
https://github.com/axboe/liburing/blob/10a0f65c617ed084281cd16c8b6e41cd6e307235/src/setup.c#L171 */

uring_queue uring_init(uint32_t size) {
	/* Let linux determine sq_entries and cq_entries, etc. */
	struct io_uring_params p = {};
	uring_queue uring;

	/* See io_uring_setup(2) for io_uring_params.flags you can set */
	uring.ringfd = io_uring_setup(size, &p);

	if (uring.ringfd < 0) {
		exit(1);
	}

	if (!(p.features & IORING_FEAT_SINGLE_MMAP)) {
		exit(1);
	}

	int sring_sz = p.sq_off.array + p.sq_entries * sizeof(uint32_t);
	int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);

	/* Determine whethether the end of cring or sring is farther in 
		(i.e. which comes before the other), that is the length of combined buffer */
	uring.mmap_ring_size = cring_sz > sring_sz ? cring_sz : sring_sz;
	intptr_t res = SYSCHECK(mmap(NULL, uring.mmap_ring_size,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_POPULATE,
		uring.ringfd, IORING_OFF_SQ_RING
	));

	uring.mmap_ring = (void*)res;
	
	/* Save useful fields for later easy reference */
	uring.sq_head = uring.mmap_ring + p.sq_off.head;
	uring.sq_tail = uring.mmap_ring + p.sq_off.tail;
	uring.sq_mask =	uring.mmap_ring + p.sq_off.ring_mask;
	uring.sq_array = uring.mmap_ring + p.sq_off.array;
	uring.sq_entries = uring.mmap_ring + p.sq_off.ring_entries;

	 /* Save useful fields for later easy reference */
	uring.cq_head =	uring.mmap_ring + p.cq_off.head;
	uring.cq_tail =	uring.mmap_ring + p.cq_off.tail;
	uring.cq_mask =	uring.mmap_ring + p.cq_off.ring_mask;
	uring.cqes = 	uring.mmap_ring + p.cq_off.cqes;
	uring.cq_entries = uring.mmap_ring + p.cq_off.ring_entries;

	/* Map in the submission queue entries array */
	uring.mmap_sqes_size = p.sq_entries * sizeof(struct io_uring_sqe);
	uring.mmap_sqes = (void*)SYSCHECK(mmap(NULL, 
		uring.mmap_sqes_size,
		PROT_READ | PROT_WRITE, 
		MAP_SHARED | MAP_POPULATE,
		uring.ringfd, IORING_OFF_SQES
	));

	uring.sqes = uring.mmap_sqes;

	uring.sq_list_head = 0;
	uring.sq_list_tail = 0;

	return uring;
}

/*
* Read from completion queue.
* In this function, we read completion events from the completion queue.
* We dequeue the CQE, update and head and return the result of the operation.
* */
int read_from_cq(uring_queue* uring) {
	/* Read barrier */
	uint32_t head = __atomic_load_n(uring->cq_head, __ATOMIC_ACQUIRE);

	/* If head == tail, the buffer is empty. */
	if (head == *uring->cq_tail) {
		return -1;
	}

	/* Get the entry */
	struct io_uring_cqe * cqe = &uring->cqes[head & (*uring->cq_mask)];
	int res = SYSCHECK(cqe->res);

	/* Write barrier so that update to the head are made visible */
	__atomic_store_n(uring->cq_head, head + 1, __ATOMIC_RELEASE);
	return res;
}

/* From liburing io_uring_get_sqe */
struct io_uring_sqe *uring_new_submission(uring_queue * uring) {
	uint32_t head = __atomic_load_n(uring->sq_head, __ATOMIC_ACQUIRE);
	uint32_t next = uring->sq_list_tail + 1;
	struct io_uring_sqe *sqe = NULL;

	if (next - head <= *uring->sq_entries) {
		sqe = &uring->sqes[uring->sq_list_tail & *uring->sq_mask];
		uring->sq_list_tail = next;
	}
	return sqe;
}

/* From liburing __io_uring_submit_and_wait (__io_uring_flush_sq) */
void uring_wait(uring_queue * uring, uint32_t count) {
	const uint32_t mask = *uring->sq_mask;
	uint32_t sq_tail = *uring->sq_tail;
	uint32_t to_submit = uring->sq_list_tail - uring->sq_list_head;

	/* If queue isn't already full */
	if (to_submit > 0) {
		/* Fill in sqes that we have queued up, adding them to the kernel ring */
		do {
			/* place the index of each new entry into the index list */
			uring->sq_array[sq_tail & mask] = uring->sq_list_head & mask;
			sq_tail++;
			uring->sq_list_head++;
		} while (--to_submit);

		/*
		* Ensure that the kernel sees the SQE updates before it sees the tail
		* update.
		*/
		__atomic_store_n(uring->sq_tail, sq_tail, __ATOMIC_RELEASE);
	}

	/*
	 * This _may_ look problematic, as we're not supposed to be reading
	 * SQ->head without acquire semantics. When we're in SQPOLL mode, the
	 * kernel submitter could be updating this right now. For non-SQPOLL,
	 * task itself does it, and there's no potential race. But even for
	 * SQPOLL, the load is going to be potentially out-of-date the very
	 * instant it's done, regardless or whether or not it's done
	 * atomically. Worst case, we're going to be over-estimating what
	 * we can submit. The point is, we need to be able to deal with this
	 * situation regardless of any perceived atomicity.
	 */
	uint32_t submitted = sq_tail - *uring->sq_head;

	SYSCHECK(io_uring_enter(uring->ringfd, submitted, count, IORING_ENTER_GETEVENTS, NULL));
}

void uring_submit_read(uring_queue * uring, int fd, void * dst, size_t count) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_READ;
	sqe->fd = fd;
	sqe->addr = (uintptr_t)dst;
	sqe->len = count;
	sqe->flags = 0;
	sqe->ioprio = 0;
}

void uring_submit_write(uring_queue * uring, int fd, void * src, size_t count) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_WRITE;
	sqe->fd = fd;
	sqe->addr = (uintptr_t)src;
	sqe->len = count;
	sqe->flags = 0;
	sqe->ioprio = 0;
}

void submit_to_sq(uring_queue * uring, int fd, int op, size_t bufsize, void * buffer) {
	/* Add our submission queue entry to the tail of the SQE ring buffer */
	uint32_t tail = *uring->sq_tail;
	uint32_t index = tail & (*uring->sq_mask);
	struct io_uring_sqe *sqe = &uring->sqes[index];

	/* Fill in the parameters required for the read or write operation */
	sqe->opcode = op;
	sqe->fd = fd;
	sqe->addr = (uintptr_t)buffer;
	sqe->flags = 0;
	sqe->ioprio = 0;

	switch(op) {
		case IORING_OP_READ:
			/* So that it's null terminated */
			memset(buffer, 0, bufsize);
			sqe->len = bufsize;
			break;
		case IORING_OP_WRITE:
			/* Could cause error if exactly bufsize length input? */
			sqe->len = strlen(buffer);
			break;
		default:
			exit(1);
			break;
	}

	//sqe->off = offset;
	uring->sq_array[index] = index;
	/* Update the tail */
	__atomic_store_n(uring->sq_tail, tail + 1, __ATOMIC_RELEASE);
	/*
	* Tell the kernel we have submitted events with the io_uring_enter() system
	* call. We also pass in the IOURING_ENTER_GETEVENTS flag which causes the
	* io_uring_enter() call to wait until min_complete (the 3rd param) events
	* complete.
	* */
	SYSCHECK(io_uring_enter(uring->ringfd, 1, 1, IORING_ENTER_GETEVENTS, NULL));
}

void uring_close(uring_queue * uring) {
	SYSCHECK(munmap(uring->mmap_ring, uring->mmap_ring_size));
	SYSCHECK(munmap(uring->mmap_sqes, uring->mmap_sqes_size));
	SYSCHECK(close(uring->ringfd));
}