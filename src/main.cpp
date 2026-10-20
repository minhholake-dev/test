#include <Arduino.h>
#define BUTTON_PIN 25
#define LED 27
#define ADC 35
#define N 32

int buffer[N] = {0};
int start = 0;
int samples = 0;

volatile bool buttonPressed = false;
volatile uint32_t drueke = 0, vorher = 0;

// Interrupt with debouncing
void IRAM_ATTR debounceShield() {
    uint32_t jetzt = millis();

    if (jetzt - vorher > 100) {
        drueke++;
        vorher = jetzt;
        buttonPressed = true;
    }
}

// Moving average filter
int filter(int m) {
    buffer[start] = m;
    start = (start + 1) % N;

    if (samples < N) samples++;

    long total = 0;
    for (int i = 0; i < N; i++) {
        total += buffer[i];
    }

    return total / samples;
}

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    ledcAttach(LED, 5000, 8);

    attachInterrupt(
        digitalPinToInterrupt(BUTTON_PIN),
        debounceShield,
        FALLING
    );
}

void loop() {
    static int blinkState = 0;
    static uint32_t blinkTime = 0;
    static uint32_t sampleTime = 0;
    static int processed = 0;

    uint32_t now = millis();

    // Detect button press
    if (buttonPressed) {
        buttonPressed = false;
        blinkState = 1;
        blinkTime = now;
    }

    // Nonblocking LED flash
    if (blinkState == 1 &&
        now - blinkTime >= 80) {
        blinkState = 2;
        blinkTime = now;
    }
    else if (blinkState == 2 &&
             now - blinkTime >= 80) {
        blinkState = 0;
    }

    // Sample ADC every 20 ms
    if (now - sampleTime >= 20) {
        sampleTime = now;

        int raw = analogRead(ADC);
        processed = filter(raw);

        Serial.print(">tho:");
        Serial.println(raw);

        Serial.print(">loc:");
        Serial.println(processed);
    }

    // Control LED brightness
    if (blinkState == 1) {
        ledcWrite(LED, 0);
    }
    else if (blinkState == 2) {
        ledcWrite(LED, 255);
    }
    else {
        ledcWrite(
            LED,
            map(processed, 0, 4095, 0, 255)
        );
    }
}
