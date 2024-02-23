/*
  gob_esp_now specific output
*/
#ifndef FOR_GOB_ESP_NOW_HPP
#define FOR_GOB_ESP_NOW_HPP

#include <cstdint>
#include <cstddef>

bool is_gob_esp_now_packet(const uint8_t* payload, const size_t length);
void output_gob_esp_now_payload(const uint8_t* payload);

#endif
