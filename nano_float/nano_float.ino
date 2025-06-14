#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

#define speed_pin 6
#define dir_pin 7
#define clk 1
#define anti_clk 0
#define EncoderA 2
#define EncoderB 3

int c_Up = 0;
float p = 0;
float pressure = 0;
volatile long encoderCount = 0;
SoftwareSerial mySerial(8, 9);
volatile float frotations = 0;
float last_rot = 0;
volatile float rotations = 0;
bool depth_regulation = false;
bool first_descend = true;
bool rising = false;

String input = "";

void setup() {
    Serial.begin(9600);
    mySerial.begin(9600);
    Wire.begin();

    pinMode(speed_pin, OUTPUT);
    pinMode(dir_pin, OUTPUT);
    digitalWrite(speed_pin, LOW);
    digitalWrite(dir_pin, LOW);

    pinMode(EncoderA, INPUT_PULLUP);
    pinMode(EncoderB, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EncoderA), encoderISR, RISING);

    vStop_motor();
}

void loop() {
    if (mySerial.available()) {
        input = mySerial.readStringUntil('\n');
        input.trim();
    }
    Serial.println(input);

    if (input == "Start") {
        depth_regulation = false;
        first_descend = true;
        rising = false;
        encoderCount = 0;
        rotations = 0;
        last_rot = 0;
        Serial.println("Starting motor for descend");
        while (rotations < (last_rot + 4.0)) {
            vMove_motor(anti_clk, 200);
            Serial.println(rotations);
        }
        vStop_motor();
        last_rot = rotations;
    }

    if (input == "1down" && rotations < 11) {
        while (rotations < (last_rot + 1.0)) {
            vMove_motor(anti_clk, 200);
            Serial.println(rotations);
        }
        vStop_motor();
        last_rot = rotations;
    }

    if (input == "lab" && rotations > 3) {
        while (rotations > (last_rot - 1.0)) {
            vMove_motor(clk, 150);
            Serial.println(rotations);
        }
        vStop_motor();
        last_rot = rotations;
    }

    if (input == "Up" && !rising) {
        depth_regulation = false;
        c_Up++;
        Serial.println("Starting motor for ascend");
        while (rotations > 0) {
            vMove_motor(clk, 200);
            Serial.println(rotations);
        }
        rising = true;
        vStop_motor();
        last_rot = rotations;
        resetArduino();
    }

    if (c_Up > 5) {
        end_prog();
    }

    last_rot = rotations;
    input = "";
}

void vMove_motor(int dir, int m_speed) {
    digitalWrite(dir_pin, (dir == clk) ? LOW : HIGH);
    analogWrite(speed_pin, m_speed);
}

void vStop_motor() {
    digitalWrite(speed_pin, LOW);
}

void encoderISR() {
    int stateA = digitalRead(EncoderA);
    encoderCount += (digitalRead(EncoderB) != stateA) ? 1 : -1;
    frotations = encoderCount / 600.0;
    rotations = frotations;
}

void end_prog() {
    vStop_motor();
    while (1);
}

void resetArduino() {
    void (*resetFunc)(void) = 0;
    resetFunc();
}
