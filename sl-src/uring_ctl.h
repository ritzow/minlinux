typedef struct {
	int ringfd;
	void * mmap_ring;
	size_t mmap_ring_size;
	void * mmap_sqes;
	size_t mmap_sqes_size;

	uint32_t * sq_mask;
	uint32_t * sq_head;
	uint32_t * sq_tail;
	uint32_t * sq_array;
	uint32_t * cq_head;
	uint32_t * cq_tail;
	uint32_t * cq_mask;
	uint32_t * cqes;

	struct io_sqring_offsets sq_off;
	struct io_cqring_offsets cq_off;
} uring_queue;

uring_queue setup_uring();
int read_from_cq();
void submit_to_sq(uring_queue *, int, int, size_t, void *, off_t);