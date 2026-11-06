// esp32-pump 動作確認サンプル（電源モジュールのテスト）
//   タクトスイッチを押すたびに、外付けLEDの点灯・消灯を切り替える。
//   電源モジュールから給電するとUSBのシリアルモニタが使えないため、状態をLEDで表す。
//     起動時: 外付けLEDが「ゆっくり3回」点滅 → 通常の起動
//             外付けLEDが「速く10回」点滅    → 電圧低下でリセットした（ブラウンアウト）
//     動作中: 基板上のLEDが0.5秒ごとに点滅（止まったらプログラムが固まっている）
//   USBでつないでいるときは、シリアルモニタ（115200bps）にも状態を表示する。
#include <Arduino.h>
#include <esp_system.h>
#include "config.h"

bool ledOn = false;
uint32_t lastHeartbeatMs = 0;
bool boardLedOn = false;

// ===== タクトスイッチ =====
// 内部プルアップを使うので、押していないときHIGH、押すとLOWになる。
bool buttonStable = HIGH;   // チャタリングを除いた確定状態
bool buttonLastRaw = HIGH;  // 前回読んだ生の状態
uint32_t buttonChangedMs = 0;

// 押された瞬間（HIGH→LOWが確定したとき）に1回だけ true を返す
bool buttonPressed(uint32_t now) {
  bool raw = digitalRead(PIN_BUTTON);
  if (raw != buttonLastRaw) {
    buttonLastRaw = raw;
    buttonChangedMs = now;
  }
  if (raw != buttonStable && now - buttonChangedMs >= BUTTON_DEBOUNCE_MS) {
    buttonStable = raw;
    return buttonStable == LOW;
  }
  return false;
}

// ===== 起動時の表示 =====
const char *resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:  return "電源投入";
    case ESP_RST_EXT:      return "外部リセット";
    case ESP_RST_SW:       return "ソフトウェアリセット";
    case ESP_RST_PANIC:    return "例外による再起動";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "ウォッチドッグによる再起動";
    case ESP_RST_DEEPSLEEP: return "ディープスリープからの復帰";
    case ESP_RST_BROWNOUT: return "電圧低下（ブラウンアウト）";
    default:               return "その他";
  }
}

void blinkLed(int times, uint32_t intervalMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(intervalMs);
    digitalWrite(PIN_LED, LOW);
    delay(intervalMs);
  }
}

// 起動した理由を外付けLEDの点滅で知らせる
void showResetReason(esp_reset_reason_t reason) {
  if (reason == ESP_RST_BROWNOUT) {
    blinkLed(10, 50);  // 速く10回: 電源の電圧が下がってリセットした
  } else {
    blinkLed(3, 200);  // ゆっくり3回: 通常の起動
  }
}

// ===== 動作中の表示 =====
// 基板上のLEDを一定間隔で点滅させる（ループが回っていることの確認）
void updateHeartbeat(uint32_t now) {
  if (now - lastHeartbeatMs < HEARTBEAT_INTERVAL_MS) return;
  lastHeartbeatMs = now;
  boardLedOn = !boardLedOn;
  digitalWrite(BOARD_LED_PIN, boardLedOn ? BOARD_LED_ON_LEVEL : BOARD_LED_OFF_LEVEL);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(BOARD_LED_PIN, BOARD_LED_OFF_LEVEL);

  esp_reset_reason_t reason = esp_reset_reason();
  showResetReason(reason);

  // 起動時のスイッチの状態を確定状態にしておく（押したまま起動しても誤動作しない）
  buttonStable = buttonLastRaw = digitalRead(PIN_BUTTON);

  delay(500); // USBシリアルがつながるのを少し待つ（点滅の時間と合わせて約1秒）
  Serial.println();
  Serial.println("esp32-pump 動作確認サンプル（電源モジュールのテスト）");
  Serial.printf("起動の理由: %s\n", resetReasonText(reason));
  Serial.println("タクトスイッチを押すとLEDの点灯・消灯が切り替わる");
}

void loop() {
  uint32_t now = millis();

  if (buttonPressed(now)) {
    ledOn = !ledOn;
    digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    Serial.println(ledOn ? "LED: 点灯" : "LED: 消灯");
  }

  updateHeartbeat(now);
}
