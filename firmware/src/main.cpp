// esp32-pump 動作確認サンプル（ポンプなし）
//   タクトスイッチを押すたびに、外付けLEDの点灯・消灯を切り替える。
//   切り替えたときはシリアルモニタ（115200bps）に状態を表示する。
#include <Arduino.h>
#include "config.h"

bool ledOn = false;

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

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // 起動時のスイッチの状態を確定状態にしておく（押したまま起動しても誤動作しない）
  buttonStable = buttonLastRaw = digitalRead(PIN_BUTTON);

  delay(1000); // USBシリアルがつながるのを少し待つ
  Serial.println();
  Serial.println("esp32-pump 動作確認サンプル（ポンプなし）");
  Serial.println("タクトスイッチを押すとLEDの点灯・消灯が切り替わる");
}

void loop() {
  if (buttonPressed(millis())) {
    ledOn = !ledOn;
    digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    Serial.println(ledOn ? "LED: 点灯" : "LED: 消灯");
  }
}
