#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)

// Set censorship mode
#define MSG_SLOT_SET_CEN _IOW(MAJOR_NUM, 1, unsigned int)

// Max message size
#define MAX_MSG_SIZE 128

#define DEVICE_NAME "message_slot"

#endif