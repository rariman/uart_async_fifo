#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_async_fifo, LOG_LEVEL_DBG);

/* FIFO item */
struct uart_item {
    void *fifo_reserved;
    size_t len;
    char data[];
};

K_FIFO_DEFINE(uart_fifo);

/* UART setup */
#define UART_NODE DT_ALIAS(work_uart)
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);

/* Botão */
#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

/* LED */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led_0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* Buffers UART */
#define RX_BUF_SIZE 64
static uint8_t rx_dma_buf[RX_BUF_SIZE];
static uint8_t rx_line_buf[RX_BUF_SIZE];
static size_t rx_pos = 0;

/* Dump FIFO ao pressionar botão */
static void button_pressed_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    LOG_INF("Button pressed - dumping FIFO");

    gpio_pin_set_dt(&led_1, 1);
    struct uart_item *item;
    while ((item = k_fifo_get(&uart_fifo, K_NO_WAIT)) != NULL) {
        LOG_DBG("Got item from FIFO @ %p: %.*s", item, (int)item->len, item->data);
        k_free(item);
    }
    gpio_pin_set_dt(&led_1, 0);
}

/* Callback UART */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type) {
    case UART_RX_RDY:
        // LOG_DBG("UART RX ready: %d bytes", evt->data.rx.len);
        // LOG_DBG("Received data: %.*s", (int)evt->data.rx.len, &evt->data.rx.buf[evt->data.rx.offset]);

        gpio_pin_set_dt(&led_0, 1);
        for (size_t i = 0; i < evt->data.rx.len; i++) {
            char c = evt->data.rx.buf[evt->data.rx.offset + i];

            if (c == '\n' || c == '\r') {
                // LOG_DBG("End of line detected, processing buffer");

                struct uart_item *item = k_malloc(sizeof(struct uart_item) + rx_pos + 1);
                if (!item) {
                    LOG_ERR("Allocation failed");
                    rx_pos = 0;
                    break;
                }

                item->len = rx_pos;
                memcpy(item->data, rx_line_buf, rx_pos);
                item->data[rx_pos] = '\0';
                LOG_DBG("Putting item into FIFO @ %p: %.*s", item, (int)item->len, item->data);
                k_fifo_put(&uart_fifo, item);

                rx_pos = 0;
                uart_rx_disable(uart_dev);
            } else if (rx_pos < RX_BUF_SIZE - 1) {
                rx_line_buf[rx_pos++] = c;
            }
        }
        gpio_pin_set_dt(&led_0, 0);
        break;

    case UART_RX_DISABLED:
        uart_rx_enable(uart_dev, rx_dma_buf, sizeof(rx_dma_buf), 50);
        break;

    default:
        break;
    }
}

int main(void)
{
    int ret;

    LOG_INF("UART async FIFO sample");

    if (!device_is_ready(uart_dev) || !device_is_ready(button.port) || !device_is_ready(led_0.port) || !device_is_ready(led_1.port)) {
        LOG_ERR("Devices not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led_0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED 0");
        return ret;
    }

    ret = gpio_pin_configure_dt(&led_1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED 1");
        return ret;
    }

    /* Configura botão */
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button");
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt");
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    /* Configura UART */
    ret = uart_callback_set(uart_dev, uart_cb, NULL);
    if (ret) {
        LOG_ERR("Failed to set UART callback");
        return ret;
    }

    ret = uart_rx_enable(uart_dev, rx_dma_buf, sizeof(rx_dma_buf), 50);
    if (ret) {
        LOG_ERR("Failed to enable UART RX");
        return ret;
    }

    return 0;
}