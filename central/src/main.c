#include "central.h"

int main() {
    int err = 1;

    err = ble_central_init();
    if (err) {
        printk("Erro ao iniciar o dispositivo Central!\n");
    }
    return 0;
}
