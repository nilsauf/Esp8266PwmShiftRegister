#include <Arduino.h>
#include <Esp8266PwmShiftRegister.hpp>

#define SHIFT_COUNT 1

Esp8266PwmShiftRegister shiftRegister(13, 14, 15, SHIFT_COUNT, 255);

// Timer callback to trigger the update of the pwm shift register
void IRAM_ATTR timerUpdate(void)
{
    noInterrupts();
    Esp8266PwmShiftRegister::singleton->update();
    interrupts();
}

void setup()
{
    // put your setup code here, to run once:

    // Attaching the timer interrupt callback
    Esp8266PwmShiftRegister::singleton->ITimer.attachInterruptInterval(Esp8266PwmShiftRegister::singleton->updateCycleCountInterval, timerUpdate);
}

void loop()
{
    // put your main code here, to run repeatedly:

    // Setting the first pin to half brightness
    shiftRegister.set(0, 125);

    // Setting the second pin to full brightness
    shiftRegister.set(1, 255);
}