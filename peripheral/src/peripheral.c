#include <zephyr/kernel.h>
#include "peripheral.h"

static struct {
    struct bt_gatt_cb gatt_callbacks;
    struct bt_conn_cb conn_callbacks;
    struct bt_conn *default_conn;
} self = {
        .gatt_callbacks = {
                .att_mtu_updated = ble_peripheral_mtu_updated,
        },
        .conn_callbacks = {
                .connected = ble_peripheral_connected,
                .disconnected = ble_peripheral_disconnected,
        },
        .default_conn = NULL,
};

static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS,(BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,BT_UUID_16_ENCODE(BLE_UART_UUID_SVC_VAL),),
};

BT_GATT_SERVICE_DEFINE(
        ble_uart_svc, BT_GATT_PRIMARY_SERVICE(BLE_UART_SVC_UUID),
        BT_GATT_CHARACTERISTIC(BLE_UART_NOTIFY_CHAR_UUID, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
        BT_GATT_CHARACTERISTIC(BLE_UART_WRITE_CHAR_UUID, BT_GATT_CHRC_WRITE,BT_GATT_PERM_WRITE, NULL, ble_peripheral_write_uart,NULL),
        BT_GATT_CCC(ble_peripheral_cfg_changed,(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),
        );

void ble_peripheral_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    ARG_UNUSED(attr);
    bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("NOTIFY %s.\n",(notify_enabled ? "enabled" : "disabled"));
}

int ble_peripheral_write_uart(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {

    int err = 0;
    char data[len + 1];

    memcpy(data, buf, len);
    data[len] = '\0';
    printk("Dado Recebido: %s.\n", data);

    for (int i = 0; i < len; i++) {
        if ((data[i] >= 'a') && ((data[i] <= 'z'))) {
            data[i] = 'A' + (data[i] - 'a');
        }
    }

    printk("Enviando Dado...\n");
    printk("%s.\n",data);

    err = bt_gatt_notify(NULL, &ble_uart_svc.attrs[1], data, len);
    if (err) {
        printk("ERRO AO NOTIFICAR.\n");
    }

    return 0;
}

void ble_peripheral_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
    printk("Updated MTU. TX:%d RX:%d bytes.\n", tx, rx);
}

void ble_peripheral_connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Houve falha na conexão. (err %u).\n", err);
    } else {
        self.default_conn = bt_conn_ref(conn);
        printk("Conectado.\n");
    }
}

void ble_peripheral_disconnected(struct bt_conn *conn, uint8_t reason) {

    int err = 0;
    printk("Desconectado, motivo: %u.\n", reason);

    if (self.default_conn) {
        bt_conn_unref(self.default_conn);
        self.default_conn = NULL;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Falha ao iniciar o Advertising. (err %d).\n", err);
    }
}

void ble_peripheral_ready(int init_err) {

    int err = 0;
    if (init_err) {
        printk("Falha na inicialização do Bluetooth  (err %d).\n", err);
        return;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Falha ao iniciar o Advertising. (err %d).\n", err);
        return;
    }

    printk("Advertising iniciado.\n");
}

int ble_peripheral_init() {

    int err = 0;
    bt_conn_cb_register(&self.conn_callbacks);
    bt_gatt_cb_register(&self.gatt_callbacks);

    err = bt_enable(ble_peripheral_ready);
    if (err) {
        printk("Falha na inicialização do Bluetooth  (err %d).\n", err);
        return err;
    }

    printk("Bluetooth inicializado.\n");
    return 0;
}