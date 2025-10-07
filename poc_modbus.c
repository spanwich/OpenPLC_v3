#include <modbus.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <ip> <port> <start register> <count> [write_value]\n",
            prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 5 && argc != 6) {
        usage(argv[0]);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int addr = atoi(argv[3]);
    int num  = atoi(argv[4]);
    int write_value = -1;

    if (argc == 6) {
        write_value = atoi(argv[5]);
    }

    if (num <= 0) {
        fprintf(stderr, "Count must be positive\n");
        return EXIT_FAILURE;
    }

    modbus_t *ctx = modbus_new_tcp(ip, port);
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create Modbus context: %s\n", modbus_strerror(errno));
        return EXIT_FAILURE;
    }

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return EXIT_FAILURE;
    }

    uint16_t *tab_reg = (uint16_t *)malloc((size_t)num * sizeof(uint16_t));
    if (tab_reg == NULL) {
        perror("malloc");
        modbus_close(ctx);
        modbus_free(ctx);
        return EXIT_FAILURE;
    }

    if (write_value >= 0) {
        int wrc = modbus_write_register(ctx, addr, (uint16_t)write_value);
        if (wrc == -1) {
            fprintf(stderr, "Write failed: %s\n", modbus_strerror(errno));
        } else {
            printf("Wrote %d to holding register %d\n", write_value, addr);
        }
    }

    int rc = modbus_read_registers(ctx, addr, num, tab_reg);
    if (rc == -1) {
        fprintf(stderr, "Read failed: %s\n", modbus_strerror(errno));
        free(tab_reg);
        modbus_close(ctx);
        modbus_free(ctx);
        return EXIT_FAILURE;
    }

    printf("Read %d holding registers starting at address %d:\n", rc, addr);
    for (int i = 0; i < rc; ++i) {
        printf("  [%d] = %u\n", addr + i, tab_reg[i]);
    }

    free(tab_reg);
    modbus_close(ctx);
    modbus_free(ctx);
    return EXIT_SUCCESS;
}
