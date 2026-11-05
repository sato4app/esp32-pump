#pragma once

// ===== ピン割り当て（ESP32-C3 Super Mini） =====
// GPIO2/8/9 は起動モードを決めるピン、GPIO18/19 はUSBなので外部回路には使わない。
// ポンプ用のTB6612FNGは GPIO4/5/6/10 を使う予定なので空けておく。
#define PIN_BUTTON 3 // タクトスイッチ（もう片方はGND。内部プルアップを使うので抵抗は不要）
#define PIN_LED    7 // 外付けLED（330Ωを直列に入れてGNDへ）

// ===== スイッチ =====
#define BUTTON_DEBOUNCE_MS 30 // この時間、状態が変わらなければ確定とみなす（チャタリング対策）
