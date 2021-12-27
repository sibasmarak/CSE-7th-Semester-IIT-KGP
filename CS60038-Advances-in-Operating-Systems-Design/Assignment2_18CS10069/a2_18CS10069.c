/*
	Roll No: 18CS10069
	Name: Siba Smarak Panigrahi
	CS60038 : Assignment 1 
	LKM For Deque with ioctl commands
*/
#include "a2_18CS10069.h"

/* Return a process with a specific pic */
static struct hashtable* get_htable_elem(int key)
{
	struct hashtable* temp = process_deque_htable->next;
	while (temp != NULL){
		if (temp->key == key){
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

/* Add a new process with pid to the linked list */
static void add_to_htable(struct hashtable* htable_elem)
{
	htable_elem->next = process_deque_htable->next;
	process_deque_htable->next = htable_elem;
}

/* Destroy hashtable of processes */
static void free_process_deque_htable(void)
{
	struct hashtable *temp, *temp2;
	temp = process_deque_htable->next;
	process_deque_htable->next = NULL;
	while (temp != NULL){
		temp2 = temp;
		printk(KERN_INFO DEVICE ": (free_process_deque_htable) Free key = %d", temp->key);
		temp = temp->next;
		kfree(temp2);
	}
	kfree(process_deque_htable);
}

/* Deletes a process from hashtable of processes */
static void remove_process(int key)
{
	struct hashtable *prev, *temp;
	prev = temp = process_deque_htable;
	temp = temp->next;

	while (temp != NULL){
		if (temp->key == key){
			prev->next = temp->next;
			temp->global_deque = destroy_deque(temp->global_deque);
			temp->next = NULL;
			printk(KERN_INFO DEVICE ": (remove_process) (PID %d) Free key = %d", current->pid, temp->key);
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}

/* Prints all the process pid */
static void print_all_processes(void)
{
	struct hashtable *temp;
	temp = process_deque_htable->next;
	printk(KERN_INFO DEVICE ": (print_all_processes) Total %d processes", open_processes);
	while (temp != NULL){
		printk(KERN_INFO DEVICE ": (print_all_processes) PID = %d", temp->key);
		temp = temp->next;
	}
}

/* Create Deque */
static Deque *create_deque(int32_t capacity)
{
	Deque *deque = (Deque *) kmalloc(sizeof(Deque), GFP_KERNEL);

	if (deque == NULL){
		printk(KERN_ALERT DEVICE ": (create_deque) (PID %d) Memory Error in allocating deque for the process", current->pid);
		return NULL;
	}
	deque->count = 0;
	deque->capacity = capacity;
	deque->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); 

	if (deque->arr == NULL){
		printk(KERN_ALERT DEVICE ": (create_deque) (PID %d) Memory Error while allocating deque->arr", current->pid);
		return NULL;
	}
	return deque;
}

/* Destroy Deque */
static Deque* destroy_deque(Deque* deque)
{
	if (deque == NULL)
		/* sanity check to check the existence of deque */
		return NULL; 
	printk(KERN_INFO DEVICE ": (destroy_deque) (PID %d) %ld bytes freed.\n", current->pid, sizeof(deque->arr));
	kfree_const(deque->arr); /* free deque->arr */
	kfree_const(deque); /* free deque */
	return NULL;
}

/* Insert a number into deque */
static int32_t insert(Deque *deque, int32_t key, int32_t lr)
{
	if (deque->count < deque->capacity){
		/* if number is even; insert at the end */
		if (lr == 1){
			deque->arr[deque->count] = key;
		}
		else{
			/* if number is odd; insert at the front */
			int32_t i;
			for(i = deque->count; i > 0; i--){
				deque->arr[i] = deque->arr[i-1];
			}
			deque->arr[0] = key;
		}
		deque->count++;
	}
	else{
		/* number of elements in deque exceeded its capacity */
		return -EACCES;  
	}
	return 0;
}

/* Extract the first element of a deque */
static int32_t remove(Deque *deque, int32_t lr)
{
	int32_t pop, i;
	if (deque->count == 0){
		/* if deque is empty; return -INF */
		return -INF;
	}
	if (lr == 0){
		/* extract from left end */
		pop = deque->arr[0]; /* read from the front */
		deque->count--; /* reduce the number of elements in deque */
		
		/* shift deque by one place to left */
		for(i = 0; i < deque->count; i++){
			deque->arr[i] = deque->arr[i+1];
		}
	}
	else if(lr == 1){
		/* extract from right end */
		pop = deque->arr[deque->count-1]; /* read from the end */
		deque->count--; /* reduce the number of elements in deque */
	}
	return pop;
}

/* handle ioctl commands for device */
static long dev_ioctl(struct file *file, unsigned int command, unsigned long arg) 
{
	struct hashtable *htable_elem;
	struct obj_info deque_info;
	int32_t deque_initialized = 0;
	int32_t num;
	int32_t ret;
	int32_t deque_size;
	int32_t first_deque_elem;

	switch (command) {
	case PB2_SET_CAPACITY:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}

		ret = copy_from_user(&deque_size, (int *)arg, sizeof(int32_t));
		if (ret)
			return -EINVAL;
	
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) Deque size received: %d", current->pid, deque_size);

		/* check deque size */
		if (deque_size <= 0 || deque_size > 100){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_SET_CAPACITY) (PID %d) Deque size must be an integer in [0,100]", current->pid);
			return -EINVAL;
		}

		htable_elem->global_deque = destroy_deque(htable_elem->global_deque);	/* reset the deque */
		htable_elem->global_deque = create_deque(deque_size);	/* allocate space for new deque */

		break;

	case PB2_INSERT_LEFT:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_LEFT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		deque_initialized = (htable_elem->global_deque) ? 1 : 0;
		
		/* cannot read from non-initialized deque */
		if (deque_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_LEFT) (PID %d) deque not initialized", current->pid);
			return -EACCES;
		}

		/* obtain the number to be written in left of deque; simultaneously checked for invalid value */
		ret = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(ret)
			return -EINVAL;

		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_INSERT_LEFT) (PID %d) Writing %d to deque\n", current->pid, num);

		/* if deque is initialized and number has been obtained */
		ret = insert(htable_elem->global_deque, num, 0); /* insert into deque */
		if (ret < 0){ 
			/* deque filled to capacity */ 
			return -EACCES;
		}

		break;

	case PB2_INSERT_RIGHT:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_RIGHT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		deque_initialized = (htable_elem->global_deque) ? 1 : 0;
		
		/* cannot read from non-initialized deque */
		if (deque_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_RIGHT) (PID %d) deque not initialized", current->pid);
			return -EACCES;
		}

		/* obtain the number to be written in right of deque; simultaneously checked for invalid value */
		ret = copy_from_user(&num, (int32_t *)arg, sizeof(int32_t));
		if(ret)
			return -EINVAL;

		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_INSERT_RIGHT) (PID %d) Writing %d to deque\n", current->pid, num);

		/* if deque is initialized and number has been obtained */
		ret = insert(htable_elem->global_deque, num, 1); /* insert into deque */
		if (ret < 0){ 
			/* deque filled to capacity */ 
			return -EACCES;
		}

		break;

	case PB2_GET_INFO:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_RIGHT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		deque_initialized = (htable_elem->global_deque) ? 1 : 0;
		
		/* cannot read from non-initialized deque */
		if (deque_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_INSERT_RIGHT) (PID %d) deque not initialized", current->pid);
			return -EACCES;
		}

		/* create obj_info struct and return to user process */
		deque_info.deque_size = htable_elem->global_deque->count;
		deque_info.capacity = htable_elem->global_deque->capacity;

		ret = copy_to_user((struct obj_info *)arg, &deque_info, sizeof(struct obj_info));
		if (ret != 0)
			return -EACCES;
		break;

	case PB2_POP_LEFT:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_EXTRACT_LEFT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		deque_initialized = (htable_elem->global_deque) ? 1 : 0;
		
		/* cannot read from non-initialized deque */
		if (deque_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_EXTRACT_LEFT) (PID %d) deque not initialized", current->pid);
			return -EACCES;
		}
		
		/* return error if deque is empty */
		if(htable_elem->global_deque->count == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_LEFT) (PID %d) deque is empty", current->pid);
			return -EACCES;
		}

		/* obtain leftmost element to send to userspace */
		first_deque_elem = remove(htable_elem->global_deque, 0);
		ret = copy_to_user((int32_t*)arg, (int32_t*)&first_deque_elem, sizeof(int32_t));

		if (ret != 0){   
			/* unable to send data to user process */ 
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_LEFT) (PID %d) first_deque_elem is %d; failed to send to user process", current->pid, first_deque_elem);
			return -EACCES;      
		}
		
		/* obtained the first elem of deque */
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_POP_LEFT) (PID %d) Sending data of %ld bytes with value %d to the user process", current->pid, sizeof(first_deque_elem), first_deque_elem);
		break;
	
	case PB2_POP_RIGHT:
		htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
		if (htable_elem == NULL) {
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_RIGHT) (PID %d) htable_elem is non-existent", current->pid);
			return -EACCES;
		}
		
		deque_initialized = (htable_elem->global_deque) ? 1 : 0;
		
		/* cannot read from non-initialized deque */
		if (deque_initialized == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_RIGHT) (PID %d) deque not initialized", current->pid);
			return -EACCES;
		}
		
		/* return error if deque is empty */
		if(htable_elem->global_deque->count == 0){
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_RIGHT) (PID %d) deque is empty", current->pid);
			return -EACCES;
		}

		/* obtain rightmost element to send to userspace */
		first_deque_elem = remove(htable_elem->global_deque, 1);
		ret = copy_to_user((int32_t*)arg, (int32_t*)&first_deque_elem, sizeof(int32_t));

		if (ret != 0){   
			/* unable to send data to user process */ 
			printk(KERN_ALERT DEVICE ": (dev_ioctl : PB2_POP_RIGHT) (PID %d) first_deque_elem is %d; failed to send to user process", current->pid, first_deque_elem);
			return -EACCES;      
		}
		
		/* obtained the first elem of deque */
		printk(KERN_INFO DEVICE ": (dev_ioctl : PB2_POP_RIGHT) (PID %d) Sending data of %ld bytes with value %d to the user process", current->pid, sizeof(first_deque_elem), first_deque_elem);
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static ssize_t dev_write(struct file *file, const char* buf, size_t count, loff_t* pos)
{
	char arr[4];
	int32_t deque_size;
	int32_t deque_initialized = 0;
	struct hashtable *htable_elem;
	char buffer[256] ={0};
	int32_t buffer_len = 0;
	int32_t num;
	int32_t ret;

	if (!buf || !count)
		return -EINVAL;
	if (copy_from_user(buffer, buf, count < 256 ? count : 256))
		return -ENOBUFS;

	htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
	if (htable_elem == NULL){
		printk(KERN_ALERT DEVICE ": (dev_write) (PID %d) htable_elem is non-existent", current->pid);
		return -EACCES;
	}
	
	deque_initialized = (htable_elem->global_deque) ? 1 : 0;
	buffer_len = count < 256 ? count : 256;

	/* deque is initialized; hence this is a number to enter into deque */
	if (deque_initialized){
		if (count != 4){
			/* input should be of length 4 */
			printk(KERN_ALERT DEVICE ": (dev_write) (PID %d) %d bytes received; should have been 4 bytes (int)", current->pid, buffer_len);
			return -EINVAL;
		}
		
		memset(arr, 0, 4 * sizeof(char));
		memcpy(arr, buf, count * sizeof(char));
		memcpy(&num, arr, sizeof(num));
		printk(KERN_INFO DEVICE ": (dev_write) (PID %d) Writing %d to deque\n", current->pid, num);

		if (num % 2 == 0){
			ret = insert(htable_elem->global_deque, num, 1); /* insert into deque from right */
		}
		else{
			ret = insert(htable_elem->global_deque, num, 0); /* insert into deque from left */
		}
		if (ret < 0){ 
			/* deque filled to capacity */ 
			return -EACCES;
		}
		return sizeof(num);
	}

	/* deque is not initialized; only one argument (N) expected from user process */
	if (buffer_len != 1){
		return -EACCES;
	}

	deque_size = buf[0];	
	printk(KERN_INFO DEVICE ": (dev_write) (PID %d) Deque size received: %d", current->pid, deque_size);

	/* check deque size */
	if (deque_size <= 0 || deque_size > 100){
		printk(KERN_ALERT DEVICE ": (dev_Write) (PID %d) Deque size must be an integer in [0,100] \n", current->pid);
		return -EINVAL;
	}

	htable_elem->global_deque = destroy_deque(htable_elem->global_deque);	/* reset the deque */
	htable_elem->global_deque = create_deque(deque_size);	/* allocate space for new deque */

	return buffer_len;
}

static ssize_t dev_read(struct file *file, char* buf, size_t count, loff_t* pos)
{
	int32_t temp_ret = -1;
	int32_t deque_initialized = 0;
	struct hashtable *htable_elem;
	int32_t first_deque_elem;

	if (!buf || !count)
		return -EINVAL;

	htable_elem = get_htable_elem(current->pid); /* get hashtable entry corresponding to the current process PID */
	if (htable_elem == NULL){
		printk(KERN_ALERT DEVICE ": (dev_read) (PID %d) htable_elem is non-existent", current->pid);
		return -EACCES;
	}

	deque_initialized = (htable_elem->global_deque) ? 1 : 0;

	 /* cannot read from non-initialized deque */
	if (deque_initialized == 0){
		printk(KERN_ALERT DEVICE ": (dev_read) (PID %d) Deque not initialized", current->pid);
		return -EACCES;
	}

	/* if deque initialized; get the first element from the front of deque */
	first_deque_elem = remove(htable_elem->global_deque, 0);
	printk(KERN_INFO DEVICE ": (dev_read) (PID %d) expecting %ld bytes\n", current->pid, count);
	temp_ret = copy_to_user(buf, (int32_t*)&first_deque_elem, count < sizeof(first_deque_elem) ? count : sizeof(first_deque_elem));
	if (temp_ret == 0 && first_deque_elem != -INF){    
		/* obtained the first elem of deque */
		printk(KERN_INFO DEVICE ": (dev_read) (PID %d) Sending data of %ld bytes with value %d to the user process", current->pid, sizeof(first_deque_elem), first_deque_elem);
		return sizeof(first_deque_elem);
	}
	else{
		printk(KERN_INFO DEVICE ": (dev_read) (PID %d) first_deque_elem is %d; failed to send to user process", current->pid, first_deque_elem);
		return -EACCES;      
	}
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	struct hashtable *htable_elem;

	/* a process cannot open the file more than once */
	if (get_htable_elem(current->pid) != NULL){
		/* successful to find the process; process already has opened the file */
		printk(KERN_ALERT DEVICE ": (dev_open) (PID %d) Process tried to open the file twice\n", current->pid);
		return -EACCES;
	}

	/* new process; create a corresponding htable_elem in process_deque_htable */ 
	htable_elem = kmalloc(sizeof(struct hashtable), GFP_KERNEL);
	*htable_elem = (struct hashtable){current->pid, NULL, NULL};

	/* obtain the lock before adding the current process to process_deque_htable */
	spin_lock(&deque_spin_lock);
	printk(DEVICE ": (dev_open) (PID %d) Adding %d to hashtable", current->pid, htable_elem->key);
	add_to_htable(htable_elem); /* add the process into hashtable */

	open_processes++;
	printk(KERN_INFO DEVICE ": (dev_open) (PID %d) Device has been opened by %d processes", current->pid, open_processes);
	print_all_processes();
	spin_unlock(&deque_spin_lock);
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	/*
		obtain the spin_lock 
		ensure no other process accesses the process linked list when current process is releasing the device
	*/
	spin_lock(&deque_spin_lock);
	remove_process(current->pid);	/* remove the process from linked list */
	open_processes--;
	printk(KERN_INFO DEVICE ": (dev_release) (PID %d) Device successfully closed\n", current->pid);
	print_all_processes();
	spin_unlock(&deque_spin_lock); /* release the spin_lock */
	return 0;
}

static int LKM_deque_init(void)
{
	struct proc_dir_entry *htable_elem = proc_create(DEVICE, 0, NULL, &file_ops); /* create device (LKM) */
	if (!htable_elem)
		return -ENOENT;
	process_deque_htable = kmalloc(sizeof(struct hashtable), GFP_KERNEL); /* allocate memory to hashtable for process ID (key) -> process deque*/
	if(process_deque_htable == NULL){
		printk(KERN_ALERT DEVICE ": (LKM_deque_init) Cannot allocate memory to process -> deque hash table.");
		return -ENOMEM;
	}
	*process_deque_htable = (struct hashtable){ -1, NULL, NULL};
	printk(KERN_ALERT DEVICE ": (LKM_deque_init) Init Deque LKM\n");
	spin_lock_init(&deque_spin_lock); /* initialize spin_lock */
	return 0;
}

static void LKM_deque_exit(void)
{
	free_process_deque_htable(); /* free the hash table memory */
	remove_proc_entry(DEVICE, NULL); /* remove the device (LKM) */

	printk(KERN_ALERT DEVICE ": Exiting Deque LKM\n");
}

module_init(LKM_deque_init);
module_exit(LKM_deque_exit);