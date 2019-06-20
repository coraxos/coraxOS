#include "coraxstd.h"
#include "circularqueue.h"
#include "kmalloc.h"
#include "assert.h"
#include "pipe.h"

/* Prototypes */
static int pipe_read(struct inode *node, void *buf, uint32_t offset,
		     uint32_t size);
static int pipe_write(struct inode *inode, void *buf, uint32_t offset,
		      uint32_t size);
static int pipe_noop();

inode_operations_t inode_pipe_ops = { .find_child = NULL,
				      .get_child = NULL,
				      .open = pipe_noop,
				      .close = pipe_noop,
				      .read = pipe_read,
				      .write = pipe_write,
				      .mkdir = NULL };

inode_t *make_pipe(size_t size)
{
	CircularQueue *queue = CircularQueueCreate(size);

	inode_t *inode = kcalloc(sizeof(inode_t));
	inode->i_op = &inode_pipe_ops;
	inode->device = queue;

	return inode;
}
static int pipe_read(struct inode *node, void *buf, uint32_t offset,
		     uint32_t size)
{
	assert(node->device != NULL);
	CircularQueue *queue = node->device;
	uint8_t *buffer = buf;

	int readAmount = 0;
	while (readAmount < size) {
		int ret = CircularQueueFront(queue);
		if (ret == NULL) { //TODO, what if we enqueue 0?
			break;
		}
		buffer[readAmount] = ret;
		CircularQueueDeQueue(queue);
		readAmount++;
	}
	return readAmount;
}
static int pipe_write(struct inode *node, void *buf, uint32_t offset,
		      uint32_t size)
{
	assert(node->device != NULL);
	CircularQueue *queue = node->device;
	uint8_t *buffer = buf;

	int writeAmount = 0;
	while (writeAmount < size) {
		bool ret = CircularQueueEnQueue(queue, buffer[writeAmount]);
		if (!ret) {
			break;
		}
		writeAmount++;
	}
	return writeAmount;
}
static int pipe_noop()
{
	return 0;
}