/*

TITLE	: 2_ESP32_Python_Socket_Comm
DATE	: 2019/12/15
AUTHOR	: e-Yantra Team

AIM: To create Wi-Fi hotspot and echo back data received from Python client via socket.

*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "driver/uart.h"
#include "driver/gpio.h"

#define PORT 3333 // Port address for socket communication
#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 1024

static const char *TAG = "ESP32";
static EventGroupHandle_t s_wifi_event_group;

#define ECHO_TEST_TXD (GPIO_NUM_32) // Connected to AVR Rx-0
#define ECHO_TEST_RXD (GPIO_NUM_33) // Connected to AVR Tx-0
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

/* Wi-Fi event handler */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/* Function to initialize Wi-Fi at station */
void wifi_init_softap() //my_wifi_config my_wifi
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "eYFi-Wireless-Serial",
            .password = "eyantra123",
            .ssid_len = 0,
            .channel = 6,
            .authmode = WIFI_AUTH_OPEN, //WIFI_AUTH_WPA_WPA2_PSK, //WIFI_AUTH_OPEN
            .ssid_hidden = 0,
            .max_connection = 4,
            .beacon_interval = 100},
    };

    printf(">>>>>>>> SSID: %s <<<<<<<<<\n", wifi_config.ap.ssid);
    printf(">>>>>>>> PASS: %s <<<<<<<<<\n", wifi_config.ap.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

int send_to_python(int sock, char tx_buffer[TX_BUFFER_SIZE])
{
    printf("\n Actually sending : %u ", strlen(tx_buffer));

    return send(sock, tx_buffer, strlen(tx_buffer), 0);
}

void app_main()
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_softap();

    char rx_buffer[RX_BUFFER_SIZE]; // buffer to store data from client
    char tx_buffer[TX_BUFFER_SIZE];
    char ipv4_addr_str[128];        // buffer to store IPv4 addresses as string
    char ipv4_addr_str_client[128]; // buffer to store IPv4 addresses as string
    int addr_family;
    int ip_protocol;

    char *some_addr;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    uint8_t *data_uart = (uint8_t *)malloc(BUF_SIZE);

    int len_uart = 0;
    unsigned int count = 0;
    char ack_buffer[100];

    while (1)
    {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        inet_ntop(AF_INET, &dest_addr.sin_addr, ipv4_addr_str, INET_ADDRSTRLEN);
        printf("[DEBUG] Self IP = %s\n", ipv4_addr_str);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0)
        {
            printf("[ERROR] Unable to create socket. ERROR %d\n", listen_sock);
            break;
        }
        printf("[DEBUG] Socket created\n");

        int flag = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            printf("[ERROR] Socket unable to bind. ERROR %d\n", err);
            break;
        }

        printf("[DEBUG] Socket bound, port %d\n", PORT);

        err = listen(listen_sock, 1);
        if (err != 0)
        {
            printf("[ERROR] Error occurred during listen. ERROR %d\n", err);
            break;
        }

        printf("[DEBUG] Socket listening\n");

        struct sockaddr_in6 source_addr; // Can store both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);

        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            printf("[ERROR] Error occurred during listen. ERROR %d\n", sock);
            break;
        }
        printf("[DEBUG] Socket accepted\n");
        int c = 0;
        while (1)
        {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);

            if (source_addr.sin6_family == PF_INET)
            {
                inet_ntop(AF_INET, &(source_addr.sin6_addr.s6_addr), ipv4_addr_str_client, INET_ADDRSTRLEN);
            }
            else if (source_addr.sin6_family == PF_INET6)
            {
                inet_ntop(AF_INET6, &(source_addr.sin6_addr.s6_addr), ipv4_addr_str_client, INET6_ADDRSTRLEN);
            }

            rx_buffer[len] = NULL; // Add NULL to the string

            if (len > 0)
            {
                int temp = uart_write_bytes(UART_NUM_1, (const char *)rx_buffer, len);
            }
        }

        if (sock != -1)
        {
            printf("[DEBUG] Shutting down socket and restarting...\n");
            shutdown(sock, 0);
            close(sock);

            shutdown(listen_sock, 0);
            close(listen_sock);
            vTaskDelay(5); // Required for FreeRTOS on ESP32
        }
    }
    return 0;
}
