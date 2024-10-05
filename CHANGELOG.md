## 0.1.0
### Initial release:
- Current support for NVIDIA GPUs only.
- Headless execution; Runs under both X11 and Wayland desktop environments.
- Fully customizable fan curve, with ability to fall-back to the GPU default fan profile.
- Validates the user-specified curve points to prevent misuse.
- Ability to print the fan curve to STDOUT for consumption by 3rd party graphing tools.
- Ability to periodically print the temperature and fan speed metrics to STDOUT.
