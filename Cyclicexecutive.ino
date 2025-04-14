#include <Arduino.h>
#include <Ticker.h>
#include <B31DGMonitor.h>

#define SIGNAL1_PIN      26
#define SIGNAL2_PIN      32
#define LED_STATUS_PIN   4
#define BUTTON_PIN       23
#define LED_BUTTON_PIN   25
#define FREQ1_INPUT_PIN  18
#define FREQ2_INPUT_PIN  22

#define HYPERPERIOD_MS   60
#define TICK_TIME_US     1000  

B31DGCyclicExecutiveMonitor monitor;
Ticker cyclicTicker;

volatile unsigned long cyclicTick = 0;
const unsigned long debounce_Delay = 250;
unsigned long last_Debounce1 = 0;
static int task1Phase = 0;
static unsigned long task1LastChange = 0;

static int task2Step = 0;
static unsigned long task2LastChange = 0;

volatile unsigned long prevEdge1 = 0, prevEdge2 = 0;
volatile unsigned int F1 = 0, F2 = 0;
volatile bool lastButtonState = HIGH;
volatile unsigned long buttonLastTime = 0;
const unsigned long debounceDelayUs = 50000; 
void busyWaitMicroseconds(unsigned long duration) {
    unsigned long start = micros();
    while ((micros() - start) < duration) {
    }
}

void Task1() {
        monitor.jobStarted(1);
        digitalWrite(SIGNAL1_PIN, HIGH);
        busyWaitMicroseconds(250);
        digitalWrite(SIGNAL1_PIN, LOW);
        busyWaitMicroseconds(50);
        digitalWrite(SIGNAL1_PIN, HIGH);
        busyWaitMicroseconds(300);
        digitalWrite(SIGNAL1_PIN, LOW);
        monitor.jobEnded(1);
}

void Task2() {
        monitor.jobStarted(2);
        digitalWrite(SIGNAL2_PIN, HIGH);
        busyWaitMicroseconds(100);
        digitalWrite(SIGNAL2_PIN, LOW);
        busyWaitMicroseconds(50);
        digitalWrite(SIGNAL2_PIN, HIGH);
        busyWaitMicroseconds(200);
        digitalWrite(SIGNAL2_PIN, LOW);
        monitor.jobEnded(2);
}

void Task3() {
  monitor.jobStarted(3);
  unsigned long now = micros();
  static bool edgeDetected = false;
  if (digitalRead(FREQ1_INPUT_PIN) == HIGH) {
    if (edgeDetected) {
      unsigned long period = now - prevEdge1;
      F1 = (period > 0) ? (1000000.0 / period) : 0;
    }
    prevEdge1 = now;
    edgeDetected = true;
  }
  monitor.jobEnded(3);
}
void Task4() {
  monitor.jobStarted(4);
  unsigned long now = micros();
  static bool edgeDetected = false;
  if (digitalRead(FREQ2_INPUT_PIN) == HIGH) {
    if (edgeDetected) {
      unsigned long period = now - prevEdge2;
      F2 = (period > 0) ? (1000000.0 / period) : 0;
    }
    prevEdge2 = now;
    edgeDetected = true;
  }
  monitor.jobEnded(4);
}

void Task5() {
  monitor.jobStarted(5);
  monitor.doWork();
  monitor.jobEnded(5);
}

void Task6(){
  digitalWrite(LED_STATUS_PIN, ((F1 + F2) > 1500) ? HIGH : LOW);
}

void checkButton() {
  unsigned long now = micros();
  bool currentState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentState == LOW && (now - buttonLastTime >= debounceDelayUs)) {
    digitalWrite(LED_BUTTON_PIN, !digitalRead(LED_BUTTON_PIN));
    monitor.doWork();
    buttonLastTime = now;
  }
  lastButtonState = currentState;
}

void cyclicExecutive() {
  unsigned long tickSlot = cyclicTick % HYPERPERIOD_MS;
  if (tickSlot % 4 == 0) {
    Task1();
  }
  if (tickSlot % 3 == 0) {
    Task2();
  }
  if (tickSlot % 10 == 0) {
    Task3();
    Task4();
  }
  if (tickSlot % 5 == 0) {
    Task5();
  }
  Task6();
  checkButton();
  cyclicTick++;
}


void setup() {
  Serial.begin(115200);
  
  pinMode(SIGNAL1_PIN, OUTPUT);
  pinMode(SIGNAL2_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_BUTTON_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FREQ1_INPUT_PIN, INPUT);
  pinMode(FREQ2_INPUT_PIN, INPUT);
  
  digitalWrite(SIGNAL1_PIN, LOW);
  digitalWrite(SIGNAL2_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);
  digitalWrite(LED_BUTTON_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), checkButton , FALLING);
  
  monitor.startMonitoring();
  
  cyclicTicker.attach_us(TICK_TIME_US, cyclicExecutive);
}

void loop() {
  delay(10);
}
