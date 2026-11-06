#pragma once
#include <Arduino.h>

// TB6612FNG のA側で1台のポンプを回す。
// start() / stop() で運転・停止を指示し、update() を周期的に呼ぶと
// 出力を PUMP_RAMP_MS かけて目標まで上げる（ソフトスタート）。
class PumpDriver {
public:
  void begin();            // ピンを出力にして、ポンプを止めた状態にする（setup()の最初に呼ぶ）
  void start(int percent); // 指定した出力（0〜100%）で運転する
  void stop();             // すぐに止め、STBYをLOWにしてポンプを切り離す
  void update();           // 制御ループから CONTROL_INTERVAL_MS ごとに呼ぶ

  bool running() const { return target_ > 0; }
  int output() const { return (int)output_; } // 現在の出力（%）

private:
  void apply(float percent);

  float target_ = 0; // 目標出力（%）
  float output_ = 0; // 現在の出力（%）
};
