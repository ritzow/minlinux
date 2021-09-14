void setup_uring();
int read_from_cq();
int submit_to_sq(int fd, int op);
extern off_t offset;