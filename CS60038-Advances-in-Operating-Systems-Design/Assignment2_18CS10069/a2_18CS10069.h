/*
	Roll No: 18CS10069
	Name: Siba Smarak Panigrahi
	CS60038 : Assignment 2 
	Header LKM For Deque
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/hashtable.h>

#define DEVICE "cs60038_a2_18CS10069"
#define INF 100000000 /* 32 bit integers */
#define current get_current()
/* ioctl commands */
#define PB2_SET_CAPACITY _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT_RIGHT _IOW(0x10, 0x32, int32_t*)
#define PB2_INSERT_LEFT _IOW(0x10, 0x33, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x34, int32_t*)
#define PB2_POP_LEFT _IOR(0x10, 0x35, int32_t*)
#define PB2_POP_RIGHT _IOR(0x10, 0x36, int32_t*)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siba Smarak Panigrahi (18CS10069)");


/* define spinlock to be used while updating an entry in hashtable */
/* in this program hashtable is representad as a linkedlist of hashtable struct (defined below)*/
static DEFINE_SPINLOCK(deque_spin_lock); 

struct obj_info{
	int32_t deque_size;// current number of elementsin deque
	int32_t capacity;// maximum capacity of the deque
};

struct Deque{
	int32_t *arr; /* deque */
	int32_t count; /* total existing elements in deque */
	int32_t capacity; /* total capacity of deque (N) */
};
typedef struct Deque Deque;

struct hashtable{
	int key; /* process ID */
	Deque* global_deque; /* corresponding deque */
	struct hashtable* next; 
};
struct hashtable *process_deque_htable, *htable_elem;

/* methods to interact with hashtable */
static struct hashtable* get_htable_elem(int key);
static void add_to_htable(struct hashtable*);
static void free_process_deque_htable(void);
static void remove_process(int key);
static void print_all_processes(void);

/* Deque methods */
static Deque *create_deque(int32_t capacity);
static Deque* destroy_deque(Deque* deque);
static int32_t insert(Deque *deque, int32_t key, int32_t lr);
static int32_t remove(Deque *deque, int32_t lr);

static long dev_ioctl(struct file *, unsigned int, unsigned long);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int open_processes = 0;

static struct proc_ops file_ops =
{
	.proc_open = dev_open,
	.proc_read = dev_read,
	.proc_write = dev_write,
	.proc_release = dev_release,
	.proc_ioctl = dev_ioctl,
};