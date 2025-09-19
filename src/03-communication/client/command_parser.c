#include "client.h"

command_type_t parse_command(int argc, char *argv[], command_t *cmd) {
    if (cmd == NULL || argc < 2) {
        return CMD_UNKNOWN;
    }
    
    // Initialize command structure
    memset(cmd, 0, sizeof(command_t));
    cmd->type = CMD_UNKNOWN;
    cmd->arg_count = argc - 2;  // Exclude program name and command
    
    // Parse command type
    if (strcmp(argv[1], "help") == 0) {
        cmd->type = CMD_HELP;
    } else if (strcmp(argv[1], "create") == 0) {
        cmd->type = CMD_CREATE;
        if (argc >= 4) {
            strncpy(cmd->room_name, argv[2], sizeof(cmd->room_name) - 1);
            cmd->room_size = atoi(argv[3]);
        }
    } else if (strcmp(argv[1], "start") == 0) {
        cmd->type = CMD_START;
        if (argc >= 3) {
            strncpy(cmd->room_name, argv[2], sizeof(cmd->room_name) - 1);
        }
    } else if (strcmp(argv[1], "stop") == 0) {
        cmd->type = CMD_STOP;
        if (argc >= 3) {
            strncpy(cmd->room_name, argv[2], sizeof(cmd->room_name) - 1);
        }
    } else if (strcmp(argv[1], "show") == 0) {
        cmd->type = CMD_SHOW;
        if (argc >= 3) {
            strncpy(cmd->room_name, argv[2], sizeof(cmd->room_name) - 1);
        }
    } else if (strcmp(argv[1], "exit") == 0) {
        cmd->type = CMD_EXIT;
    }
    
    // Copy additional arguments
    int i;
    for (i = 2; i < argc && i - 2 < MAX_ARGS; i++) {
        strncpy(cmd->args[i - 2], argv[i], sizeof(cmd->args[0]) - 1);
        cmd->args[i - 2][sizeof(cmd->args[0]) - 1] = '\0';
    }
    
    return cmd->type;
}

int validate_command(const command_t *cmd) {
    if (cmd == NULL) {
        return -1;
    }
    
    switch (cmd->type) {
        case CMD_CREATE:
            // Validate create command: need room name and size
            if (strlen(cmd->room_name) == 0) {
                fprintf(stderr, "Error: Room name is required for create command\n");
                return -1;
            }
            if (cmd->room_size <= 0) {
                fprintf(stderr, "Error: Room size must be a positive number\n");
                return -1;
            }
            if (cmd->room_size > 10000) {
                fprintf(stderr, "Error: Room size too large (max 10000)\n");
                return -1;
            }
            break;
            
        case CMD_START:
        case CMD_STOP:
		// These commands need a room name
            if (strlen(cmd->room_name) == 0) {
                fprintf(stderr, "Error: Room name is required\n");
                return -1;
            }
            break;
        case CMD_SHOW:
            
			break;
            
        case CMD_HELP:
        case CMD_EXIT:
            // These commands don't need additional validation
            break;
            
        case CMD_UNKNOWN:
        default:
            fprintf(stderr, "Error: Unknown command\n");
            return -1;
    }
    
    return 0;
}
