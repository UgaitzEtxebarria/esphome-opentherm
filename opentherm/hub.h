#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"

#include "OpenTherm.h"

#include "sensor.h"
#include "binary_sensor.h"
#include "switch.h"
#include "output.h"

#include <unordered_map>
#include <unordered_set>

namespace esphome {
namespace opentherm {

// OpenTherm component for ESPHome
class OpenthermHub : public Component {
protected:
    // Communication pins for the OpenTherm interface
    int in_pin, out_pin;
    // The OpenTherm interface from @ihormelnyk's library
    OpenTherm* ot;

    #define OPENTHERM_DECLARE_SENSOR(s) sensor::Sensor* s ## _sensor;
    OPENTHERM_SENSOR_LIST(OPENTHERM_DECLARE_SENSOR, )

    #define OPENTHERM_DECLARE_BINARY_SENSOR(s) binary_sensor::BinarySensor* s ## _binary_sensor;
    OPENTHERM_BINARY_SENSOR_LIST(OPENTHERM_DECLARE_BINARY_SENSOR, )

    // The set of initial requests to make on starting communication with the boiler
    std::unordered_set<byte> initial_requests;
    // and the repeating requests which are made repeatedly to update various sensors
    std::unordered_set<byte> repeating_requests;
    // Indicates if we are still working on the initial requests or not
    bool initializing = true;
    // Index for the current request in one of the _requests sets.
    std::unordered_set<byte>::const_iterator current_request_iterator;

    // There are three ways to set the boiler temperature setpoint:
    // 1. By setting a sensor which provides the current temperature setpoint
    sensor::Sensor* t_set_input_sensor;
    // 2. By setting a FloatOutput with a value 0.0-1.0 to set the relative setpoint,
    // for example from a PID climate controller
    // This also requires providing the min_t_set and max_t_set values, or setting the
    // option to request the latter from the boiler.
    SetpointOutput* t_set_input_relative;
    // 3. By setting a number which provides the current temperature setpoint
    number::Number* t_set_input_number;

    // This method returns the current setpoint, or 0.0 is no setpoint is set
    // If more than one input is configured, the first one takes precedence
    float get_t_set_input(byte request_id);

    // There are five status variables, which can either be set as a simple variable,
    // or as a switch. ch_enable and dhw_enable default to true, the others to false.
    bool ch_enable = true, dhw_enable = false, cooling_enable, otc_active, ch2_active;
    std::vector<OpenthermSwitch*> switches;

    // Helper methods to publish a state to a sensor or binary sensor
    // of a certain type, if it has been registered with the hub
    void publish_to_sensor(OpenthermSensorType type, float state);
    void publish_to_binary_sensor(OpenthermBinarySensorType type, bool state);

    // Create OpenTherm messages based on the message id
    unsigned int build_request(byte request_id);

    // Callbacks to pass to OpenTherm interface for globally defined interrupts
    void(*handle_interrupt_callback)();
	void(*process_response_callback)(unsigned long, OpenThermResponseStatus);

public:
    // Constructor with references to the global interrupt handlers
    OpenthermHub(void(*handle_interrupt_callback)(void), void(*process_response_callback)(unsigned long, OpenThermResponseStatus));

    // Interrupt handler, which notifies the OpenTherm interface of an interrupt
    void IRAM_ATTR handle_interrupt();

    // Handle responses from the OpenTherm interface
    void process_response(unsigned long response, OpenThermResponseStatus status);

    // Setters for the input and output OpenTherm interface pins
    void set_in_pin(int in_pin) { this->in_pin = in_pin; }
    void set_out_pin(int out_pin) { this->out_pin = out_pin; }

    #define OPENTHERM_SET_SENSOR(s) void set_ ## s ## _sensor(sensor::Sensor* sensor) { this->s ## _sensor = sensor; }
    OPENTHERM_SENSOR_LIST(OPENTHERM_SET_SENSOR, )

    #define OPENTHERM_SET_BINARY_SENSOR(s) void set_ ## s ## _binary_sensor(binary_sensor::BinarySensor* binary_sensor) { this->s ## _binary_sensor = binary_sensor; }
    OPENTHERM_BINARY_SENSOR_LIST(OPENTHERM_SET_BINARY_SENSOR, )

    // Add a request to the set of initial requests
    void add_initial_request(byte request) { this->initial_requests.insert(request); }
    // Add a request to the set of repeating requests. Note that a large number of repeating
    // requests will slow down communication with the boiler. Each request may take up to 1 second,
    // so with all sensors enabled, it may take about half a minute before a change in setpoint
    // will be processed.
    void add_repeating_request(byte request) { this->repeating_requests.insert(request); }

    // Setters for the inputs that control the boiler setpoint temperature
    void set_t_set_input_sensor(sensor::Sensor* t_set_input_sensor) { this->t_set_input_sensor = t_set_input_sensor; }
    void set_t_set_input_relative(SetpointOutput* t_set_input_relative) { this->t_set_input_relative = t_set_input_relative; }
    void set_t_set_input_number(number::Number* t_set_input_number) { this->t_set_input_number = t_set_input_number; }

    // Setters for the status variables
    void set_ch_enable(bool ch_enable) { this->ch_enable = ch_enable; }
    void set_dhw_enable(bool dhw_enable) { this->dhw_enable = dhw_enable; }
    void set_cooling_enable(bool cooling_enable) { this->cooling_enable = cooling_enable; }
    void set_otc_active(bool otc_active) { this->otc_active = otc_active; }
    void set_ch2_active(bool ch2_active) { this->ch2_active = ch2_active; }

    // Method to register switches
    void register_switch(OpenthermSwitch* sw);
    
    float get_setup_priority() const override{
        return setup_priority::HARDWARE;
    }

    void setup() override;
    void on_shutdown() override;
    void loop() override;
    void dump_config() override;
};

} // namespace opentherm
} // namespace esphome
