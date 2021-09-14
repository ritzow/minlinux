#include "syscall.h"
#include "uring_ctl.h"

#define QUEUE_DEPTH 1
#define BLOCK_SZ    1024

/* Example code from https://man.archlinux.org/man/io_uring.7 */

int ring_fd;
unsigned *sring_tail, *sring_mask, *sring_array, 
            *cring_head, *cring_tail, *cring_mask;
struct io_uring_sqe *sqes;
struct io_uring_cqe *cqes;
char buff[BLOCK_SZ];
off_t offset;

void setup_uring() {
    struct io_uring_params p = {};
    void *sq_ptr, *cq_ptr;
    /* See io_uring_setup(2) for io_uring_params.flags you can set */
    ring_fd = io_uring_setup(QUEUE_DEPTH, &p);

    if (ring_fd < 0) {
		sys_exit(1);
    }

    /*
     * io_uring communication happens via 2 shared kernel-user space ring
     * buffers, which can be jointly mapped with a single mmap() call in
     * kernels >= 5.4.
     */
    int sring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
    int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);

    /* Rather than check for kernel version, the recommended way is to
     * check the features field of the io_uring_params structure, which is a 
     * bitmask. If IORING_FEAT_SINGLE_MMAP is set, we can do away with the
     * second mmap() call to map in the completion ring separately.
     */
    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        if (cring_sz > sring_sz)
            sring_sz = cring_sz;
        cring_sz = sring_sz;
    }


    /* Map in the submission and completion queue ring buffers.
     *  Kernels < 5.4 only map in the submission queue, though.
     */
    sq_ptr = mmap(NULL, sring_sz, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE,
                  ring_fd, IORING_OFF_SQ_RING);

    if (sq_ptr < 0) {
        sys_exit(1);
    }
    if (p.features & IORING_FEAT_SINGLE_MMAP) {
        cq_ptr = sq_ptr;
    } else {
        /* Map in the completion queue ring buffer in older kernels separately */
        cq_ptr = mmap(NULL, cring_sz, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE,
            ring_fd, IORING_OFF_CQ_RING);
        if (cq_ptr < 0) {
            sys_exit(1);
        }
    }
    /* Save useful fields for later easy reference */
    sring_tail = sq_ptr + p.sq_off.tail;
    sring_mask = sq_ptr + p.sq_off.ring_mask;
    sring_array = sq_ptr + p.sq_off.array;
    /* Map in the submission queue entries array */
    sqes = mmap(0, p.sq_entries * sizeof(struct io_uring_sqe),
                   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                   ring_fd, IORING_OFF_SQES);
    if (sqes < 0) {
		sys_exit(1);
    }
    /* Save useful fields for later easy reference */
    cring_head = cq_ptr + p.cq_off.head;
    cring_tail = cq_ptr + p.cq_off.tail;
    cring_mask = cq_ptr + p.cq_off.ring_mask;
    cqes = cq_ptr + p.cq_off.cqes;
    return 0;
}

/*
* Read from completion queue.
* In this function, we read completion events from the completion queue.
* We dequeue the CQE, update and head and return the result of the operation.
* */
int read_from_cq() {
    struct io_uring_cqe *cqe;
    unsigned head, reaped = 0;
    /* Read barrier */

    __atomic_load(cring_head, &head, __ATOMIC_ACQUIRE);
    /*
    * Remember, this is a ring buffer. If head == tail, it means that the
    * buffer is empty.
    * */
    if (head == *cring_tail)
        return -1;
    /* Get the entry */
    cqe = &cqes[head & (*cring_mask)];
    if (cqe->res < 0) {
        sys_exit(1);
    }    
    head++;
    /* Write barrier so that update to the head are made visible */
    __atomic_store(cring_head, &head, __ATOMIC_RELEASE);
    return cqe->res;
}

/*
* Submit a read or a write request to the submission queue.
* */
int submit_to_sq(int fd, int op) {
    unsigned index, tail;
    /* Add our submission queue entry to the tail of the SQE ring buffer */
    tail = *sring_tail;
    index = tail & *sring_mask;
    struct io_uring_sqe *sqe = &sqes[index];
    /* Fill in the parameters required for the read or write operation */
    sqe->opcode = op;
    sqe->fd = fd;
    sqe->addr = (unsigned long) buff;
    if (op == IORING_OP_READ) {
        memset(buff, 0, sizeof(buff));
        sqe->len = BLOCK_SZ;
    }
    else {
        sqe->len = strlen(buff);
    }
    sqe->off = offset;
    sring_array[index] = index;
    tail++;
    /* Update the tail */
    __atomic_store(sring_tail, &tail, __ATOMIC_RELEASE);
    /*
    * Tell the kernel we have submitted events with the io_uring_enter() system
    * call. We also pass in the IOURING_ENTER_GETEVENTS flag which causes the
    * io_uring_enter() call to wait until min_complete (the 3rd param) events
    * complete.
    * */
    int ret =  io_uring_enter(ring_fd, 1,1,
                              IORING_ENTER_GETEVENTS);
    if(ret < 0) {
        sys_exit(1);
    }
    return ret;
}