#include "PumpDriver.h"
#include "config.h"

static const uint32_t MAX_DUTY = (1u << PUMP_PWM_BITS) - 1;
static const uint8_t LEDC_CHANNEL = 0;

// Arduino-ESP32 のコア3.xでLEDC(PWM)の関数が変わったため、両方で動くようにする
static void pwmAttach(uint8_t pin) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(pin, PUMP_PWM_FREQ_HZ, PUMP_PWM_BITS, LEDC_CHANNEL);
#else
  ledcSetup(LEDC_CHANNEL, PUMP_PWM_FREQ_HZ, PUMP_PWM_BITS);
  ledcAttachPin(pin, LEDC_CHANNEL);
#endif
}

static void pwmWrite(uint8_t pin, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(LEDC_CHANNEL, duty);
#endif
}

void PumpDriver::begin() {
  // 起動直後のピンは状態が決まっていないため、真っ先にSTBYと方向ピンをLOWにする
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, LOW);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  pwmAttach(PIN_PWMA);
  stop();
}

void PumpDriver::start(int percent) {
  target_ = constrain(percent, 0, 100);
  if (target_ == 0) {
    stop();
    return;
  }
  // 回す向きを決める（TB6612FNG: IN1=H IN2=L で正転、IN1=L IN2=H で逆転）
  digitalWrite(PIN_AIN1, PUMP_INVERT ? LOW : HIGH);
  digitalWrite(PIN_AIN2, PUMP_INVERT ? HIGH : LOW);
  digitalWrite(PIN_STBY, HIGH);
}

void PumpDriver::stop() {
  target_ = 0;
  apply(0);
  // IN1=L IN2=L で惰性停止。ポンプは急ブレーキの必要がないので、こちらにしている
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_STBY, LOW);
}

void PumpDriver::update() {
  if (output_ == target_) return;
  // 0→100% を PUMP_RAMP_MS で上げる。出力を下げるときも同じ速さにする
  const float step = 100.0f * CONTROL_INTERVAL_MS / PUMP_RAMP_MS;
  float next = output_ < target_ ? min(output_ + step, target_)
                                 : max(output_ - step, target_);
  apply(next);
}

void PumpDriver::apply(float percent) {
  output_ = percent;
  pwmWrite(PIN_PWMA, (uint32_t)(percent * MAX_DUTY / 100.0f + 0.5f));
}
