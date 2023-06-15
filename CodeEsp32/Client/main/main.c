#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "i2c_lib.h"
#include "hdc1080.h"

#include "cJSON.h"


#define EXAMPLE_ESP_WIFI_SSID      "1111"
#define EXAMPLE_ESP_WIFI_PASS      "01245678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define HOST_IP_ADDR "192.168.137.154"

#define PORT 5000

#define BUTTON_PIN      ( 0 )
#define LED_PIN         ( 2 )

#define BUFFER_SIZE 1024

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static hdc1080_sensor_t* sensor;

int sock;

void gpios_setup( void )
{
    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level( LED_PIN, 0 ); 
}
void socket_receive_task(void *pvParameters)
{
    char rx_buffer[BUFFER_SIZE];

    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            printf("Error occurred during receiving data!\n");
            break;
        } else if (len == 0) {
            printf("Connection closed by the server!\n");
            break;
        } else {
            rx_buffer[len] = '\0';
            printf("Received data: %s\n", rx_buffer);
        }

        memset(rx_buffer, 0, sizeof(rx_buffer));
    }
    vTaskDelete(NULL);
}
void sensor_reader_task(void *pvParameters)
{   
    float temperature;
    float humidity;
    /* init i2c */
    i2c_init(I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ);
    /* init sensor */
    sensor = hdc1080_init_sensor(I2C_BUS, HDC1080_ADDR);

    if (sensor)
    {
        hdc1080_registers_t registers = hdc1080_get_registers(sensor);
        ESP_LOGI(TAG, "Initialized HDC1080 sensor");
        registers.acquisition_mode = 1;
        
        /*Set mode for sensor*/
        hdc1080_set_registers(sensor, registers);
    }
    else
        printf("Could not initialize HDC1080 sensor\n");

    while (1)
    {   

        gpio_set_level(LED_PIN, 0);
        if (gpio_get_level(BUTTON_PIN) == 0)
        {   
            /* read sensor and send it to Gateway */
            if (hdc1080_read(sensor, &temperature, &humidity))
            {
                ESP_LOGI(TAG, "Send data sensor: temperature %.2f oC, humidity %.2f.", temperature, humidity);
                cJSON *root;
                root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "topic", "temp-hum");
                cJSON_AddNumberToObject(root, "Temperature", temperature);
                cJSON_AddNumberToObject(root, "Humidity", humidity);
                char *data = cJSON_Print(root);
                /* send data via socket */
                int err = send(sock, data, strlen(data), 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    if (errno == 128)
                    esp_restart();
                }
            }
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
    vTaskDelete(NULL);
}
void test_sensor_reader_task(void *pvParameters)
{  
    while (1)
    {   
            cJSON *root;
            root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "topic", "temp-hum");
            cJSON_AddNumberToObject(root, "temperature", rand() % 100 + 1);
            cJSON_AddNumberToObject(root, "humidity", rand() % 100 + 1);
            // cJSON_AddNumberToObject(root, "smoke", rand() % 100 + 1);
            char *data = cJSON_Print(root);
            /* send data via socket */
            int err = send(sock, data, strlen(data), 0);
            ESP_LOGW(TAG, "Sent data!");
            if (err < 0) 
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                if (errno == 128)
                esp_restart();
            }
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
void test_smoke_reader_task(void *pvParameters)
{  
    vTaskDelay(300 / portTICK_PERIOD_MS);
    while (1)
    {   
            cJSON *root;
            root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "topic", "smoke");
            cJSON_AddNumberToObject(root, "smoke", rand() % 100 + 1);
            char *data = cJSON_Print(root);
            /* send data via socket */
            int err = send(sock, data, strlen(data), 0);
            ESP_LOGW(TAG, "Sent data!");
            if (err < 0) 
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                if (errno == 128)
                esp_restart();
            }
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void socket_create_connect(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    /* init socket client */
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", HOST_IP_ADDR, PORT);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Successfully connected");

    xTaskCreate(socket_receive_task, "sensor_reader", 4096, NULL, 5, NULL);  
    xTaskCreate(test_sensor_reader_task, "sensor_reader", 4096, NULL, 5, NULL);  
    // xTaskCreate(test_smoke_reader_task, "sensor_reader", 4096, NULL, 5, NULL); 
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void app_main(void)
{
   /*  Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    /*setup gpio pin */
    gpios_setup();
    /* init wifi */
    wifi_init_sta();
    /* create read sensor task */
    socket_create_connect();
    
}
