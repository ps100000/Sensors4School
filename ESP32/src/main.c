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
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
struct mess{
    int data;
    int time;
};
int tasklock;
struct mess data[64][2];                       //Array für Messwerte (64 Stück)
/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define PORT 1234

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

static const char *TAG = "example";

static const adc1_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

uint32_t adc1_get_smooth(adc1_channel_t ch){
    int sum = 0;
    for (int i = 0; i < 16; i++){
        sum = sum + adc1_get_raw(ch);
    }
    int g = sum / 16;

    if (g <= 1024){
        return g * 1212121;                       //0.001
    }
    if (g <= 2048){
        return g * 1078787;                       //0.00089
    }
    if (g <= 3072){
        return g * 1030303;                       //0.00085
    }
    if (g <= 4095){
        return g * 969696;                        //0.0008
    }
    return -1;
}

int measurement(){ 
    gpio_set_level(13, 1);                           //Impuls an
    ets_delay_us(10);                                //warte 10us
    gpio_set_level(13, 0);                           //Impuls aus
    for(uint32_t i = 0; (i < 1024 * 1024) && (gpio_get_level(12) == 0); i++){                  //Überprüfe ob Echo Signal low
    }
    uint32_t start = xthal_get_ccount();             //Startzeit in start speichern
    while(gpio_get_level(12) == 1){                  //Überprüfe ob Echo Signal high
    }
    uint32_t end = xthal_get_ccount();               //endzeit in end speichern
    int distance = (end - start) / 10000;            //umrechnung in cm
    return distance;
} 

void rx_copy_to_buffer(uint32_t from, uint32_t pos, char rx_buffer[]){
    for(int p = 0; p < 4; p++){
        rx_buffer[pos + p] = from & 0xff;
        from = from >> 8;  
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
        xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");

        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
        ESP_LOGI(TAG, "IPv6: %s", ip6);
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
	wifi_config_t apConfig = {
	   .ap = {
	      .ssid="ESP32_TESTAP",
	      .ssid_len=0,
	      .password="",
	      .channel=0,
	      .authmode=WIFI_AUTH_OPEN,
	      .ssid_hidden=0,
	      .max_connection=4,
	      .beacon_interval=100
	   }
	};
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP, &apConfig) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[1052];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    uint32_t package_id = 0;

    while (1) {

        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket binded");
        ESP_LOGI(TAG, "Waiting for data");
        struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(sourceAddr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);
        // Error occured during receiving
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        }
        // Data received
        else {
            while (1 == 1) {   
                int changed = tasklock;
                while(changed == tasklock){
                    vTaskDelay(50 / portTICK_RATE_MS);
                }
                
                uint32_t timestamp = xthal_get_ccount();
                rx_copy_to_buffer(timestamp, 0, rx_buffer);
                package_id = package_id + 1;
                rx_copy_to_buffer(package_id, 4, rx_buffer);
                rx_copy_to_buffer(1, 8, rx_buffer);
                rx_copy_to_buffer(4 * (3 + 2 * 64), 12, rx_buffer);
                rx_copy_to_buffer(42, 16, rx_buffer);
                rx_copy_to_buffer(42, 20, rx_buffer);
                rx_copy_to_buffer(64, 24, rx_buffer);


                for(int i = 0; i < 64; i = i + 1){
                    rx_copy_to_buffer(data[i][tasklock].data, i * 8 + 28, rx_buffer);
                    rx_copy_to_buffer(data[i][tasklock].time, i * 8 + 32, rx_buffer);
                }   

                
                // Get the sender's ip address as string
                if (sourceAddr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (sourceAddr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
                len = 1052;
                int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                    break;
                }else{
                    ESP_LOGI(TAG, "send------------------------------------------------");
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }

    }
    vTaskDelete(NULL);
    
}

static void messure_task(void*pvParameters){
    tasklock = 0;           
    while(1!=0){                                         //Endlosschleife           
        int j;                                           //neue Variable für Verzögerung (j)
        data[0][!tasklock].data = measurement();                //erstes Feld mit Messwert füllen
        for(int i = 1; i < 64; i = i + 1){               //Messwerte 1-63
            data[i][!tasklock].data = measurement();            //Feld i mit Messwert füllen
            j = data[i][!tasklock].data - data[i - 1][!tasklock].data;     //Differenz vorheriger und jetziger Messwert          

            if (data[i][!tasklock].data > 400){
                data[i][!tasklock].data = 0;
            }
            

            if(j < 0){                                   //Betrag bilden 
                j = j * (-1);
            }
            j = 100 - j;
            if(j < 0){                                   //Verhinderung j < 0
                j = 0;
            }
            j /= 10;
            data[i][!tasklock].time = j;

            ESP_LOGI("-","%dcm",data[i][!tasklock].data);           //gibt momentane Messwert aus
            ESP_LOGI("-","%dms",j);                      //gibt momentane Wartezeit aus
            vTaskDelay(j / portTICK_PERIOD_MS);          //wartet Wert von j in MS
        }
        ESP_LOGI("-","XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        if (tasklock == 0){
            tasklock = 1;
        }else {
            tasklock = 0;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);           //wartet 5000 MS
    }
}


static void rgb_task(void*pvParameters){
    tasklock = 0;           
    while(1!=0){                                         //Endlosschleife           
        for(int i = 1; i < 21; i = i + 1){               //Messwerte 1-63
            gpio_set_level(14,1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
                data[i*3][tasklock].data = adc1_get_smooth(ADC_CHANNEL_6);
                gpio_set_level(14,0);
                gpio_set_level(27,1);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                data[i*3+1][tasklock].data = adc1_get_smooth(ADC_CHANNEL_6);
                gpio_set_level(27,0);
                gpio_set_level(26,1);
                gpio_set_level(25,1);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                data[i*3+2][tasklock].data = adc1_get_smooth(ADC_CHANNEL_6);
                gpio_set_level(26,0);
                gpio_set_level(25,0);
            ESP_LOGI("-","%d",data[i*3][tasklock].data);           //gibt momentane Messwert aus
        }
        if (tasklock == 0){
            tasklock = 1;
        }else {
            tasklock = 0;
        }
    }
}

void app_main()
{
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[13], PIN_FUNC_GPIO);//Pin 13 als GPIO setzen
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[12], PIN_FUNC_GPIO);//Pin 12 als GPIO setzen
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[14], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[27], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[26], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[25], PIN_FUNC_GPIO);
    
    gpio_set_direction(14,GPIO_MODE_OUTPUT);
    gpio_set_direction(27,GPIO_MODE_OUTPUT);
    gpio_set_direction(26,GPIO_MODE_OUTPUT);
    gpio_set_direction(25,GPIO_MODE_OUTPUT);
    gpio_set_direction(2,GPIO_MODE_OUTPUT);              //LED-Pin als Ausgang setzen
    gpio_set_direction(12,GPIO_MODE_INPUT);              //Pin 12 als Eingang setzen
    gpio_set_direction(13,GPIO_MODE_OUTPUT);             //Pin 13 als Ausgang setzen

    gpio_set_pull_mode(12,GPIO_PULLDOWN_ONLY);           //Pin 12 pulldown anschalten
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_11);
   
    gpio_set_level(2,0);                                 //LED aus
    gpio_set_level(14,0);
    gpio_set_level(27,0);
    gpio_set_level(26,0);
    gpio_set_level(25,0);


    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();

    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 4, NULL);
    xTaskCreate(messure_task,"messure_task",  4096, NULL, 4, NULL);
}
