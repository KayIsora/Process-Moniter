#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include <time.h>
#include <pthread.h>
#include "../commom/data_structures.h"

// IPC communication types
typedef enum {
    IPC_MSG_COMMAND = 1,
    IPC_MSG_DATA = 2,
    IPC_MSG_STATUS = 3,
    IPC_MSG_ERROR = 4
} ipc_message_type_t;

// IPC message structure
typedef struct {
    ipc_message_type_t type;
    uint32_t length;
    char room_name[MAX_ROOM_NAME];
    char data[MAX_MESSAGE_SIZE];
    time_t timestamp;
} ipc_message_t;

// Function declarations
int ipc_init(void);
int ipc_send_command(const command_t *cmd);
int ipc_receive_data(monitor_data_t *data);
int ipc_read_kernel_data(const char *room_name, monitor_data_t *data);
int ipc_write_kernel_control(const char *room_name, int enable);
void ipc_cleanup(void);
int ipc_send_raw_message(const ipc_message_t *msg);
int ipc_receive_raw_message(ipc_message_t *msg);
int ipc_set_nonblocking(int enable);
int ipc_get_status(void);

#endif /* IPC_HANDLER_H */
