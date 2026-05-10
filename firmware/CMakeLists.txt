cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(PICO_PLATFORM rp2350 CACHE STRING "Platform")

include(pico_sdk_import.cmake)

project(picobootcd C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(EFI_IMG     ${CMAKE_CURRENT_BINARY_DIR}/efi.img)
set(BOOT_ISO    ${CMAKE_CURRENT_BINARY_DIR}/boot.iso)
set(ISO_HEADER  ${CMAKE_CURRENT_BINARY_DIR}/iso_data.h)
set(ISO_ROOT    ${CMAKE_CURRENT_BINARY_DIR}/iso_root)

# `latest` is convenient but not reproducible; pin a tag for reproducible builds.
set(UEFI_SHELL_URL "https://github.com/pbatard/UEFI-Shell/releases/latest/download/shellx64.efi"
    CACHE STRING "URL to fetch BOOTX64.EFI from")
set(EFI_APP ${CMAKE_CURRENT_BINARY_DIR}/shellx64.efi)

if(NOT EXISTS ${EFI_APP})
    message(STATUS "Downloading UEFI Shell from ${UEFI_SHELL_URL}")
    file(DOWNLOAD ${UEFI_SHELL_URL} ${EFI_APP} STATUS _dl_status SHOW_PROGRESS)
    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        file(REMOVE ${EFI_APP})
        message(FATAL_ERROR "Failed to download UEFI Shell: ${_dl_status}")
    endif()
endif()

add_custom_command(
    OUTPUT ${EFI_IMG}
    COMMAND dd if=/dev/zero of=${EFI_IMG} bs=1024 count=1200 status=none
    COMMAND mkfs.vfat -F 12 -n EFIBOOT ${EFI_IMG}
    COMMAND env MTOOLS_SKIP_CHECK=1 mmd -i ${EFI_IMG} ::/EFI
    COMMAND env MTOOLS_SKIP_CHECK=1 mmd -i ${EFI_IMG} ::/EFI/BOOT
    COMMAND env MTOOLS_SKIP_CHECK=1 mcopy -i ${EFI_IMG} ${EFI_APP} ::/EFI/BOOT/BOOTX64.EFI
    DEPENDS ${EFI_APP}
)

add_custom_command(
    OUTPUT ${BOOT_ISO}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/EFI/BOOT
    COMMAND ${CMAKE_COMMAND} -E copy ${EFI_IMG} ${ISO_ROOT}/efi.img
    COMMAND ${CMAKE_COMMAND} -E copy ${EFI_APP} ${ISO_ROOT}/EFI/BOOT/BOOTX64.EFI
    COMMAND xorriso -as mkisofs
        -V PICOCD
        -o ${BOOT_ISO}
        -eltorito-platform efi
        -no-emul-boot
        -e efi.img
        ${ISO_ROOT}
    DEPENDS ${EFI_IMG}
)

# Force `const` so the ISO lands in .rodata/flash, not .data/RAM.
add_custom_command(
    OUTPUT ${ISO_HEADER}
    COMMAND xxd -i -n iso_image ${BOOT_ISO} > ${ISO_HEADER}
    COMMAND sed -i "s/^unsigned char iso_image/const unsigned char iso_image/" ${ISO_HEADER}
    COMMAND sed -i "s/^unsigned int iso_image_len/const unsigned int iso_image_len/" ${ISO_HEADER}
    DEPENDS ${BOOT_ISO}
)

add_custom_target(generate_iso DEPENDS ${ISO_HEADER})

add_executable(picobootcd
    main.c
    usb_descriptors.c
    msc_disk.c
)

add_dependencies(picobootcd generate_iso)

target_include_directories(picobootcd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(picobootcd
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(picobootcd)
