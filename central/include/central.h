#ifndef CENTRAL_H
#define CENTRAL_H

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>

#include "stdint.h"
#include "stdlib.h"
#include "string.h"


#define BLE_UART_UUID_SVC_VAL 0x2BC4
#define BLE_UART_SVC_UUID BT_UUID_DECLARE_16(BLE_UART_UUID_SVC_VAL)
#define BLE_UART_NOTIFY_CHAR_UUID_VAL 0x2BC5
#define BLE_UART_NOTIFY_CHAR_UUID BT_UUID_DECLARE_16(BLE_UART_NOTIFY_CHAR_UUID_VAL)
#define BLE_UART_WRITE_CHAR_UUID_VAL 0x2BC6
#define BLE_UART_WRITE_CHAR_UUID BT_UUID_DECLARE_16(BLE_UART_WRITE_CHAR_UUID_VAL)

int ble_central_write_input(uint8_t *buf, uint16_t buf_len);
int ble_central_init(void);

//task de input
void input_task(void);

//callbacks
uint8_t ble_central_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *buf, uint16_t length);
uint8_t ble_central_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params);
void ble_central_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx);
void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);
void ble_central_connected(struct bt_conn *conn, uint8_t err);
void ble_central_disconnected(struct bt_conn *conn, uint8_t reason);
void ble_central_start_scan(void);
void ble_central_search_for_peripherals(int err);
bool ble_central_eir_found(struct bt_data *data, void *user_data);

#endif //CENTRAL_H//
