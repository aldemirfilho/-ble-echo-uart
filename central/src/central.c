#include "central.h"

static struct {
    struct bt_conn_cb conn_callbacks;
    struct bt_gatt_cb gatt_callbacks;
    struct bt_gatt_discover_params discover_params;
    struct bt_gatt_subscribe_params subscribe_params;
    struct bt_conn *default_conn;
    struct bt_uuid_16 uuid;
    uint16_t write_handle;
} self = {
        .conn_callbacks =
                {
                    .connected = ble_central_connected,
                    .disconnected = ble_central_disconnected,
                },
        .gatt_callbacks =
                {
                    .att_mtu_updated = ble_central_mtu_updated,
                },
        .discover_params = {0},
        .subscribe_params = {0},
        .default_conn = NULL,
        .uuid = BT_UUID_INIT_16(0),
        .write_handle = 0,
};

void ble_central_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
    printk("Updated MTU. TX:%d RX:%d bytes.\n", tx, rx);
}

bool ble_central_eir_found(struct bt_data *data, void *user_data) {
    bt_addr_le_t *addr = user_data;
    int i;

    printk("[AD]: %u data_len %u.\n", data->type, data->data_len);

    switch (data->type) {
        case BT_DATA_UUID16_SOME:
        case BT_DATA_UUID16_ALL:
            if (data->data_len % sizeof(uint16_t) != 0U) {
                printk("AD mal formado.\n");
                return true;
            }

            for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
                struct bt_le_conn_param *param;
                struct bt_uuid *uuid;
                uint16_t u16;
                int err;

                memcpy(&u16, &data->data[i], sizeof(u16));
                uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
                if (bt_uuid_cmp(uuid, BLE_UART_SVC_UUID)) {
                    continue;
                }

                err = bt_le_scan_stop();
                if (err) {
                    printk("Falha no scan (err %d).\n", err);
                    continue;
                }

                param = BT_LE_CONN_PARAM_DEFAULT;
                err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &self.default_conn);
                if (err) {
                    printk("Falha na criação da conexão (err %d).\n", err);
                    ble_central_start_scan();
                }
                return false;
            }
    }
    return true;
}

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad) {
    char dev[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, dev, sizeof(dev));
    printk("[Device]: %s, AD evt type %u, AD data len %u, RSSI %i.\n", dev, type, ad->len, rssi);

    if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        bt_data_parse(ad, ble_central_eir_found, (void *)addr);
    }
}

uint8_t ble_central_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *buf, uint16_t length) {

    if (!buf) {
        printk("[Unsubscribed]\n");
        params->value_handle = 0U;
        return BT_GATT_ITER_CONTINUE;
    }

    char data[length + 1];

    memcpy(data, buf, length);
    data[length] = '\0';

    printk("Dados recebidos como notificação: %s. Length %u.\n", data, length);
    return BT_GATT_ITER_CONTINUE;
}

uint8_t ble_central_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params) {
    int err;

    if (!attr) {
        printk("Discover complete.\n");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    printk("|Discover attribute handle: %u.\n", attr->handle);

    if (!bt_uuid_cmp(self.discover_params.uuid, BLE_UART_SVC_UUID)) {
        memcpy(&self.uuid, BLE_UART_NOTIFY_CHAR_UUID, sizeof(self.uuid));
        self.discover_params.uuid = &self.uuid.uuid;
        self.discover_params.start_handle = attr->handle + 1;
        self.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &self.discover_params);
        if (err) {
            printk("Discover failed (err %d).\n", err);
        }
    } else if (!bt_uuid_cmp(self.discover_params.uuid,
                            BLE_UART_NOTIFY_CHAR_UUID)) {
        memcpy(&self.uuid, BLE_UART_WRITE_CHAR_UUID, sizeof(self.uuid));
        self.discover_params.uuid = &self.uuid.uuid;
        self.discover_params.start_handle = attr->handle + 1;
        self.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
        self.subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &self.discover_params);
        if (err) {
            printk("Discover failed (err %d).\n", err);
        }
    } else if (!bt_uuid_cmp(self.discover_params.uuid, BLE_UART_WRITE_CHAR_UUID)) {
        memcpy(&self.uuid, BT_UUID_GATT_CCC, sizeof(self.uuid));
        self.discover_params.uuid = &self.uuid.uuid;
        self.discover_params.start_handle = attr->handle + 1;
        self.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        self.write_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &self.discover_params);
        if (err) {
            printk("Discover failed (err %d).\n", err);
        }
    } else {
        self.subscribe_params.notify = ble_central_notify;
        self.subscribe_params.value = BT_GATT_CCC_NOTIFY;
        self.subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &self.subscribe_params);
        if (err && err != -EALREADY) {
            printk("Subscribe failed (err %d).\n", err);
        } else {
            printk("[Subscribed]\n");
        }
        return BT_GATT_ITER_STOP;
    }
    return BT_GATT_ITER_STOP;
}

void ble_central_connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        printk("Falha ao conectar: %s (%u).\n", addr, conn_err);

        bt_conn_unref(self.default_conn);
        self.default_conn = NULL;

        ble_central_start_scan();
        return;
    }

    printk("[Connected]: %s.\n", addr);

    if (conn == self.default_conn) {
        memcpy(&self.uuid, BLE_UART_SVC_UUID, sizeof(self.uuid));
        self.discover_params.uuid = &self.uuid.uuid;
        self.discover_params.func = ble_central_discover_func;
        self.discover_params.start_handle = 0x0001;
        self.discover_params.end_handle = 0xffff;
        self.discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(self.default_conn, &self.discover_params);
        if (err){
            printk("Discover failed(err %d).\n", err);
            return;
        }
    }
}

void ble_central_disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Desconectado. (motivo: %u).\n", reason);

    if (self.default_conn) {
        bt_conn_unref(self.default_conn);
        self.default_conn = NULL;
    }

    ble_central_start_scan();
}

void ble_central_start_scan(void) {
    int err = 0;

    struct bt_le_scan_param scan_param = {
            .type = BT_LE_SCAN_TYPE_ACTIVE,
            .options = BT_LE_SCAN_OPT_NONE,
            .interval = BT_GAP_SCAN_FAST_INTERVAL,
            .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Falha ao iniciar o scan, (err %d).\n", err);
        return;
    }

    printk("Scan iniciado!\n");
}

void ble_central_search_for_peripherals(int err) {
    ble_central_start_scan();
}

int ble_central_write_input(uint8_t *buf, uint16_t buf_len) {
    int err = 0;

    if (self.default_conn == NULL) {
        printk("Not connected!");
        return -1;
    }

    err = bt_gatt_write_without_response(self.default_conn, self.write_handle,
                                         buf, buf_len, false);
    if (err) {
        printk("%s: Write cmd failed (%d).\n", __func__, err);
    }

    return 0;
}

void input_task(void) {
    int err = 0;
    char *line = NULL;

    console_getline_init();

    while (true) {

        k_sleep(K_MSEC(100));

        printk("Digite uma string:\n");
        printk(">\n");
        line = console_getline();

        if (line == NULL) {
            printk("entrada inválida!\n");
            continue;
        }

        printk("Enviando dado...\n");
        printk("%s\n", line);
        err = ble_central_write_input(line, strlen(line));
    }
}

int ble_central_init() {
    int err = 0;

    bt_conn_cb_register(&self.conn_callbacks);
    bt_gatt_cb_register(&self.gatt_callbacks);

    err = bt_enable(ble_central_search_for_peripherals);
    if (err) {
        printk("Falha ao iniciar o Bluetooth! (err %d).\n", err);
        return err;
    }
    printk("Bluetooth inicializado com sucesso!\n");
    return 0;
}
K_THREAD_DEFINE(input, 1024, input_task, NULL, NULL, NULL, 1, 0, 1000);
