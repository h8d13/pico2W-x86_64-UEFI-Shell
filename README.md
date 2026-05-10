pico2W-x86_64-UEFI-Shell
========================

Resources

  pico-sdk 2.1.1   https://github.com/raspberrypi/pico-sdk
                   submodules: tinyusb, btstack, cyw43-driver, lwip, mbedtls

  picotool 2.2.0   https://github.com/raspberrypi/picotool
                   built with `-include picotool/_prefix.h` to force
                   <pthread.h> in before pico-sdk's __unused macro

  shellx64.efi     https://github.com/pbatard/UEFI-Shell/releases/latest
                   auto-downloaded by CMakeLists.txt at configure time
