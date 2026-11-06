// esp32-pump 動作確認サンプル（ポンプ制御）
//   起動時はポンプを止めた状態にし、タクトスイッチを押すたびにポンプの運転・停止を切り替える。
//   外付けLEDはポンプ運転中に点灯する。
//   電源モジュールから給電するとUSBのシリアルモニタが使えないため、状態をLEDで表す。
//     起動時: 外付けLEDが「ゆっくり3回」点滅 → 通常の起動
//             外付けLEDが「速く10回」点滅    → 電圧低下でリセットした（ブラウンアウト）
//     動作中: 基板上のLEDが0.5秒ごとに点滅（止まったらプログラムが固まっている）
//   USBでつないでいるときは、シリアルモニタ（115200bps）からも操作できる。
#include <Arduino.h>
#include <esp_system.h>
#include "config.h"
#include "PumpDriver.h"

PumpDriver pump;

int pumpPercent = PUMP_DEFAULT_PERCENT; // 運転時の出力（P コマンドで変更できる）
uint32_t pumpStartMs = 0;               // 運転を始めた時刻（自動停止の判定に使う）
uint32_t lastControlMs = 0;
uint32_t lastHeartbeatMs = 0;
bool boardLedOn = false;
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

// ===== 起動時の表示 =====
const char *resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "電源投入";
    case ESP_RST_EXT:       return "外部リセット";
    case ESP_RST_SW:        return "ソフトウェアリセット";
    case ESP_RST_PANIC:     return "例外による再起動";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "ウォッチドッグによる再起動";
    case ESP_RST_DEEPSLEEP: return "ディープスリープからの復帰";
    case ESP_RST_BROWNOUT:  return "電圧低下（ブラウンアウト）";
    default:                return "その他";
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
void updateLeds(uint32_t now) {
  digitalWrite(PIN_LED, pump.running() ? HIGH : LOW);

  // 基板上のLEDを一定間隔で点滅させる（ループが回っていることの確認）
  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMs = now;
    boardLedOn = !boardLedOn;
    digitalWrite(BOARD_LED_PIN, boardLedOn ? BOARD_LED_ON_LEVEL : BOARD_LED_OFF_LEVEL);
  }
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
  // 電源投入直後にポンプが回り続けないよう、何よりも先にポンプを止める
  pump.begin();

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
  Serial.println("esp32-pump 動作確認サンプル（ポンプ制御）");
  Serial.printf("起動の理由: %s\n", resetReasonText(reason));
  Serial.println("タクトスイッチ: ポンプの運転/停止の切り替え");
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
