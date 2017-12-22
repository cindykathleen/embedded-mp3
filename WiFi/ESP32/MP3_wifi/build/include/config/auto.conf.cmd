deps_config := \
	/home/dom/esp/esp-idf/components/app_trace/Kconfig \
	/home/dom/esp/esp-idf/components/aws_iot/Kconfig \
	/home/dom/esp/esp-idf/components/bt/Kconfig \
	/home/dom/esp/esp-idf/components/esp32/Kconfig \
	/home/dom/esp/esp-idf/components/ethernet/Kconfig \
	/home/dom/esp/esp-idf/components/fatfs/Kconfig \
	/home/dom/esp/esp-idf/components/freertos/Kconfig \
	/home/dom/esp/esp-idf/components/heap/Kconfig \
	/home/dom/esp/esp-idf/components/libsodium/Kconfig \
	/home/dom/esp/esp-idf/components/log/Kconfig \
	/home/dom/esp/esp-idf/components/lwip/Kconfig \
	/home/dom/esp/esp-idf/components/mbedtls/Kconfig \
	/home/dom/esp/esp-idf/components/openssl/Kconfig \
	/home/dom/esp/esp-idf/components/pthread/Kconfig \
	/home/dom/esp/esp-idf/components/spi_flash/Kconfig \
	/home/dom/esp/esp-idf/components/spiffs/Kconfig \
	/home/dom/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/dom/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/dom/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/dom/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/dom/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/dom/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
