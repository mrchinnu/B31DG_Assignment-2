#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <B31DGMonitor.h>

#define SIGNAL1_PIN     26
#define SIGNAL2_PIN     32
#define F1_INPUT_PIN    18
#define F2_INPUT_PIN    22
#define LED_SUM_PIN     4
#define LED_BUTTON_PIN  25
#define BUTTON_PIN      23

B31DGCyclicExecutiveMonitor monitor;
float F1 = 0, F2 = 0;
#define FREQ_TIMEOUT_US 3000
const unsigned long debounce_Delay = 250;
unsigned long last_Debounce1 = 0;

void busyWaitMicroseconds(unsigned long duration) {
    unsigned long start = micros();
    while ((micros() - start) < duration) {
    }
}
void taskSignal1(void *pvParam) {
    esp_task_wdt_add(NULL);
    const TickType_t period = pdMS_TO_TICKS(4);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        monitor.jobStarted(1);
        digitalWrite(SIGNAL1_PIN, HIGH);
        busyWaitMicroseconds(250);
        digitalWrite(SIGNAL1_PIN, LOW);
        busyWaitMicroseconds(50);
        digitalWrite(SIGNAL1_PIN, HIGH);
        busyWaitMicroseconds(300);
        digitalWrite(SIGNAL1_PIN, LOW);
        monitor.jobEnded(1);
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
void taskSignal2(void *pvParam) {
    esp_task_wdt_add(NULL);
    const TickType_t period = pdMS_TO_TICKS(3);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        monitor.jobStarted(2);
        digitalWrite(SIGNAL2_PIN, HIGH);
        busyWaitMicroseconds(100);
        digitalWrite(SIGNAL2_PIN, LOW);
        busyWaitMicroseconds(50);
        digitalWrite(SIGNAL2_PIN, HIGH);
        busyWaitMicroseconds(200);
        digitalWrite(SIGNAL2_PIN, LOW);
        monitor.jobEnded(2);
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
float measureFrequency(int pin) {
    unsigned long startTime, endTime;
    unsigned long t0 = micros();
    while (digitalRead(pin) == LOW) {
        if (micros() - t0 > FREQ_TIMEOUT_US) return 0;
    }
    startTime = micros();
    while (digitalRead(pin) == HIGH) {
        if (micros() - startTime > FREQ_TIMEOUT_US) break;
    }
    while (digitalRead(pin) == LOW) {
        if (micros() - startTime > FREQ_TIMEOUT_US) break;
    }
    endTime = micros();
    unsigned long period = endTime - startTime;
    return (period == 0) ? 0 : 1000000.0 / period;
}
void taskMeasureF1(void *pvParam) {
    esp_task_wdt_add(NULL);
    const TickType_t period = pdMS_TO_TICKS(10);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        monitor.jobStarted(3);
        F1 = measureFrequency(F1_INPUT_PIN);
        monitor.jobEnded(3);
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
void taskMeasureF2(void *pvParam) {
    esp_task_wdt_add(NULL);
    const TickType_t period = pdMS_TO_TICKS(10);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        monitor.jobStarted(4);
        F2 = measureFrequency(F2_INPUT_PIN);
        monitor.jobEnded(4);
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
void taskDoWork(void *pvParam) {
    esp_task_wdt_add(NULL);
    const TickType_t period = pdMS_TO_TICKS(5);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        monitor.jobStarted(5);
        monitor.doWork();
        monitor.jobEnded(5);
        esp_task_wdt_reset();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
void taskSumLED(void *pvParam) {
    for (;;) {
        digitalWrite(LED_SUM_PIN, (F1 + F2) > 1500 ? HIGH : LOW);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void IRAM_ATTR checkButton() {
  if (millis() - last_Debounce1 > debounce_Delay) {
    digitalWrite(LED_BUTTON_PIN, !digitalRead(LED_BUTTON_PIN));
    monitor.doWork();
    last_Debounce1 = millis();
  }
}

void setup() {
    pinMode(SIGNAL1_PIN, OUTPUT);
    pinMode(SIGNAL2_PIN, OUTPUT);
    pinMode(F1_INPUT_PIN, INPUT);
    pinMode(F2_INPUT_PIN, INPUT);
    pinMode(LED_SUM_PIN, OUTPUT);
    pinMode(LED_BUTTON_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial.begin(115200);
    esp_task_wdt_deinit();
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), checkButton , FALLING);

    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = 3000,
        .idle_core_mask = (1 << 0),
        .trigger_panic = true
    };

    esp_task_wdt_init(&wdtConfig);

    monitor.startMonitoring();

    xTaskCreatePinnedToCore(taskSignal2, "Signal2", 2048, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(taskSignal1, "Signal1", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(taskDoWork, "DoWork", 2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(taskMeasureF1, "MeasureF1", 2048, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(taskMeasureF2, "MeasureF2", 2048, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(taskSumLED, "SumLED", 2048, NULL, 2, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(100));
}
