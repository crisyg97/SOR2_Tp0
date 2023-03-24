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

#define SUCCESS 0
#define DEVICE_NAME "UNGS"
#define BUF_LEN 80

static int Major;
static int Device_Open = 0;
static char msg[BUF_LEN];
static int msg_length = 0;

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
    int ret = unregister_chrdev(Major, DEVICE_NAME);
    if(ret < 0){
        printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
    }
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
static ssize_t device_read(struct file *file, // see include/linux/fs.h   
                           char *buffer,      //buffer to fill with data 
                           size_t length,     //length of the buffer 
                           loff_t *offset)
{
    int bytes_read = 0; //cantidad de bytes escritos en el buffer
    if (*msg_Ptr == 0){ //si se esta en el final del mensaje retorna 0 significando el final del archivo
        return 0;
    }                                           //Debido a que el búfer está en el segmento de datos del usuario,
    while (length && *msg_Ptr){                 //no el segmento de datos del kernel, la asignación no funcionaria
        put_user(*(msg_Ptr++), buffer++);       //Escribe un valor simple en el espacio del usuario.
        length--;
        bytes_read++;
    }

    return bytes_read; //Se supone que las funciones de lectura devuelven el número de bytes realmente insertados en el búfer
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/UNGS
 */
static ssize_t device_write(struct file *file, const char *tmp, size_t length, loff_t *offset){
    printk(KERN_INFO "Message writen to device: ");
	
    int i;
    for(i=0; i < length && i < BUF_LEN; i++){
        get_user(message[i], buffer + i) //copia una sola variable simple del espacio del usuario al espacio del kernel
    }

    msg_Ptr = Message;

    return length; //devolver la cantidad de caracteres de entrada utilizados
}