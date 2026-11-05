#pragma once

// ===== ピン割り当て（ESP32-C3 Super Mini） =====
// GPIO2/8/9 は起動モードを決めるピン、GPIO18/19 はUSBなので外部回路には使わない。
// ポンプは TB6612FNG のA側（AO1/AO2）につなぐ。
#define PIN_AIN1  4
#define PIN_AIN2  5
#define PIN_PWMA  6
#define PIN_STBY  10 // LOWでポンプを切り離す（3.3Vに直結しない）

#define PIN_BUTTON 3 // タクトスイッチ（もう片方はGND。内部プルアップを使うので抵抗は不要）
#define PIN_LED    7 // 外付けLED（330Ωを直列に入れてGNDへ）。ポンプ運転中に点灯

#define BOARD_LED_PIN 8 // 基板上のLED。動作中であることを示すためゆっくり点滅させる
// Super MiniのGPIO8のLEDはLOWで点灯する。点灯・消灯が逆なら HIGH と LOW を入れ替える。
#define BOARD_LED_ON_LEVEL  LOW
#define BOARD_LED_OFF_LEVEL HIGH

// ===== ポンプ =====
#define PUMP_PWM_FREQ_HZ 20000 // 可聴域より上にしてモーターの「キーン」音を消す
#define PUMP_PWM_BITS    10    // 0〜1023

// 運転時の出力（%）。ポンプの定格電圧より電源電圧が高いときは下げる。
// 例: 定格5Vのポンプを電源6Vで回すなら 5 / 6 ≒ 83% 以下にする。
#define PUMP_DEFAULT_PERCENT 100

// 停止→運転の出力を、この時間をかけて上げる（突入電流による電圧低下・リセットを防ぐ）
// CONTROL_INTERVAL_MS 以上の値にする
#define PUMP_RAMP_MS 500

// 運転を始めてからこの時間がたったら自動で止める（空回り・水のあふれ防止）。0で無効。
#define PUMP_MAX_RUN_MS 30000

// 配線の都合で逆回転になる（水を吸わない）場合は true にする（配線の入れ替えでも可）
#define PUMP_INVERT false

// ===== スイッチ =====
#define BUTTON_DEBOUNCE_MS 30 // この時間、状態が変わらなければ確定とみなす（チャタリング対策）

// ===== 制御 =====
#define CONTROL_INTERVAL_MS 10 // 制御ループの周期（100Hz）
