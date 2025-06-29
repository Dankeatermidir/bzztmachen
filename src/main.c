#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_wifi.h"
#include "mdns.h"
//default configuration for increased priority to make sure there will be no delay
//if causing problems try commenting out code below
#define HTTP_CONFIGURATION(){                           \
        .task_priority      = configMAX_PRIORITIES-5,       \
        .stack_size         = 4096,                     \
        .core_id            = tskNO_AFFINITY,           \
        .task_caps          = (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),       \
        .server_port        = 80,                       \
        .ctrl_port          = ESP_HTTPD_DEF_CTRL_PORT,  \
        .max_open_sockets   = 6,                        \
        .max_uri_handlers   = 5,                        \
        .max_resp_headers   = 8,                        \
        .backlog_conn       = 5,                        \
        .lru_purge_enable   = false,                    \
        .recv_wait_timeout  = 5,                        \
        .send_wait_timeout  = 5,                        \
        .global_user_ctx = NULL,                        \
        .global_user_ctx_free_fn = NULL,                \
        .global_transport_ctx = NULL,                   \
        .global_transport_ctx_free_fn = NULL,           \
        .enable_so_linger = false,                      \
        .linger_timeout = 0,                            \
        .keep_alive_enable = false,                     \
        .keep_alive_idle = 0,                           \
        .keep_alive_interval = 0,                       \
        .keep_alive_count = 0,                          \
        .open_fn = NULL,                                \
        .close_fn = NULL,                               \
        .uri_match_fn = NULL                            \
}

#define configTIMER_TASK_PRIORITY 19


//############# Here custom configuration begins ############################
// Adjust values if needed

#define DEVICE_PASSWORD "imgonnagetzapped" // proof of possesion needed during wifi provisioning
#define DEVICE_NAME "PROV_bzztmachen" // device name during provisioning, should begin with "PROV_"
#define MAX_PLAYERS 3 //Maximum number of HV outputs, when changing value other changes are necessary
#define SHOCK_TIME 8 // * 0.1s, might vary up to +100ms
#define DUTY_CYCLE 450 // / 1024, better don't increase if you like your transformers working
#define MAX_FREQ 1000
#define MIN_FREQ 50 //depends on tansformer used


#define RESET_PIN GPIO_NUM_10 //needed for reseting wifi provisioning, otherwise reflash
// to reset short to ground for ~1s

// flags are used to determine shock time left 
int8_t flags[MAX_PLAYERS] = {0,0,0}; // adjust zeros


// lists determine gpios, timers and channels used
// if using different board or changing MAX_PLAYERS, make sure they match wiring and mcu specification
gpio_num_t gpio_list[MAX_PLAYERS] = //gpios used for mosfet control
{
    GPIO_NUM_0,
    GPIO_NUM_1,
    GPIO_NUM_2
};

gpio_num_t led_list[MAX_PLAYERS] =  //gpios used for indication LEDs
{
    GPIO_NUM_3,
    GPIO_NUM_20,
    GPIO_NUM_21
};

ledc_timer_t timer_list[MAX_PLAYERS] = //timers used for PWM
{
    LEDC_TIMER_0,
    LEDC_TIMER_1,
    LEDC_TIMER_2
};

ledc_channel_t ch_list[MAX_PLAYERS] = //channels used for PWM
{
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2
};


// ###################### Here custom configuration ends #################################
//better don't change anything below

void flagger_callback(TimerHandle_t handle)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (flags[i]>0)
        {
            flags[i]--;
        }
        else
        {
            if (ledc_get_duty(LEDC_LOW_SPEED_MODE, ch_list[i]) != 0)
            {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_list[i], 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_list[i]);
                gpio_set_level(led_list[i], 0);
            }
        }
    }
}

void init_flagger()
{
    TimerHandle_t flagger = xTimerCreate("flagger", pdMS_TO_TICKS(100), pdTRUE, (void*)0, flagger_callback);
    xTimerStart(flagger,0);
}

void init_pins(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        gpio_reset_pin(gpio_list[i]);
        ledc_timer_config_t timer =
        {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = timer_list[i],
            .duty_resolution =LEDC_TIMER_10_BIT,
            .freq_hz = 50,
            .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_timer_config(&timer);
        ledc_channel_config_t channel = 
        {
            .gpio_num       = gpio_list[i],
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = ch_list[i],
            .timer_sel      = timer_list[i],
            .duty           = 0,  
            .hpoint         = 0
        };
        ledc_channel_config(&channel);
        gpio_reset_pin(led_list[i]);    
        gpio_set_direction(led_list[i], GPIO_MODE_OUTPUT);
        gpio_set_level(led_list[i],0);
    }
    gpio_config_t reset_pin_conf =
    {
        .pin_bit_mask = 1ULL << RESET_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&reset_pin_conf);
}


static esp_err_t reset_device() {
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_list[i], 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_list[i]);
        gpio_set_level(led_list[i], 0);
    }
    wifi_prov_mgr_reset_sm_state_for_reprovision();
    wifi_prov_mgr_deinit();
    esp_restart();
}

void reset_task(void *arg){
    while (1) {
        if (gpio_get_level(RESET_PIN) == 0){
            vTaskDelay (300/portTICK_PERIOD_MS);
            if (gpio_get_level(RESET_PIN) == 0)
            {
                reset_device();
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void init_reset_task(){
    xTaskCreate(reset_task,"reset",256,NULL,10,NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        esp_wifi_connect();
    }
 }

static void wifi_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
}

static void start_provisioning(void)
{
    wifi_prov_mgr_config_t prov_config =
    {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned)
    {
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        const char *pop = DEVICE_PASSWORD;
        const char *service_name = DEVICE_NAME;
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, NULL));
    }
    else
    {
        wifi_config_t wifi_config;
        ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
}

void start_mdns_service(void)
{
    esp_err_t err =mdns_init();
    if (err)
    {
        return;
    }
    mdns_hostname_set("bzztmachen");
    mdns_instance_name_set("BZZT MACHEN SERVER");
}

static esp_err_t machen_handler(httpd_req_t *req)
{
    char buf[24];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
    if (ret <= 0)
    {
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    uint8_t player = 0;
    uint32_t freq = 100;
    if (sscanf(buf, "%hhu,%lu", &player, &freq) == 2)
    {
        if (flags[player] < 1)
        {
            flags[player]=SHOCK_TIME;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, ch_list[player], DUTY_CYCLE);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, ch_list[player]);
            gpio_set_level(led_list[player], 1);
            freq = MIN(MAX(freq, MIN_FREQ), MAX_FREQ);
            ledc_set_freq(LEDC_LOW_SPEED_MODE, timer_list[player], freq);
            httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        else
        {
            httpd_resp_send(req, "TOO SOON", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
    }
    httpd_resp_send(req, "ERROR", HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
}

static httpd_handle_t start_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK)
    {
        static const httpd_uri_t bzzt_machen = {
            .uri       = "/machen",
            .method    = HTTP_POST,
            .handler   = machen_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &bzzt_machen);
    }
    return server;
}



void app_main(void)
{
    wifi_init();
    start_provisioning();
    start_mdns_service();
    start_server();
    init_pins();
    init_flagger();
    init_reset_task();
}