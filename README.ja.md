# M5Stack_ESP-NOW-Sniffer

[English](README.md)

## 概要
ESP-NOW のパケットを、 M5Stack でスニッフィングしてシリアル出力します。

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

## コンパイル準備
PlatformIO 前提となっています。
```ini
; **** Required setting ****
; Appropriate values for your environment.
; COUNTRY 2-character string : Country code ("JP" as default)
; CHANNEL_MIN integer : Minimum Wi-Fi channel (1 as default)
; CHANNEL_MAX integer : Maximum Wi-Fi channel (13 as default)
build_flags = -Wall -Wextra -Wreturn-local-addr -Werror=format -Werror=return-local-addr
  -DCOUNTRY="\"JP\"" -DCHANNEL_MIN=1 -DCHANNEL_MAX=13
```
日本以外の環境では build_flags で適切な define 設定が必要です。

| Symbol      | Type          | Content            |
|---|---|---|
| COUNTRY     | 文字列(2文字) | 国コード           |
| CHANNEL_MIN | 数値          | 最小チャンネル番号 |
| CHANNEL_MAX | 数値          | 最大チャンネル番号 |


## 操作方法

| ボタン | 機能 |
|---|---|
|A|チャンネル番号減少|
|C|チャンネル番号増加|
|B|スニッフィング開始/停止(トグル)|

## 注意
- キャプチャするパケットが多いと出力が追い付かない為、リアルタイム性は高くありません。
- [こちらの資料](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html#frame-format)を
参考にしていますが、私の解釈が間違っているかもしれません。

