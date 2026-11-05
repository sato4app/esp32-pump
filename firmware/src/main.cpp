// esp32-pump 動作確認サンプル
//   タクトスイッチを押すたびにポンプの運転・停止を切り替える。
//   外付けLEDはポンプ運転中に点灯し、基板上のLEDは動作中であることを示すためにゆっくり点滅する。
//   シリアルモニタ（115200bps）からも操作できる。
#include <Arduino.h>
#include "config.h"
#include "PumpDriver.h"

PumpDriver pump;

int pumpPercent = PUMP_DEFAULT_PERCENT; // 運転時の出力（P コマンドで変更できる）
uint32_t pumpStartMs = 0;               // 運転を始めた時刻（自動停止の判定に使う）
uint32_t lastControlMs = 0;
String serialLine; // シリアルモニタから入力中のコマンド

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

// ===== ポンプ操作 =====
void startPump(const char *reason) {
  pump.start(pumpPercent);
  pumpStartMs = millis();
  Serial.printf("ポンプ運転（出力 %d%%）: %s\n", pumpPercent, reason);
}

void stopPump(const char *reason) {
  pump.stop();
  Serial.printf("ポンプ停止: %s\n", reason);
}

void togglePump(const char *reason) {
  if (pump.running()) {
    stopPump(reason);
  } else {
    startPump(reason);
  }
}

// 運転時間が上限を超えたら止める（空回り・水のあふれ防止）
void checkMaxRunTime(uint32_t now) {
  if (PUMP_MAX_RUN_MS == 0 || !pump.running()) return;
  if (now - pumpStartMs >= PUMP_MAX_RUN_MS) {
    stopPump("運転時間の上限に達した");
  }
}

// ===== LED =====
void updateLeds(uint32_t now) {
  digitalWrite(PIN_LED, pump.running() ? HIGH : LOW);
  bool boardOn = (now / 500) % 2;
  digitalWrite(BOARD_LED_PIN, boardOn ? BOARD_LED_ON_LEVEL : BOARD_LED_OFF_LEVEL);
}

// ===== シリアルコマンド =====
//   ON         運転する
//   OFF        止める
//   P <0-100>  運転時の出力（%）を変える。運転中ならその出力へ徐々に変える
//   ?          状態を表示する
void printStatus() {
  Serial.printf("状態: %s / 出力 %d%%（設定 %d%%）/ スイッチ %s\n",
                pump.running() ? "運転中" : "停止中", pump.output(), pumpPercent,
                buttonStable == LOW ? "押下" : "開放");
  if (pump.running() && PUMP_MAX_RUN_MS > 0) {
    uint32_t elapsed = millis() - pumpStartMs;
    uint32_t remaining = elapsed < PUMP_MAX_RUN_MS ? PUMP_MAX_RUN_MS - elapsed : 0;
    Serial.printf("自動停止まで: %lu 秒\n", (unsigned long)(remaining / 1000));
  }
}

void handleSerialCommand(String command) {
  command.trim();
  command.toUpperCase();
  if (command.length() == 0) return;
  Serial.print("受信: ");
  Serial.println(command);

  if (command == "ON") {
    startPump("シリアルコマンド");
  } else if (command == "OFF") {
    stopPump("シリアルコマンド");
  } else if (command.startsWith("P ")) {
    int percent = -1;
    if (sscanf(command.c_str() + 2, "%d", &percent) != 1 || percent < 0 || percent > 100) {
      Serial.println("形式: P <出力 0〜100>");
      return;
    }
    pumpPercent = percent;
    Serial.printf("運転時の出力を %d%% にした\n", pumpPercent);
    if (pump.running()) {
      if (pumpPercent == 0) {
        stopPump("出力0%");
      } else {
        pump.start(pumpPercent); // 運転時間の計測はそのまま続ける
      }
    }
  } else if (command == "?") {
    printStatus();
  } else {
    Serial.println("コマンド: ON / OFF / P <0-100> / ?");
  }
}

void readSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      handleSerialCommand(serialLine);
      serialLine = "";
    } else if (serialLine.length() < 32) {
      serialLine += c;
    }
  }
}

void setup() {
  Serial.begin(115200);

  // 起動直後にポンプが一瞬でも回らないよう、最初にポンプを止めた状態にする
  pump.begin();

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(BOARD_LED_PIN, BOARD_LED_OFF_LEVEL);

  // 起動時のスイッチの状態を確定状態にしておく（押したまま起動しても誤動作しない）
  buttonStable = buttonLastRaw = digitalRead(PIN_BUTTON);

  delay(1000); // USBシリアルがつながるのを少し待つ
  Serial.println();
  Serial.println("esp32-pump 動作確認サンプル");
  Serial.println("タクトスイッチ: 運転/停止の切り替え");
  Serial.println("コマンド: ON / OFF / P <0-100> / ?");
}

void loop() {
  uint32_t now = millis();

  if (buttonPressed(now)) {
    togglePump("タクトスイッチ");
  }
  readSerial();

  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    pump.update();
    checkMaxRunTime(now);
  }

  updateLeds(now);
}
