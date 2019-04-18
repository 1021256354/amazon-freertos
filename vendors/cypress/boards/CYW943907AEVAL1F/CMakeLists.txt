cmake_minimum_required(VERSION 3.13)

set(cypress94390_module_dir "${AFR_VENDORS_DIR}/cypress/boards/CYW943907AEVAL1F")
set(cypress94390_ports_dir "${cypress94390_module_dir}/ports")
set(wicedsdk_dir "${AFR_VENDORS_DIR}/cypress/WICED_SDK")
set(cypress94390_demos_dir "${cypress94390_module_dir}/aws_demos/common")
set(cypress94390_tests_dir "${cypress94390_module_dir}/aws_tests/common")

if(AFR_IS_TESTING)
    set(cypress94390_app_dir "${cypress94390_tests_dir}")
else()
    set(cypress94390_app_dir "${cypress94390_demos_dir}")
endif()

set(cypress_board "CYW943907AEVAL1F")
# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS Console metadata
# -------------------------------------------------------------------------------------------------
afr_set_board_metadata(ID "CYW943907AEVAL1F")
afr_set_board_metadata(DISPLAY_NAME "Cypress CYW943907AEVAL1F Development Kit")
afr_set_board_metadata(DESCRIPTION "The Cypress CYW943907AEVAL1F Evaluation Kit")
afr_set_board_metadata(VENDOR_NAME "Cypress")
afr_set_board_metadata(FAMILY_NAME "ARM")
afr_set_board_metadata(IS_ACTIVE "TRUE")
afr_set_board_metadata(DATA_RAM_MEMORY "640KB")
afr_set_board_metadata(PROGRAM_MEMORY "2MB")
afr_set_board_metadata(CODE_SIGNER "AmazonFreeRTOS-Default")
afr_set_board_metadata(SUPPORTED_IDE "WICED-Studio6.2.1")
afr_set_board_metadata(RECOMMENDED_IDE "WICED-Studio6.2.1")
afr_set_board_metadata(IDE_WicedStudio_NAME "WICED Studio by Cypress")
afr_set_board_metadata(IDE_WicedStudio_COMPILER "GCC")
afr_set_board_metadata(IDE_WicedStudio_RECOMMENDED "TRUE")
afr_set_board_metadata(IDE_WicedStudio_PROJECT_LOCATION "${AFR_ROOT_DIR}/demos/cypress/CYW943907AEVAL1F/wicedstudio")
afr_set_board_metadata(DEMO_COMMON_LOCATION "${cypress94390_demos_dir}")
afr_set_board_metadata(THIRD_PARTY_LIB_LOCATION "${wicedsdk_dir}")

# -------------------------------------------------------------------------------------------------
# Compiler settings
# -------------------------------------------------------------------------------------------------
afr_mcu_port(compiler)

# Compile definitions/macros
target_compile_definitions(
     AFR::compiler::mcu_port
     INTERFACE AmazonFreeRTOS
)
set(
    defined_symbols
    SFLASH_APPS_HEADER_LOC=0x00010000 
    PLATFORM_NO_DDR=1 
    PLATFORM_NO_GMAC=1 
    OTP_WORD_NUM_SHA_KEY=258 
    OTP_WORD_NUM_SHA_KEY_R=242 
    OTP_WORD_NUM_AES_KEY=282 
    OTP_WORD_NUM_AES_KEY_R=274 
    SECUREBOOT_SHA_KEY_SIZE=32 
    GSIO_I2C_REPEATED_START_NEEDS_GPIO=1 
    PLATFORM_HIB_WAKE_CTRL_FREQ_BITS_FLIPPED 
    CRLF_STDIO_REPLACEMENT 
    WICED_DISABLE_CONFIG_TLS 
    PLATFORM_SECURESFLASH_ENABLED=0 
    PLATFORM_SECUREDCT_ENABLED=0 
    NORMAL_IMAGE_DCT_1_AREA_BASE=0x00008000 
    MIN_WM8533_DB_LEVEL=-53.0 
    MAX_WM8533_DB_LEVEL=6.0 
    PLATFORM_ALP_CLOCK_RES_FIXUP=0 
    MAX_WATCHDOG_TIMEOUT_SECONDS=22 
    MAX_WAKE_FROM_SLEEP_DELAY_TICKS=500 
    PLATFORM_L1_CACHE_SHIFT=5 
    PLATFORM_SUPPORTS_LOW_POWER_MODES
    BCM43909 
    ENABLE_ARM_DSP_INSTRUCTIONS
    PLAT_NOTIFY_FREE
    USING_WICEDFS
    #wifi_firmware_image="${wicedsdk_dir}/resources/firmware/43909/43909B0.bin"
    WWD_STARTUP_DELAY=10
    BOOTLOADER_MAGIC_NUMBER=0x4d435242
    NETWORK_LwIP=1 
    LwIP_VERSION=\"v2.0.3\"
    RTOS_FreeRTOS=1 
    configUSE_MUTEXES 
    configUSE_RECURSIVE_MUTEXES 
    FreeRTOS_VERSION=\"v10.0.0\" 
    SFLASH_SUPPORT_MACRONIX_PARTS 
    USING_EXTERNAL_ADC 
    WICED_AMAZON_FREERTOS_SDK 
    WICED_SDK_WIFI_CONFIG_DCT_H=\"${wicedsdk_dir}/include/default_wifi_config_dct.h\"
    WICED_SDK_BT_CONFIG_DCT_H=\"${wicedsdk_dir}/include/default_bt_config_dct.h\"
    __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=32768
    BCMDRIVER
    BCM_WICED
    BCM_CPU_PREFETCH
    BCMCHIPID=BCM43909_CHIP_ID
    UNRELEASEDCHIP
    BCMM2MDEV_ENABLED
    CFG_GMAC
    BCMCHIPREV=2
    NO_WIFI_FIRMWARE
)

target_compile_definitions(
    AFR::compiler::mcu_port
    INTERFACE $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${defined_symbols}>
)

# Kernel
afr_mcu_port(kernel)
set(toolchain_path "${AFR_TOOLCHAIN_PATH}/../lib/gcc/arm-none-eabi/7.2.1")
set(include_path "${AFR_TOOLCHAIN_PATH}/../arm-none-eabi/include")
set(adc_drivers_path "${AFR_TOOLCHAIN_PATH}/../../../../libraries/drivers/adc/MAX11615")

set(
    kernel_inc_dirs
    "${AFR_KERNEL_DIR}/portable/ThirdParty/GCC/Wiced_CY"
    "${wicedsdk_dir}/include"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/spi_flash"
    "${wicedsdk_dir}/libraries/utilities/ring_buffer" 
    "${wicedsdk_dir}/WICED/platform/GCC/" 
    "${wicedsdk_dir}/libraries/utilities/crc/."
    "${wicedsdk_dir}/libraries/utilities/TLV/."
    "${wicedsdk_dir}/WICED/network/LwIP/WWD/FreeRTOS/."
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/."
    "${wicedsdk_dir}/WICED/platform/MCU/."
    "${wicedsdk_dir}/WICED/platform/ARM_CR4"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/WAF"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/include"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/tlsf"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/spi_flash" 
    "${wicedsdk_dir}/platforms/CYW943907AEVAL1F"
    "${wicedsdk_dir}/WICED/platform/include"
    "${wicedsdk_dir}/WICED/WWD/"
    "${wicedsdk_dir}/WICED/WWD/include" 
    "${wicedsdk_dir}/WICED/WWD/include/network" 
    "${wicedsdk_dir}/WICED/WWD/include/RTOS"
    "${wicedsdk_dir}/WICED/WWD/internal/bus_protocols/SoC/43909"
    "${wicedsdk_dir}/WICED/WWD/internal/chips/4390x" 
    "${wicedsdk_dir}/libraries/utilities/linked_list" 
    "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WICED" 
    "${wicedsdk_dir}/WICED/network/LwIP/WWD/." 
    "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WICED/."
    "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WWD/."
    "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WWD/ARM_CR4"
    "${wicedsdk_dir}/libraries/utilities/wifi/."
    "${wicedsdk_dir}/WICED/."
    "${wicedsdk_dir}/WICED/platform/include"
    "${wicedsdk_dir}/libraries/filesystems/wicedfs/src"
    "${wicedsdk_dir}/libraries/utilities/wifi"
    "${wicedsdk_dir}/WICED/network/LwIP/WICED"
    "${wicedsdk_dir}/platforms/CYW943907AEVAL1F/."
    "${wicedsdk_dir}/platforms/CYW943907AEVAL1F/board_revision/P103"
    "${AFR_3RDPARTY_DIR}/jsmn"
    "${AFR_3RDPARTY_DIR}/pkcs11"
    "${AFR_3RDPARTY_DIR}/lwip/src/include"
    "${AFR_3RDPARTY_DIR}/lwip/src/include/lwip"
    "${AFR_3RDPARTY_DIR}/lwip/src/portable"
    "${AFR_3RDPARTY_DIR}/lwip/src/portable/cypress/CYW943907AEVAL1F"
    "${AFR_3RDPARTY_DIR}/lwip/src/portable/cypress/CYW943907AEVAL1F/include"
    "${AFR_3RDPARTY_DIR}/lwip/src/portable/arch"
    "${AFR_3RDPARTY_DIR}/mbedtls/include"
    "${cypress94390_app_dir}/application_code/cypress_code/include"
    "${cypress94390_app_dir}/config_files"
    "$<IF:${AFR_IS_TESTING},${AFR_TESTS_DIR},${AFR_DEMOS_DIR}>/include"
    "${toolchain_path}/include"
    "${toolchain_path}/include-fixed"
    "${include_path}"
    "${adc_drivers_path}"
)

afr_glob_src(apps_src DIRECTORY "${wicedsdk_dir}/apps" RECURSE)
afr_glob_src(lwip_api_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/api")
afr_glob_src(lwip_apps_httpd_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/httpd")
afr_glob_src(lwip_apps_lwiperf_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/lwiperf")
afr_glob_src(lwip_apps_mdns_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/mdns")
afr_glob_src(lwip_apps_netbiosns_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/netbiosns")
afr_glob_src(lwip_apps_snmp_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/snmp")
afr_glob_src(lwip_apps_sntp_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/sntp")
afr_glob_src(lwip_apps_tftp_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/apps/tftp")
afr_glob_src(lwip_core_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/core" RECURSE)
afr_glob_src(lwip_netif_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/netif" RECURSE)
afr_glob_src(lwip_arch_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/portable/arch")
afr_glob_src(lwip_portable_src DIRECTORY "${AFR_3RDPARTY_DIR}/lwip/src/portable/cypress/CYW943907AEVAL1F" RECURSE)
afr_glob_src(wwd_bus_protocol_chips_src DIRECTORY "${wicedsdk_dir}/WICED/internal")
afr_glob_src(wwd_internal_src DIRECTORY "${wicedsdk_dir}/WICED/WWD/internal" RECURSE)
afr_glob_src(wwd_FreeRTOS DIRECTORY "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WWD" RECURSE)
afr_glob_src( shared_src DIRECTORY "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/shared")


set(platform_src
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/vector_table_GCC.S"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/start_GCC.S"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/BCM4390x_platform.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/WAF/waf_platform.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_unhandled_isr.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_vector_table.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_filesystem.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_tick.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_chipcommon.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_deep_sleep.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/wwd_platform.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_audio_info.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/platform_gspi_slave.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/wwd_m2m.c"
    "${wicedsdk_dir}/WICED/platform/MCU/platform_resource.c"
    "${wicedsdk_dir}/WICED/platform/MCU/platform_stdio.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wiced_platform_common.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wiced_apps_common.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wiced_waf_common.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wiced_dct_external_common.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wiced_dct_update.c"
    "${wicedsdk_dir}/WICED/platform/MCU/platform_nsclock.c"
    "${wicedsdk_dir}/WICED/platform/ARM_CR4/exception_handlers.c"
    "${wicedsdk_dir}/WICED/platform/ARM_CR4/crt0_GCC.c"
    "${wicedsdk_dir}/WICED/platform/ARM_CR4/platform_cache.c"
    "${wicedsdk_dir}/WICED/platform/ARM_CR4/platform_cache_asm.S"
)

set( peripheral_src
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_watchdog.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_chipcontrol.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_m2m.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_pinmux.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_gpio.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_spi_i2c.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_mcu_powersave.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_cores_powersave.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_backplane.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_rtc.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_hib.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_otp.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_8021as_clock.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_audio_timer.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_spi_flash.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_ascu.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_pwm.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/platform_adc.c"
)

target_include_directories(
    AFR::kernel::mcu_port
    INTERFACE
        ${kernel_inc_dirs}
)

target_sources(
    AFR::kernel::mcu_port
    INTERFACE
    "${AFR_KERNEL_DIR}/portable/ThirdParty/GCC/Wiced_CY/portASM.S"
    "${AFR_KERNEL_DIR}/portable/MemMang/heap_4.c"
    "${AFR_KERNEL_DIR}/portable/ThirdParty/GCC/Wiced_CY/port.c"
    "${cypress94390_app_dir}/application_code/cypress_code/app_dct.c"
    "${lwip_api_src}"
    "${lwip_apps_httpd_src}"
    "${lwip_apps_lwiperf_src}"
    "${lwip_apps_mdns_src}"
    "${lwip_apps_netbiosns_src}"
    "${lwip_apps_snmp_src}"
    "${lwip_apps_sntp_src}"
    "${lwip_apps_tftp_src}"
    "${lwip_core_src}"
    "${lwip_netif_src}"
    "${lwip_arch_src}"
    "${lwip_portable_src}"
    "${peripheral_src}"
    "${wicedsdk_dir}/WICED/RTOS/wiced_rtos_common.c"
    "${wicedsdk_dir}/WICED/RTOS/FreeRTOS/WICED/wiced_rtos.c"
    "${wicedsdk_dir}/WICED/network/wiced_network_common.c"
    "${wicedsdk_dir}/WICED/network/wiced_tcpip_common.c"
    "${wicedsdk_dir}/WICED/network/LwIP/WICED/wiced_network.c"
    "${wicedsdk_dir}/WICED/network/LwIP/WICED/tcpip.c"
    "${wicedsdk_dir}/WICED/network/LwIP/WICED/wiced_ping.c"
    "${wwd_FreeRTOS}"
    "${wwd_internal_src}"
    "${wicedsdk_dir}/libraries/filesystems/wicedfs/src/wicedfs.c"
    "${wicedsdk_dir}/libraries/filesystems/wicedfs/wicedfs_drivers.c"
    "${wicedsdk_dir}/libraries/filesystems/wicedfs/src/unit/wicedfs_unit_images.c"
    "${platform_src}"
    "${wicedsdk_dir}/libraries/utilities/crc/crc.c"
    "${wicedsdk_dir}/libraries/utilities/TLV/tlv.c"
    "${wicedsdk_dir}/libraries/utilities/ring_buffer/ring_buffer.c"
    "${wicedsdk_dir}/WICED/platform/GCC/cxx_funcs.c"
    "${wicedsdk_dir}/platforms/CYW943907AEVAL1F/platform.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/spi_flash/spi_flash.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/spi_flash/spi_flash_compatible.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/uart/platform_uart.c"
    "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/peripherals/crypto/tiny_crypto/platform_tiny_crypto.c"
    "${shared_src}"
    "${wicedsdk_dir}/WICED/internal/wiced_core.c"
    "${wicedsdk_dir}/WICED/internal/wiced_low_power.c"
    "${wicedsdk_dir}/WICED/internal/waf.c"
    "${wicedsdk_dir}/WICED/internal/wifi.c"
    "${wicedsdk_dir}/WICED/internal/wiced_crypto.c"
    "${wicedsdk_dir}/WICED/internal/system_monitor.c"
    "${wicedsdk_dir}/WICED/internal/wiced_lib.c"
    "${wicedsdk_dir}/WICED/platform/MCU/wwd_resources.c"
    "${wicedsdk_dir}/libraries/utilities/wifi/wifi_utils.c"
    "${apps_src}"
)

set(common_flags "-mthumb" "-mcpu=cortex-r4" "-mthumb-interwork" "-mlittle-endian")
set(c_flags "-Wall" "-Waddress" "-Wundef" "-fsigned-char" "-ffunction-sections" "-fdata-sections" "-fno-common" "-std=gnu11" "-fdiagnostics-color" "-Os" )

set(linker_flags "-Wl,-static" "-Wl,--warn-common" "-Wl,--gc-sections" "-Wl,-Os" "-Wl,--cref" "-mthumb" "-mcpu=cortex-r4" "-mthumb-interwork" "-Wl,-A,thumb" "-Wl,-z,max-page-size=0x10" "-Wl,-z,common-page-size=0x10" "-mlittle-endian" "-nostartfiles" "-Wl,--defsym,START_STACK_SIZE=4000" "-Wl,--defsym,FIQ_STACK_SIZE=0" "-Wl,--defsym,IRQ_STACK_SIZE=1024" "-Wl,--defsym,SYS_STACK_SIZE=0") 

# Compiler flags
target_compile_options(
    AFR::compiler::mcu_port
    INTERFACE
        ${common_flags}
        $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${c_flags}>
)


# Linker flags
target_link_options(
    AFR::compiler::mcu_port
    INTERFACE ${linker_flags}
)

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS portable layers
# -------------------------------------------------------------------------------------------------

# WiFi
afr_mcu_port(wifi)
target_sources(
    AFR::wifi::mcu_port
    INTERFACE "${cypress94390_ports_dir}/wifi/aws_wifi.c"
)

# PKCS11
afr_mcu_port(pkcs11)
target_sources(
    AFR::pkcs11::mcu_port
    INTERFACE 
        "${cypress94390_ports_dir}/pkcs11/aws_pkcs11_pal.c"
        "${cypress94390_ports_dir}/pkcs11/hw_poll.c"
        "${AFR_MODULES_PORTS_DIR}/pkcs11/mbedtls/aws_pkcs11_mbedtls.c"
)
# Link to AFR::pkcs11_mbedtls if you want to use default implementation based on mbedtls.
target_link_libraries(
    AFR::pkcs11::mcu_port
    INTERFACE 
        3rdparty::mbedtls
        AFR::crypto
)

# Secure sockets
afr_mcu_port(secure_sockets)
target_link_libraries(
    AFR::secure_sockets::mcu_port
    INTERFACE 
        AFR::tls
        AFR::wifi
)

target_sources(
    AFR::secure_sockets::mcu_port
    INTERFACE "${AFR_MODULES_PORTS_DIR}/secure_sockets/lwip/aws_secure_sockets.c"
)

# -------------------------------------------------------------------------------------------------
# Amazon FreeRTOS demos and tests
# -------------------------------------------------------------------------------------------------
file(COPY "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/BCM43907/GCC_tiny_bootloader_memory.ld" DESTINATION . )
file(COPY "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/BCM43907/GCC_app_without_rom_memory.ld" DESTINATION .)
file(COPY "${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/GCC/ddr.ld" DESTINATION GCC/.)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

if(AFR_IS_TESTING)
    set(exe_target aws_tests)
else()
    set(exe_target aws_demos)
endif()

afr_glob_src(network_manager_src DIRECTORY "${AFR_DEMOS_DIR}/network_manager")
afr_glob_src(board_code_src DIRECTORY "${cypress94390_app_dir}/application_code/cypress_code")
add_executable(
    ${exe_target}
    ${board_code_src}
    ${network_manager_src}
    "${cypress94390_app_dir}/application_code/main.c"
    "${cypress94390_ports_dir}/pkcs11/hw_poll.c"
)

target_link_libraries(
    ${exe_target}
    PRIVATE
        AFR::kernel
        AFR::wifi
        AFR::utils
        "${AFR_ROOT_DIR}/libgcc.a"
        "${AFR_ROOT_DIR}/libm.a"
        "${AFR_ROOT_DIR}/libc.a"
        "${AFR_ROOT_DIR}/libstdc++.a"
        -Wl,-T"${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/GCC/dct.ld"
        -Wl,-T"${wicedsdk_dir}/WICED/platform/MCU/BCM4390x/GCC/app_without_rom.ld"
)

find_program(gcc_objcopy arm-none-eabi-objcopy)

if(NOT gcc_objcopy)
    message(FATAL_ERROR "Cannot find arm-none-eabi-objcopy.")
endif()

set(output_file "$<TARGET_FILE_DIR:${exe_target}>/${exe_target}.hex")
add_custom_command(
    TARGET ${exe_target} POST_BUILD
    COMMAND "${gcc_objcopy}" -v -O ihex "$<TARGET_FILE:${exe_target}>"
    "${output_file}"
)
