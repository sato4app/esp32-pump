#pragma once

// ===== ピン割り当て（ESP32-C3 Super Mini） =====
// GPIO2/8/9 は起動モードを決めるピン、GPIO18/19 はUSBなので外部回路には使わない。
// ポンプ用のTB6612FNGは GPIO4/5/6/10 を使う予定なので空けておく。
#define PIN_BUTTON 3 // タクトスイッチ（もう片方はGND。内部プルアップを使うので抵抗は不要）
#define PIN_LED    7 // 外付けLED（330Ωを直列に入れてGNDへ）

#define BOARD_LED_PIN 8 // 基板上のLED。プログラムが動いている間ゆっくり点滅させる
// Super MiniのGPIO8のLEDはLOWで点灯する。点灯・消灯が逆なら HIGH と LOW を入れ替える。
#define BOARD_LED_ON_LEVEL  LOW
#define BOARD_LED_OFF_LEVEL HIGH

// ===== スイッチ =====
#define BUTTON_DEBOUNCE_MS 30 // この時間、状態が変わらなければ確定とみなす（チャタリング対策）

// ===== 動作表示 =====
#define HEARTBEAT_INTERVAL_MS 500 // 基板上のLEDの点滅間隔（この時間ごとに点灯・消灯を切り替える）
