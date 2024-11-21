## 0.3.0
### MINOR Changes:
- Adds a `-P` / `--persistence-mode` cmdline option to optionally enable "Persistence Mode" when the utility is invoked.

### PATCH Changes:
- Fixes build error in tests
- Fixes build error with use of `dprintf(...)` without format args

## 0.2.0
### MINOR Changes:
- Adds an alternative implementation of async/timer operations that models `std::execution`
- Removes the external dependency on `exios`

## 0.1.1
### PATCH Changes:
- Bumps exios depdendency to version `0.4.1`
- Fixes the installation path for the manpage. Now installs to the `$MANDIR/man1` subfolder.
- Increases code diagnostic warning level and adds hardening for release builds

## 0.1.0
### Initial release:
- Current support for NVIDIA GPUs only.
- Headless execution; Runs under both X11 and Wayland desktop environments.
- Fully customizable fan curve, with ability to fall-back to the GPU default fan profile.
- Validates the user-specified curve points to prevent misuse.
- Ability to print the fan curve to STDOUT for consumption by 3rd party graphing tools.
- Ability to periodically print the temperature and fan speed metrics to STDOUT.

