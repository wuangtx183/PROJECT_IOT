#include "TCS3200.h"

// ================= CONSTRUCTOR =================
TCS3200::TCS3200(uint8_t S0, uint8_t S1, uint8_t S2, uint8_t S3, uint8_t OUT) {
    _S0 = S0; _S1 = S1; _S2 = S2; _S3 = S3; _OUT = OUT;
}

// ================= INIT =================
void TCS3200::begin() {
    pinMode(_S0, OUTPUT);
    pinMode(_S1, OUTPUT);
    pinMode(_S2, OUTPUT);
    pinMode(_S3, OUTPUT);
    pinMode(_OUT, INPUT);

    // ✅ Tần số 2% (ổn định nhất cho ESP32)
    digitalWrite(_S0, LOW);
    digitalWrite(_S1, HIGH);
}

// ================= READ ONE COLOR =================
int TCS3200::getPulse(uint8_t s2_state, uint8_t s3_state) {
    digitalWrite(_S2, s2_state);
    digitalWrite(_S3, s3_state);
    delayMicroseconds(200);

    unsigned long t = pulseIn(_OUT, HIGH, 100000); // 100ms

    if (t == 0) return -1;
    return (int)t;
}


// ================= READ RGB =================
void TCS3200::read() {
    R = getPulse(LOW, LOW);      // RED
    G = getPulse(HIGH, HIGH);   // GREEN
    B = getPulse(LOW, HIGH);    // BLUE
}

// ================= PRINT =================
void TCS3200::printRGB() {
    Serial.print("[R: "); Serial.print(R);
    Serial.print(" | G: "); Serial.print(G);
    Serial.print(" | B: "); Serial.print(B);
    Serial.println("]");
}

// ================= DETECT COLOR =================
String TCS3200::detect() {

    if (R <= 0 || G <= 0 || B <= 0) return "NONE";

    // 🔴 RED: R nhỏ nhất rõ ràng
    if (R < G * 0.7 && R < B * 0.7) {
        return "RED";
    }

    // 🔵 BLUE: B nhỏ nhất rõ ràng
    if (B < R * 0.7 && B < G * 0.7) {
        return "BLUE";
    }

    // 🟡 YELLOW: R ≈ G và B yếu
    if (abs(R - G) < (R * 0.25) && B > R * 1.3) {
        return "YELLOW";
    }

    return "NONE";
}
