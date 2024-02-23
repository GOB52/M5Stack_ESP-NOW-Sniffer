/*
  M5Stack ESP-NOW sniffer, serial output of ESP-NOW packets
 */
#include <M5Unified.h>
#include <esp_wifi.h>
#include <gob_unifiedButton.hpp> // For CoreS3
#include <freertos/FreeRTOS.h>
#include <cstring>
#include "for_gob_esp_now.hpp"

// Settings (See also build_flags in platformio.ini)
#ifndef COUNTRY
#pragma message "COUNTRY is not defined. set to JP"
# define COUNTRY "JP"
#endif
#ifndef CHANNEL_MIN
#pragma message "CHANNEL_MIN is not defined. set to 1"
# define CHANNEL_MIN (1)
#endif
#ifndef CHANNEL_MAX
#pragma message "CHANNEL_MAX is not defined. set to 13"
# define CHANNEL_MAX (13)
#endif
#ifndef QUEUE_SIZE
# define QUEUE_SIZE (1024 * 16)
#endif

namespace
{
// See also https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html#frame-format
struct esp_now_mac_header_t
{
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr0[6]; // destination addr
    uint8_t addr1[6]; // source addr
    uint8_t addr2[6]; // always ff:ff:ff:ff:ff:ff
    uint16_t sequence_ctrl;
} __attribute__((__packed__)); // 24 bytes

struct esp_now_vendor_content_t
{
    uint8_t element_id; // 221D / DDH
    uint8_t length;
    uint8_t organization_id[3]; // 0x18fe34
    uint8_t type; // 4D / 4H
    uint8_t version; // ESP-NOW version
    uint8_t body[1]; // [0~250] payload by esp_now_send

    static constexpr size_t minimum_length = 7;
    inline uint8_t bodySize() const { return length - 5; }

} __attribute__((__packed__));

struct esp_now_frame_t
{
    esp_now_mac_header_t mac_header;
    uint8_t category_code; // 127D / 7FH
    uint8_t organization_id[3]; // 0x18fe34
    uint32_t random_values;
    esp_now_vendor_content_t vendor_content; // (7-257)
    // uint32_t fcs; // frame check sequence
    
    static constexpr size_t minimum_length = 24 + 1 + 3 + 4 + 7 + 4;
    static constexpr size_t maximum_length = 24 + 1 + 3 + 4 + 257 + 4;
} __attribute__((__packed__));;

auto& lcd = M5.Display;
goblib::UnifiedButton unifiedButton;

constexpr uint8_t channel_min{CHANNEL_MIN}, channel_max{CHANNEL_MAX};
constexpr wifi_country_t wc =
{
    .cc = COUNTRY,
    .schan = channel_min,
    .nchan = channel_max - channel_min + 1,
    .max_tx_power = 0, // maximum
    .policy = WIFI_COUNTRY_POLICY_AUTO
};

uint8_t channel{channel_min}; // Current sniffing channel
bool sniff{true}, failed{true};
QueueHandle_t queue;
uint64_t hcount{}, ocount{};

bool is_esp_now_packet(const wifi_promiscuous_pkt_t* pkt)
{
    if(pkt->rx_ctrl.sig_len >= esp_now_frame_t::minimum_length)
    {
        auto enf = (const esp_now_frame_t*)pkt->payload;
        return enf->category_code == 0x7F
        && enf->organization_id[0] == 0x18
        && enf->organization_id[1] == 0xfe
        && enf->organization_id[2] == 0x34
        && enf->vendor_content.element_id == 0xDD
        && enf->vendor_content.organization_id[0] == 0x18
        && enf->vendor_content.organization_id[1] == 0xfe
        && enf->vendor_content.organization_id[2] == 0x34
        && enf->vendor_content.type == 4
        ;
    }
    return false;
}

// No serial output here to avoid WDT since it runs on Core0
void packet_handler(void* buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if(!is_esp_now_packet(pkt)) { return; }

    // Variable length, so instead of sending a copy via xSendQueue, store the copy in a variable
    uint8_t* p{};
    while((p = (uint8_t*)malloc(pkt->rx_ctrl.sig_len)) == nullptr)
    {
        delay(1); // Retry allocation...
    }
    std::memcpy(p, pkt->payload, pkt->rx_ctrl.sig_len);

    while(xQueueSendToBack(queue, &p, 0) != pdTRUE)
    {
        delay(1); // Retry sending...
    }
    ++hcount;
}
//
}

void dump(const uint8_t* buf, const size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        M5.Log.printf("%02x%c", buf[i], ((i + 1) % 16) ? ' ' : '\n');
    }
    M5.Log.println();
}

// Must be process in Core1
void output_task(void*)
{
    for(;;)
    {
        uint8_t* p{};
        xQueueReceive(queue, &p, portMAX_DELAY);
        if(!p) { continue; }
        
        auto enf = (const esp_now_frame_t*)p;
        auto dst = enf->mac_header.addr0;
        auto src = enf->mac_header.addr1;
        ++ocount;

        if(is_gob_esp_now_packet(enf->vendor_content.body, enf->vendor_content.bodySize()))
        {
            M5.Log.printf(
                "[%20llu]\n\r"
                "src : %02x:%02x:%02x:%02x:%02x:%02x\n\r"
                "dest: %02x:%02x:%02x:%02x:%02x:%02x\n\r"
                ,
                ocount,
                src[0], src[1], src[2], src[3], src[4], src[5],
                dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]
                          );
                output_gob_esp_now_payload(enf->vendor_content.body);
        }
        else
        {
            M5.Log.printf(
                "[%20llu]\n\r"
                "src : %02x:%02x:%02x:%02x:%02x:%02x\n\r"
                "dest: %02x:%02x:%02x:%02x:%02x:%02x\n\r"
                "payload: %u\n\r"
                ,
                ocount,
                src[0], src[1], src[2], src[3], src[4], src[5],
                dst[0], dst[1], dst[2], dst[3], dst[4], dst[5],
                enf->vendor_content.bodySize()
                          );
            dump(enf->vendor_content.body, enf->vendor_content.bodySize());
        }
        free(p);
    }
}

void setup()
{
    M5.begin();
    unifiedButton.begin(&lcd);
    lcd.clear(TFT_DARKGRAY);

    //
    esp_err_t res{};
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if((res = esp_wifi_init(&cfg)) != ESP_OK) { M5_LOGE("Failed to init. 0X%x", res);  return; }
    if((res = esp_wifi_set_country(&wc)) != ESP_OK) { M5_LOGE("Failed to set_country. 0X%x", res);  return; }
    esp_wifi_set_promiscuous(sniff);
    esp_wifi_set_promiscuous_rx_cb(packet_handler);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    queue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t*));
    if(!queue) { M5_LOGE("Failed to create queue"); return; }
    xTaskCreateUniversal(output_task, "output", 1024 * 8, nullptr, 1 /*priority */, nullptr, 1 /* core */);

    //
    lcd.setFont(&fonts::Font4);
    unifiedButton.setFont(&fonts::Font4);
    lcd.clear(TFT_DARKGREEN);
    failed = false;
    M5_LOGI("Settings: (%s) %d - %d", wc.cc, channel_min, channel_max);
}

void loop()
{
    if(failed) { return; }

    M5.update();
    unifiedButton.update();
    
    //Change channel
    static uint8_t prev{channel};
    if(M5.BtnA.wasClicked()) { --channel; }
    if(M5.BtnC.wasClicked()) { ++channel; }
    if(prev != channel)
    {
        if(channel < channel_min) { channel = channel_max; }
        if(channel > channel_max) { channel = channel_min; }
        prev = channel;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
    // Sniff ON/OFF
    if(M5.BtnB.wasClicked())
    {
        sniff = !sniff;
        esp_wifi_set_promiscuous(sniff);
        lcd.clear(sniff ? TFT_DARKGREEN : TFT_BLACK);
    }

    //
    lcd.setCursor(0,0);
    lcd.printf("%s\n", sniff ? "Sniffing" : "Idle");
    lcd.printf("Channel:%02d\n\n", channel);
    lcd.printf("Heap:%07u\n", esp_get_free_heap_size());
    lcd.printf("Captured:%09llu\n", hcount);
    lcd.printf("Awaiting:%09llu\n", hcount - ocount);
    unifiedButton.draw();
    delay(1);
}
