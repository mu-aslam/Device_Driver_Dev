#include <linux/module.h>
#include <linux/fs.h>
//#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "ioctl_const.h"

#define MAX_LENGTH length
#define SUCCESS 0

#define CHAR_DEV_NAME "CharDev_ioctl"
#define LICENSE_TYPE "GPL"
#define DRIVER_AUTHOR "Muhammad Aslam aslaam.shafee@gmail.com"
#define DRIVER_DESC   "First Module"

static char *char_device_buf;
struct cdev *tboost_cdev;
static unsigned int length = 4000;
dev_t mydev;
int count=1;
static struct class *tboost_class;

static int char_dev_open(struct inode *inode,
			    struct file  *file)
{
   static int counter = 0;
   counter++;
   printk(KERN_INFO "Number of times open() was called: %d\n", counter);
   printk(KERN_INFO "MAJOR number = %d, MINOR number = %d\n",imajor (inode),iminor(inode));
   printk(KERN_INFO "Process id of the current process: %d\n",current->pid );
   printk (KERN_INFO "ref=%d\n", module_refcount(THIS_MODULE));
   return SUCCESS;
}

static int char_dev_release(struct inode *inode,
		            struct file *file)
{
   return SUCCESS;
}

static ssize_t char_dev_read(struct file *file, 
		             char *buf,
			     size_t lbuf,
			     loff_t *ppos)
{
   int maxbytes; /* number of bytes from ppos to MAX_LENGTH */
   int bytes_to_do; /* number of bytes to read */
   int nbytes; /* number of bytes actually read */

   maxbytes = MAX_LENGTH - *ppos;
	
   if(maxbytes > lbuf) 
      bytes_to_do = lbuf;
   else 
      bytes_to_do = maxbytes;
	
   if(bytes_to_do == 0) {
      printk("Reached end of device\n");
      return -ENOSPC; /* Causes read() to return EOF */
   }
   nbytes = bytes_to_do - 
   copy_to_user( buf, /* to */
   char_device_buf + *ppos, /* from */
   bytes_to_do ); /* how many bytes */

   *ppos += nbytes;
   return nbytes;	
}

static ssize_t char_dev_write(struct file *file,
 		              const char *buf,
			      size_t lbuf,
			      loff_t *ppos)
{
   int nbytes; /* Number of bytes written */
   int bytes_to_do; /* Number of bytes to write */
   int maxbytes; /* Maximum number of bytes that can be written */

   maxbytes = MAX_LENGTH - *ppos;

   if(maxbytes > lbuf) 
      bytes_to_do = lbuf;
   else 
      bytes_to_do = maxbytes;

   if(bytes_to_do == 0) {
      printk("Reached end of device\n");
      return -ENOSPC; /* Returns EOF at write() */
   }

   nbytes = bytes_to_do -
   copy_from_user(char_device_buf + *ppos, /* to */
		  buf, /* from */
		  bytes_to_do ); /* how many bytes */
   *ppos += nbytes;
   return nbytes;
}


/*
				     big_kernel_lock()	
ioctl----> sys_ioctl---->do_ioctl ------------------->fops--->ioctl==> char_dev_ioctl 
                                       unlock()


*/
static long char_dev_ioctl(/*struct inode *inode,*/
                            struct file *filp,
                            unsigned int cmd,
                            unsigned long arg)
{
 /*	step 1: verify args */

  unsigned int i, size;
  char *new_buf;
  char c;
  int retbytes;
  if(_IOC_TYPE(cmd) != BOOST_MAGIC) return -ENOTTY;
  if(_IOC_NR(cmd) > BOOST_MAX_CMDS) return -ENOTTY;

  if(_IOC_DIR(cmd) & _IOC_READ)
     if(!access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)))
        return -EFAULT;
  if(_IOC_DIR(cmd) & _IOC_WRITE)
     if(!access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd)))
        return -EFAULT;

	/* implement support of commands using switch/case */

        /* sounds ok lets continue */
        switch(cmd) {

        case BOOST_FILL_ZERO:
                for(i=0; i<MAX_LENGTH; i++) char_device_buf[i] = 0;
		printk(KERN_DEBUG " FILL_ZERO config executed\n ");
                break;

        case BOOST_FILL_CHAR:
                retbytes = copy_from_user( &c, (char *)arg, sizeof(char) );
                for(i=0; i<MAX_LENGTH; i++) char_device_buf[i] = c;
		printk(KERN_DEBUG "FILL_CHAR config executed \n");
                break;

        case BOOST_SET_SIZE:
		 //if (!capable(CAP_SYS_ADMIN))
           //     	return -EPERM;

                retbytes=copy_from_user(&size, (unsigned int*)arg,
                                        sizeof(unsigned int));
                new_buf = (char *)kmalloc(size*sizeof(char),
                                          GFP_KERNEL);
                if(!new_buf) return -ENOSPC;
                kfree(char_device_buf);
                char_device_buf = (char *)new_buf;
                MAX_LENGTH = size;
                for(i=0; i<MAX_LENGTH; i++) char_device_buf[i] = 0;
                filp->f_pos = 0;
		printk(KERN_DEBUG "SET_SIZE config executed\n");
                break;
	
		case BOOST_GET_SIZE:
			//if (!capable(CAP_SYS_ADMIN))
							//return -EPERM;

			size = MAX_LENGTH;
					retbytes=copy_to_user((unsigned int*)arg, &size , sizeof(unsigned int));
			printk(KERN_DEBUG "GET_SIZE config executed \n");	
					break;
		case WRITE_VALUE:
				if( copy_from_user(char_device_buf ,(char*) arg, sizeof(char)*10) )
				{
						pr_err("Data Write : Err!\n");
				}
				pr_info("WRITE_VALUE is executed\n");
				break;
		case READ_VALUE:
				if( copy_to_user((char*) arg, char_device_buf, sizeof(char)*10) )
				{
						pr_err("Data Read : Err!\n");
				}
				pr_info("READ_VALUE is executed\n");
				break;
		default:
				pr_info("Default\n");
				break;				

        }

        return SUCCESS;
}	


static struct file_operations char_dev_fops = {
	.owner = THIS_MODULE,
	//.ioctl = char_dev_ioctl,
    .unlocked_ioctl = char_dev_ioctl,
	.read = char_dev_read,
	.write = char_dev_write,
	.open = char_dev_open,
	.release = char_dev_release,
};

static __init int char_dev_init(void)
{
	int ret;
	/*allocation of major/minor number*/
	if(alloc_chrdev_region (&mydev, 0, count, CHAR_DEV_NAME) < 0) {
           printk (KERN_ERR "failed to reserve major/minor range\n");
           return -1;
    }
		//creating cdev device 
        if(!(tboost_cdev = cdev_alloc ())) {
           printk (KERN_ERR "cdev_alloc() failed\n");
           unregister_chrdev_region (mydev, count);
           return -1;
 	}
	cdev_init(tboost_cdev,&char_dev_fops);
	// adding cdev to the system 
	ret=cdev_add(tboost_cdev,mydev,count);
	if(ret < 0 ) {
	   printk(KERN_INFO "Error registering device driver\n");
	   cdev_del (tboost_cdev);
           unregister_chrdev_region (mydev, count); 	
	   return -1;
	}
	// creating a class
	tboost_class = class_create (THIS_MODULE, "VIRTUAL");
	// creating a logical device
        device_create (tboost_class, NULL, mydev, NULL, "%s", CHAR_DEV_NAME);

	printk(KERN_INFO"\nDevice Registered: %s\n",CHAR_DEV_NAME);
	printk(KERN_INFO "Major number = %d, Minor number = %d\n",MAJOR(mydev),MINOR(mydev));

	char_device_buf =(char *)kmalloc(MAX_LENGTH,GFP_KERNEL);
	return 0;
}

static __exit void  char_dev_exit(void)
{
	 device_destroy (tboost_class, mydev);
         class_destroy (tboost_class);
	 cdev_del(tboost_cdev);
	 unregister_chrdev_region(mydev,1);
	 kfree(char_device_buf);
	 printk(KERN_INFO "\n Driver unregistered \n");
}
module_init(char_dev_init);
module_exit(char_dev_exit);

// Following macros are definded in linux/module.h
MODULE_LICENSE(LICENSE_TYPE); // It tell the license type since linux kernel is open source, we are definding our moduel as GPL
MODULE_AUTHOR(DRIVER_AUTHOR);// use "Name <email>" or just "Name"
MODULE_DESCRIPTION(DRIVER_DESC);// what is the function of module
MODULE_SUPPORTED_DEVICE("sample_module"); // will decribe in the next examples
