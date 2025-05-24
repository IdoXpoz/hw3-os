#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "message_slot.h"

int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel_id;
    unsigned int censorship_mode;
    char *message;
    int message_len;
    int result;
    
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <message_slot_file> <channel_id> <censorship_mode> <message>\n", argv[0]);
        exit(1);
    }
    
    channel_id = (unsigned int)atoi(argv[2]);
    censorship_mode = (unsigned int)atoi(argv[3]);
    message = argv[4];
    message_len = strlen(message);
    
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Failed to open message slot device");
        exit(1);
    }
    
    // Set censorship mode
    result = ioctl(fd, MSG_SLOT_SET_CEN, &censorship_mode);
    if (result < 0) {
        perror("Failed to set censorship mode");
        close(fd);
        exit(1);
    }
    
    // Set channel id
    result = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);
    if (result < 0) {
        perror("Failed to set channel id");
        close(fd);
        exit(1);
    }
    
    // Write the message
    result = write(fd, message, message_len); // sending the message len removes the null char
    if (result < 0 || result != message_len) {
        perror("Failed to write message");
        close(fd);
        exit(1);
    }
    
    close(fd);
    exit(0);
}