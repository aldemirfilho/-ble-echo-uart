//
// Created by tayco on 02/11/2022.
//
#include <zephyr/kernel.h>
#include "peripheral.h"

int main() {
    int err = 1;
    err = ble_peripheral_init();

    if (err) {
        printk("Erro ao inicializar o dispositivo Periférico!.\n");
    }
    return 0;
}