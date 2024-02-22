# M5Stack_ESP-NOW-Sniffer

[日本語](README.ja.md)


## Overview
ESP-NOW packets are sniffed by M5Stack and output to serial.

```log
00:56:23.162 > [                   1]
00:56:23.162 > src : f4:12:fa:84:8e:48
00:56:23.162 > dest: 30:c6:f7:1d:1c:cc
00:56:23.162 > payload: 11
00:56:23.162 > 00 52 2f 01 0b 00 06 48 01 00 73 
00:56:23.368 > [                   2]
00:56:23.368 > src : f4:12:fa:84:8e:48
00:56:23.368 > dest: 30:c6:f7:1d:1c:cc
00:56:23.368 > payload: 11
00:56:23.368 > 00 52 2f 01 0b 00 06 48 01 00 73 
00:56:23.575 > [                   3]
00:56:23.575 > src : f4:12:fa:84:8e:48
00:56:23.575 > dest: 30:c6:f7:1d:1c:cc
00:56:23.575 > payload: 11
00:56:23.575 > 00 52 2f 01 0b 00 06 48 01 00 73 
00:56:23.782 > [                   4]
00:56:23.782 > src : f4:12:fa:84:8e:48
00:56:23.782 > dest: 30:c6:f7:1d:1c:cc
00:56:23.782 > payload: 11
00:56:23.782 > 00 52 2f 01 0b 00 06 48 01 00 73 
```

## Compile environment
For PlatformIO.  

```ini
; **** Required setting ****
; Appropriate values for your environment.
; COUNTRY 2-character string : Country code ("JP" as default)
; CHANNEL_MIN integer : Minimum Wi-Fi channel (1 as default)
; CHANNEL_MAX integer : Maximum Wi-Fi channel (13 as default)
build_flags = -Wall -Wextra -Wreturn-local-addr -Werror=format -Werror=return-local-addr
  -DCOUNTRY="\"JP\"" -DCHANNEL_MIN=1 -DCHANNEL_MAX=13
```

In non-Japan environments, appropriate define settings are required in build_flags.

| Symbol      | Type          | Content            |
|---|---|---|
| COUNTRY     | String(2 characters) | Country code |
| CHANNEL_MIN | Integer          | Minimum channel No |
| CHANNEL_MAX | Integer          | Maximum channel No |

## Operation

| Button | Description |
|---|---|
|A|Decrease channel No|
|C|Increase channel No|
|B|Start / Stop sniffing (toggle) |

## Attention
- Real-time performance is not high because output cannot keep up with the large number of packets to be captured.
- I am referring to [this document](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html#frame-format), but my interpretation may be incorrect.



