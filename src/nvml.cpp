#include "nvml.hpp"
#include "dlfcn.h"
#include "nvml.h"
#include "symbol.hpp"
#include <stdexcept>

#define CHECK_NVML_RESULT(op, msg)                                             \
    do {                                                                       \
        if (auto const result = (op); result != NVML_SUCCESS) {                \
            throw std::runtime_error { std::string { msg } + ": " +            \
                                       ::gfc::nvml::lib().nvmlErrorString(     \
                                           result) };                          \
        }                                                                      \
    }                                                                          \
    while (0)

namespace gfc::nvml
{
auto load_nvml() -> NVML
{
    auto lib = dlopen("libnvidia-ml.so.1", RTLD_LAZY);
    if (!lib) {
        throw std::runtime_error {
            std::string { "Couldn't load libnvidia-ml.so.1: " } + dlerror()
        };
    }
    NVML nvml {};
    TRY_ATTACH_SYMBOL(&nvml.nvmlInit_v2, "nvmlInit_v2", lib);
    TRY_ATTACH_SYMBOL(&nvml.nvmlShutdown, "nvmlShutdown", lib);
    TRY_ATTACH_SYMBOL(&nvml.nvmlErrorString, "nvmlErrorString", lib);
    TRY_ATTACH_SYMBOL(
        &nvml.nvmlDeviceGetCount_v2, "nvmlDeviceGetCount_v2", lib);
    TRY_ATTACH_SYMBOL(&nvml.nvmlDeviceGetHandleByIndex_v2,
                      "nvmlDeviceGetHandleByIndex_v2",
                      lib);
    TRY_ATTACH_SYMBOL(
        &nvml.nvmlDeviceGetTemperature, "nvmlDeviceGetTemperature", lib);
    TRY_ATTACH_SYMBOL(
        &nvml.nvmlDeviceSetFanSpeed_v2, "nvmlDeviceSetFanSpeed_v2", lib);
    TRY_ATTACH_SYMBOL(&nvml.nvmlDeviceSetDefaultFanSpeed_v2,
                      "nvmlDeviceSetDefaultFanSpeed_v2",
                      lib);
    TRY_ATTACH_SYMBOL(&nvml.nvmlDeviceGetNumFans, "nvmlDeviceGetNumFans", lib);

    return nvml;
}

auto lib() -> NVML const&
{
    static const NVML lib = load_nvml();

    return lib;
}

auto init() -> void { CHECK_NVML_RESULT(lib().nvmlInit_v2(), "init"); }

auto shutdown() noexcept -> void
{
    try {
        lib().nvmlShutdown();
    }
    catch (...) {
    }
}

auto get_device_count() -> std::size_t
{
    unsigned int count;
    CHECK_NVML_RESULT(lib().nvmlDeviceGetCount_v2(&count), "get_device_count");
    return count;
}

auto get_device_handle_by_index(unsigned int index) -> nvmlDevice_t
{
    nvmlDevice_t device;
    CHECK_NVML_RESULT(lib().nvmlDeviceGetHandleByIndex_v2(index, &device),
                      "get_device_handle_by_index");
    return device;
}
auto get_device_temperature(nvmlDevice_t device,
                            nvmlTemperatureSensors_t sensor_type) -> std::size_t
{
    unsigned int temperature;
    CHECK_NVML_RESULT(
        lib().nvmlDeviceGetTemperature(device, sensor_type, &temperature),
        "get_device_temperature");
    return temperature;
}

auto set_device_fan_speed(nvmlDevice_t device,
                          unsigned int fan_index,
                          unsigned int pc) -> void
{
    CHECK_NVML_RESULT(lib().nvmlDeviceSetFanSpeed_v2(device, fan_index, pc),
                      "set_device_fan_speed");
}

auto set_device_default_fan_speed(nvmlDevice_t device, unsigned int fan_index)
    -> void
{
    CHECK_NVML_RESULT(lib().nvmlDeviceSetDefaultFanSpeed_v2(device, fan_index),
                      "set_device_default_fan_speed");
}

auto get_device_fan_count(nvmlDevice_t device) -> unsigned int
{
    unsigned int count;
    CHECK_NVML_RESULT(lib().nvmlDeviceGetNumFans(device, &count),
                      "get_device_fan_count");
    return count;
}

} // namespace gfc::nvml
