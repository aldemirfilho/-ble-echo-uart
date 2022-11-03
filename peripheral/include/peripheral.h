#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "stdint.h"
#include "stdlib.h"
#include "string.h"


#define BLE_UART_UUID_SVC_VAL 0x2BC4
#define BLE_UART_SVC_UUID BT_UUID_DECLARE_16(BLE_UART_UUID_SVC_VAL)
#define BLE_UART_NOTIFY_CHAR_UUID_VAL 0x2BC5
#define BLE_UART_NOTIFY_CHAR_UUID BT_UUID_DECLARE_16(BLE_UART_NOTIFY_CHAR_UUID_VAL)
#define BLE_UART_WRITE_CHAR_UUID_VAL 0x2BC6
#define BLE_UART_WRITE_CHAR_UUID BT_UUID_DECLARE_16(BLE_UART_WRITE_CHAR_UUID_VAL)

int ble_peripheral_init(void);
int ble_peripheral_write_uart(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

void ble_peripheral_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
void ble_peripheral_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx);
void ble_peripheral_connected(struct bt_conn *conn, uint8_t err);
void ble_peripheral_disconnected(struct bt_conn *conn, uint8_t reason);
void ble_peripheral_ready(int init_err);

#endif //PERIPHERAL_H//
