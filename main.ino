// Pin Definitions
const int redPins[4] = {5, 16, 17, 19};
const int yellowPins[4] = {18, 21, 22, 23};
const int greenPins[4] = {25, 26, 27, 32};
const int triggerPins[4] = {12, 33, 14, 13};
const int echoPins[4] = {39, 34, 35, 36};
const int touchPins[4] = {4, 0, 2, 15};

// Constants
const int NUM_SIGNALS = 4;
const unsigned long MIN_GREEN_TIME = 5000;  // 5 seconds
const unsigned long MAX_GREEN_TIME = 30000; // 30 seconds
const unsigned long YELLOW_TIME = 3000;     // 3 seconds
const unsigned long MAX_WAIT_TIME = 120000; // 2 minutes

// Thresholds (in cm)
const int LOW_TRAFFIC_THRESHOLD = 100;
const int HIGH_TRAFFIC_THRESHOLD = 20;

unsigned long signalTimers[NUM_SIGNALS] = {0, 0, 0, 0};
unsigned long lastChangeTime = 0;
int currentSignal = 0;
enum SignalState { RED, YELLOW, GREEN };
SignalState signalStates[NUM_SIGNALS] = {RED, RED, RED, RED};

int measureTraffic(int signalIndex);
int determineNextSignal(int traffic[], unsigned long currentTime);
void changeSignal(int newSignal);
void updateSignals();
void checkTouchInputs();

void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i < NUM_SIGNALS; i++) {
    pinMode(redPins[i], OUTPUT);
    pinMode(yellowPins[i], OUTPUT);
    pinMode(greenPins[i], OUTPUT);
    pinMode(triggerPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }

  updateSignals();
  Serial.println("Traffic Light Controller Initialized");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check traffic density
  int traffic[NUM_SIGNALS];
  for (int i = 0; i < NUM_SIGNALS; i++) {
    traffic[i] = measureTraffic(i);
    Serial.print("Traffic at signal ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(traffic[i]);
    Serial.println(" cm");
  }
  
  int nextSignal = determineNextSignal(traffic, currentTime);
  
  if (nextSignal != currentSignal && (currentTime - lastChangeTime) >= MIN_GREEN_TIME) {
    changeSignal(nextSignal);
    lastChangeTime = currentTime;
  }

  updateSignals();
  checkTouchInputs();
  
  delay(1000);
}

int measureTraffic(int signalIndex) {
  digitalWrite(triggerPins[signalIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPins[signalIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPins[signalIndex], LOW);
  
  long duration = pulseIn(echoPins[signalIndex], HIGH, 30000); // 30ms timeout
  if (duration == 0) {
    return LOW_TRAFFIC_THRESHOLD;
  }
  int distance = duration * 0.034 / 2; 
  
  return distance;
}

int determineNextSignal(int traffic[], unsigned long currentTime) {
  int highTrafficCount = 0;
  int highestTrafficSignal = -1;
  
  for (int i = 0; i < NUM_SIGNALS; i++) {
    if (traffic[i] < HIGH_TRAFFIC_THRESHOLD) {
      highTrafficCount++;
      if (highestTrafficSignal == -1 || traffic[i] < traffic[highestTrafficSignal]) {
        highestTrafficSignal = i;
      }
    }
  }
  
  for (int i = 0; i < NUM_SIGNALS; i++) {
    if (currentTime - signalTimers[i] > MAX_WAIT_TIME && i != currentSignal) {
      Serial.print("Starvation prevented. Switching to signal ");
      Serial.println(i + 1);
      return i;
    }
  }
  
  if (highTrafficCount > 0) {
    Serial.print("High traffic detected. Switching to signal ");
    Serial.println(highestTrafficSignal + 1);
    return highestTrafficSignal;
  }
  
  if (currentTime - lastChangeTime >= MAX_GREEN_TIME) {
    int nextSignal = (currentSignal + 1) % NUM_SIGNALS;
    Serial.print("Max green time reached. Switching to signal ");
    Serial.println(nextSignal + 1);
    return nextSignal;
  }
  
  return currentSignal;
}

void changeSignal(int newSignal) {
  signalStates[currentSignal] = YELLOW;
  updateSignals();
  delay(YELLOW_TIME);
  
  // Change current signal to red and new signal to green
  signalStates[currentSignal] = RED;
  currentSignal = newSignal;
  signalStates[currentSignal] = GREEN;
  signalTimers[currentSignal] = millis();
  
  Serial.print("Signal changed to ");
  Serial.println(currentSignal + 1);
}

void updateSignals() {
  for (int i = 0; i < NUM_SIGNALS; i++) {
    digitalWrite(redPins[i], signalStates[i] == RED ? HIGH : LOW);
    digitalWrite(yellowPins[i], signalStates[i] == YELLOW ? HIGH : LOW);
    digitalWrite(greenPins[i], signalStates[i] == GREEN ? HIGH : LOW);
  }
}

void checkTouchInputs() {
  for (int i = 0; i < NUM_SIGNALS; i++) {
    if (touchRead(touchPins[i]) < 40) {
      Serial.print("Touch detected on signal ");
      Serial.println(i + 1);e
    }
  }
}