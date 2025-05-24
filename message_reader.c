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
    char buffer[MAX_MSG_SIZE];
    int bytes_read;
    int result;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <message_slot_file> <channel_id>\n", argv[0]);
        exit(1);
    }
    
    channel_id = (unsigned int)atoi(argv[2]);

    if (channel_id == 0) {
        fprintf(stderr, "Channel ID must be a non-zero integer.\n");
        exit(1);
    }
    
    // Open message slot device file
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Failed to open message slot device");
        exit(1);
    }
    
    // Set the channel id
    result = ioctl(fd, MSG_SLOT_CHANNEL, &channel_id);
    if (result < 0) {
        perror("Failed to set channel id");
        close(fd);
        exit(1);
    }
    
    // Read message
    bytes_read = read(fd, buffer, MAX_MSG_SIZE);
    if (bytes_read < 0) {
        perror("Failed to read message");
        close(fd);
        exit(1);
    }
    
    close(fd);
    
    // Write message to stdout
    result = write(STDOUT_FILENO, buffer, bytes_read);
    if (result < 0 || result != bytes_read) {
        perror("Failed to write to stdout");
        exit(1);
    }
    
    exit(0);
}