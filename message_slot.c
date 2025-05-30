#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/init.h>   
#include <linux/slab.h>     /* for kmalloc and kfree */

#include "message_slot.h"

MODULE_LICENSE("GPL");

typedef struct channel_struct {
    unsigned int id;                  // Channel ID
    char* message;                    // Message content
    unsigned int msg_size;            // Message size
    struct channel_struct* next;      // Pointer to next channel
} channel;

typedef struct {
    unsigned int minor;               // Minor number
    channel* channels_list_head;      // Pointer to first channel in the list
    unsigned int channel_count;       // Number of active channels
} message_slot;

// Array of message slots, indexed by minor number
// Since we're using register_chrdev, minor numbers are 0-255
message_slot* slots[256] = {NULL};

typedef struct {
    unsigned int channel_id;
    unsigned int is_censored;  // 0 = disabled, 1 = enabled
} file_data;

//================== HELPER FUNCTIONS ===========================

static channel* find_channel(message_slot* slot, unsigned int channel_id) {
    channel* current_channel = slot->channels_list_head;
    
    while (current_channel != NULL) {
        if (current_channel->id == channel_id) {
            return current_channel;
        }
        current_channel = current_channel->next;
    }
    
    return NULL;
}

static void apply_censorship(char* message, unsigned int size) {
    unsigned int i;
    for (i = 2; i < size; i += 3) {
        message[i] = '#';
    }
}

//================== DEVICE FUNCTIONS ===========================

static int device_open(struct inode *inode, struct file *file) {
    unsigned int minor = iminor(inode);
    file_data *fd_data;
    int new_slot = 0;
    
    // If the slot doesn't exist yet, create it
    if (slots[minor] == NULL) {
        new_slot = 1;
        slots[minor] = kmalloc(sizeof(message_slot), GFP_KERNEL);
        if (slots[minor] == NULL) {
            return -ENOMEM;
        }
        
        slots[minor]->minor = minor;
        slots[minor]->channel_count = 0;
        slots[minor]->channels_list_head = NULL;
    }
    
    // Allocate and initialize file's private data (for channel ID and censorship)
    fd_data = kmalloc(sizeof(file_data), GFP_KERNEL);
    if (fd_data == NULL) {
        // Clean up the slot if we just created it
        if (new_slot) {
            kfree(slots[minor]);
            slots[minor] = NULL;
        }
        return -ENOMEM;
    }
    
    fd_data->channel_id = 0;      
    fd_data->is_censored = 0;     
    
    file->private_data = fd_data;
    
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    if (file->private_data) {
        kfree(file->private_data);
        file->private_data = NULL;
    }
    return 0;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    file_data *fd_data = (file_data*)file->private_data;
    message_slot *slot;
    channel *chan;
    char *new_message;
    unsigned int minor = iminor(file_inode(file));
    
    // Check if private_data exists
    if (fd_data == NULL) {
        return -EINVAL;
    }
    
    // Check if channel ID is set
    if (fd_data->channel_id == 0) {
        return -EINVAL;
    }
    
    // Validate message length
    if (length == 0 || length > MAX_MSG_SIZE) {
        return -EMSGSIZE;
    }
    
    // Get the message slot for this file
    slot = slots[minor];
    if (slot == NULL) {
        return -EINVAL;
    }
    
    // Find the channel or create a new one
    chan = find_channel(slot, fd_data->channel_id);
    if (chan == NULL) {
        if (slot->channel_count >= (1 << 20)) {
            return -ENOMEM;
        }
        
        chan = kmalloc(sizeof(channel), GFP_KERNEL);
        if (chan == NULL) {
            return -ENOMEM;
        }
        
        chan->id = fd_data->channel_id;
        chan->message = NULL;
        chan->msg_size = 0;
        chan->next = slot->channels_list_head;
        slot->channels_list_head = chan;
        slot->channel_count++;
    }
    
    // Allocate space for the new message
    new_message = kmalloc(length, GFP_KERNEL);
    if (new_message == NULL) {
        return -ENOMEM;
    }
    
    // Copy the message from user space (we need to save the msg in kernel space)
    if (copy_from_user(new_message, buffer, length)) {
        kfree(new_message);
        return -EFAULT;
    }
    
    // Apply censorship if enabled
    if (fd_data->is_censored) {
        apply_censorship(new_message, length);
    }
    
    // Replace the old message
    if (chan->message != NULL) {
        kfree(chan->message);
    }
    
    chan->message = new_message;
    chan->msg_size = length;
    
    return length;
}


static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
    file_data *fd_data = (file_data*)file->private_data;
    message_slot *slot;
    channel *chan;
    unsigned int minor = iminor(file_inode(file));
    
    // Check if private_data exists
    if (fd_data == NULL) {
        return -EINVAL;
    }
    
    // Check if channel ID is set
    if (fd_data->channel_id == 0) {
        return -EINVAL;
    }
    
    // Get the message slot for this file
    slot = slots[minor];
    if (slot == NULL) {
        return -EINVAL;
    }
    
    // Find the channel
    chan = find_channel(slot, fd_data->channel_id);
    if (chan == NULL || chan->message == NULL || chan->msg_size == 0) {
        return -EWOULDBLOCK;
    }
    
    // Check if the buffer is large enough
    if (length < (chan->msg_size) ) {
        return -ENOSPC;
    }
    
    // Copy the message to user space
    if (copy_to_user(buffer, chan->message, chan->msg_size)) {
        return -EFAULT;
    }
    
    return chan->msg_size;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    file_data *fd_data = (file_data*)file->private_data;
    
    // Check if private_data exists
    if (fd_data == NULL) {
        return -EINVAL;
    }
    
    // Verify that cmd is valid
    if (cmd != MSG_SLOT_CHANNEL && cmd != MSG_SLOT_SET_CEN) {
        return -EINVAL;
    }
    
    if (cmd == MSG_SLOT_CHANNEL) {
        // Validate channel ID (must be non-zero)
        if (arg == 0) {
            return -EINVAL;
        }
        
        fd_data->channel_id = arg;
    } else if (cmd == MSG_SLOT_SET_CEN) {
        // Set censorship mode: 0 = disabled, non-zero = enabled
        fd_data->is_censored = (arg != 0) ? 1 : 0;
    }
    
    return 0;
}

static struct file_operations device_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,  
    .unlocked_ioctl = device_ioctl,
    .read = device_read,
    .write = device_write
};

static int __init message_slot_init(void) {
    int rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &device_fops);
    
    if (rc < 0) {
        printk(KERN_ERR "message_slot: registration failed with %d\n", rc);
        return rc;
    }
    
    printk(KERN_INFO "message_slot: module loaded with major number %d\n", MAJOR_NUM);
    return 0;
}

static void __exit message_slot_exit(void) {
    int i;
    message_slot *slot;
    channel *current_channel, *next;
    
    // Free all allocated memory
    for (i = 0; i < 256; ++i) {
        slot = slots[i];
        if (slot != NULL) {
            // Free all channels in the list
            current_channel = slot->channels_list_head;
            while (current_channel != NULL) {
                next = current_channel->next;  
                
                if (current_channel->message != NULL) {
                    kfree(current_channel->message);
                }
                
                kfree(current_channel);
                current_channel = next;
            }
            
            kfree(slot);
            slots[i] = NULL;
        }
    }
    
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    
    printk(KERN_INFO "message_slot: module unloaded\n");
}

module_init(message_slot_init);
module_exit(message_slot_exit);