/*
 * chardev.c: Crea un chardev que se le envia datos y se los puede leer luego. 
 *
 * Basado en chardev.c desde TLDP.org's LKMPG book.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h> /* for put_user */


int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define PROCFS_MAX_SIZE 2048 
#define BUFFER_SIZE 256
static char device_buffer[PROCFS_MAX_SIZE]; //The buffer (2k) for this module
//static int buffer_position;
static unsigned long procfs_buffer_size = 0;


#define SUCCESS 0
#define DEVICE_NAME "UNGS"
#define BUF_LEN 80

static int Major;
static int Device_Open = 0;
static char msg[BUF_LEN];
//static int msg_length = 0;
static char *msg_Ptr;

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};


/*
 * This function is called when the module is loaded
 */
int init_module(void){
    Major = register_chrdev(0, DEVICE_NAME, &fops);

    if(Major < 0){
        printk(KERN_ALERT "Registrando char device con %d\n", Major);
        return Major;
    }

    printk(KERN_INFO "Tengo major number %d.Hablarle al driver ", Major);
    printk(KERN_INFO ", crear un dev_file con \n");
    printk(KERN_INFO "sudo rm /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, Major);
    printk(KERN_INFO "sudo chmod 666 /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "Probá varios minor numbers. Probar cat y echo\n");
    printk(KERN_INFO "al device file.\n");
    printk(KERN_INFO "Eliminar el /dev y el modulo al termina.\n");

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void){
    /*
     * Unregister the device
     */
    unregister_chrdev(Major, DEVICE_NAME);
    printk(KERN_INFO "UNGS : Driver desregistrado\n");
}

/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/chardev
 */
static int device_open(struct inode *inode, struct file *file){
    static int counter = 0;

    if(Device_Open){
        return -EBUSY;
    }
    Device_Open++;
    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    msg_Ptr = msg;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file){
    Device_Open--;

    module_put(THIS_MODULE);

    return SUCCESS;
}

/*
 * Called when a process, which already opened the dev file, attempts to read
 * from it.
 */
static ssize_t device_read(struct file *filp,  char *buffer, size_t length, loff_t *offset){
    // see include/linux/fs.h //length of the buffer //buffer to fill with data
    
    static int finished = 0;

    if(finished){
      printk(KERN_INFO "procfs_read: END\n");
      finished = 0;
      return 0;
    }
    finished = 1;
    
   // Elimina cualquier caracter de nueva linea al final de la cadena
    device_buffer[strcspn(device_buffer, "\n")] = '\0';
 
    char temp[procfs_buffer_size + 1];
    int i;
 
    for(i = 0; i < procfs_buffer_size; i++){
      temp[i] = device_buffer[procfs_buffer_size - i - 1];
    }
 
    temp[procfs_buffer_size] = '\n';
    /* 
    * We use put_to_user to copy the string from the kernel's
    * memory segment to the memory segment of the process
    * that called us. get_from_user, BTW, is
    * used for the reverse. 
    */
    if(copy_to_user(buffer, temp, procfs_buffer_size + 1)){
      return -EFAULT;
    }
    printk(KERN_INFO "procfs_read: read %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size + 1; /* Return the number of bytes "read" */
    
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/UNGS
 */
static ssize_t device_write(struct file *filp, size_t len, const char *buff, loff_t *offset){
    if(len > PROCFS_MAX_SIZE){ 
        procfs_buffer_size = PROCFS_MAX_SIZE;
    }
    else{
        procfs_buffer_size = len;
    }
    if(copy_from_user(device_buffer, buff, procfs_buffer_size) ){
      return -EFAULT;
    }
    printk(KERN_INFO "procfs_write: write %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size;
}