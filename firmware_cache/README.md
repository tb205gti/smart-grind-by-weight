# Firmware Cache

This directory stores compiled firmware binaries during development.

**Note**: Firmware binaries are automatically excluded from git via `.gitignore` to prevent repository bloat.

## Usage

The `./tools/grinder` script automatically manages this directory:
- Built firmware is cached here for BLE OTA uploads
- Old builds are automatically cleaned up by build scripts
- Maximum cache size is limited to prevent disk space issues

## Manual Cleanup

To manually clear the firmware cache:
```bash
rm -f firmware_cache/*.bin
```