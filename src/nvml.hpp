#ifndef GPUFANCTL_NVML_HPP_INCLUDED
#define GPUFANCTL_NVML_HPP_INCLUDED

#include "nvml.h"
#include <cstddef>

namespace gfc::nvml
{

struct NVML
{
    PFN_nvmlInit_v2 nvmlInit_v2;
    PFN_nvmlShutdown nvmlShutdown;
    PFN_nvmlErrorString nvmlErrorString;
    PFN_nvmlDeviceGetCount_v2 nvmlDeviceGetCount_v2;
    PFN_nvmlDeviceGetHandleByIndex_v2 nvmlDeviceGetHandleByIndex_v2;
    PFN_nvmlDeviceGetTemperature nvmlDeviceGetTemperature;
    PFN_nvmlDeviceSetFanSpeed_v2 nvmlDeviceSetFanSpeed_v2;
    PFN_nvmlDeviceSetDefaultFanSpeed_v2 nvmlDeviceSetDefaultFanSpeed_v2;
    PFN_nvmlDeviceGetNumFans nvmlDeviceGetNumFans;
    PFN_nvmlDeviceSetPersistenceMode nvmlDeviceSetPersistenceMode;
};

auto lib() -> NVML const&;

auto init() -> void;
auto shutdown() noexcept -> void;
auto get_device_count() -> std::size_t;
auto get_device_handle_by_index(unsigned int index) -> nvmlDevice_t;
auto get_device_temperature(nvmlDevice_t device,
                            nvmlTemperatureSensors_t sensor_type)
    -> std::size_t;
auto set_device_fan_speed(nvmlDevice_t device,
                          unsigned int fan_index,
                          unsigned int pc) -> void;
auto set_device_default_fan_speed(nvmlDevice_t device, unsigned int fan_index)
    -> void;
auto get_device_fan_count(nvmlDevice_t device) -> unsigned int;
auto set_device_persistence_mode(nvmlDevice_t device, nvmlEnableState_t state)
    -> void;
} // namespace gfc::nvml

#endif // GPUFANCTL_NVML_HPP_INCLUDED
