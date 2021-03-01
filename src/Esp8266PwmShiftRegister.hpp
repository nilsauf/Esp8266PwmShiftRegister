#ifndef ESP_PWM_SHIFT_REGISTER
#define ESP_PWM_SHIFT_REGISTER

#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

#include <Arduino.h>
#include <ESP8266TimerInterrupt.h>
#include <FastEsp8266ShiftRegister.hpp>

class Esp8266PwmShiftRegister
{
public:
    Esp8266PwmShiftRegister(const uint8_t dataPin, const uint8_t clockPin, const uint8_t latchPin, const uint8_t shiftRegisterCount = 1, const uint8_t resolution = 255)
    {
        this->_shiftRegister = new FastEsp8266ShiftRegister(dataPin, clockPin, latchPin, shiftRegisterCount);
        this->_resolution = resolution;
        this->_shiftRegisterCount = shiftRegisterCount;

        // init data
        // internally a two-dimensional array: first dimension time, second dimension shift register bytes
        // data[t + sr * resolution]
        this->data = (uint8_t *)malloc(resolution * shiftRegisterCount * sizeof(uint8_t));
        for (int t = 0; t < resolution; ++t)
        {
            for (int i = 0; i < shiftRegisterCount; ++i)
            {
                this->data[t + i * resolution] = 0;
            }
        }

        Esp8266PwmShiftRegister::singleton = this; // make this object accessible for timer interrupts

        // the boolean will be used to increase the performance in other functions
        this->singleShiftRegister = (shiftRegisterCount == 1);

        this->ITimer = ESP8266Timer();
    }

    /**
    * Set a pin of the shift register to a given PWM value.
    * @param pin The index of the pin (starting at 0). If multiple shift registers are chained, the first pin of the second shift register would be addressed with pin = 8.
    * @param value The PWM value (between 0 and 255 as it will be scaled to the resolution that was passed to the constructor). 
    */
    void set(uint8_t pin, uint8_t value)
    {
        value = (uint8_t)(value / 255.0 * this->_resolution + .5); // round
        uint8_t shiftRegister = pin / 8;
        for (int t = 0; t < this->_resolution; ++t)
        {
            // set (pin % 8)th bit to (value > t)
            this->data[t + shiftRegister * this->_resolution] ^= (-(value > t) ^ this->data[t + shiftRegister * this->_resolution]) & (1 << (pin % 8));
        }
    };

    void update()
    {
        // higher performance for single shift register mode
        if (singleShiftRegister)
        {
            this->_shiftRegister->shiftOut(this->data[time]);
        }
        else
        {
            for (int shiftRegisterIndex = this->_shiftRegisterCount - 1; shiftRegisterIndex >= 0; shiftRegisterIndex--)
            {
                this->_shiftRegister->shiftOut(this->data[time + shiftRegisterIndex * this->_resolution]);
            }
        }
        this->_shiftRegister->update();

        if (++time == this->_resolution)
        {
            time = 0;
        }
    }

    static Esp8266PwmShiftRegister *singleton; // used inside the ISR
    ESP8266Timer ITimer;
    const long updateCycleCountInterval = 20;

private:
    FastEsp8266ShiftRegister *_shiftRegister;
    uint8_t _resolution;
    uint8_t _shiftRegisterCount;
    uint8_t *data;             // data matrix [t + sr * resolution]
    volatile uint8_t time = 0; // time resolution of resolution steps

    bool singleShiftRegister; // true if (shiftRegisterCount == 1)
};

// One static reference to the Esp8266PwmShiftRegister that was lastly created. Used for access through timer interrupts.
Esp8266PwmShiftRegister *Esp8266PwmShiftRegister::singleton = NULL;

#endif
