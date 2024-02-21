

set(min_supported_idf_version "4.4.0")
set(max_supported_idf_version "4.4.99")
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")

set(LIBRARY_SRCS
  library/button/button_8574.c
  library/button/button_adc.c
  library/button/button_gpio.c
  library/button/iot_button.c

  library/out/iot_out.c
  library/out/out_8574.c
  library/out/out_gpio.c

  library/pcf/i2cdev.c
  library/pcf/ice_pcf8574.c
  library/pcf/pcf8563.c
  library/pcf/i2c_gateway.c
  library/pcf/ice_ssd1306.cpp
  
  library/rs485/rs485.cpp
  library/storage/storage.cpp
  library/network/network.cpp
  library/jsontool/jsontool.cpp
  library/core/core.cpp
  library/classes/classes.cpp
  library/lamp/lamp.cpp
  library/priz/priz.cpp
  library/kontaktor/kontaktor.cpp
  library/onoff/onoff.cpp
  library/curtain/curtain.cpp
  library/air/air.cpp
  library/security/security.cpp
  #library/htime/htime.cpp
  #library/atime/ESP32Time.cpp
  library/water/water.cpp
  library/gas/gas.cpp
  library/tcpserver/tcpserver.cpp
  library/cihazlar/cihazlar.cpp
  library/broadcast/broadcast.cpp
  library/iptool/iptool.cpp
  library/uart/uart.cpp
  library/mwater/mwater.cpp
  library/tmrole/tmrole.cpp
  library/wifi_now/wifi_now.cpp
  library/asansor/asansor.cpp

  #library/fonts/fonts.c 
  #library/ssd1306/OLEDDisplay.cpp
  #library/ssd1306/OLEDDisplayFonts.cpp

  library/enc28j60/esp_eth_mac_enc28j60.c
  library/enc28j60/esp_eth_phy_enc28j60.c
  #library/enc28j60/eth_encj60.cpp
  library/enc28j60/ethernet.cpp

  #library/ww5500/esp_eth_mac_w5500.c
  #library/ww5500/esp_eth_phy_w5500.c
  library/ww5500/ww5500.cpp

  library/mqtt/mqtt.cpp

  )
set(includedirs
  library/core
  library/button
  library/out
  library/pcf
  library/rs485
  library/storage
  library/network
  library/jsontool
  library/classes
  library/lamp
  library/priz
  library/kontaktor
  library/onoff
  library/curtain
  library/air
  library/security
  #library/htime
  #library/atime
  library/water
  library/gas
  library/tcpserver
  library/cihazlar
  library/broadcast
  library/iptool
  library/uart
  library/mwater
  library/tmrole
  library/wifi_now

  library/asansor
  #library/ssd1306
  #library/fonts
  library/enc28j60
  library/ww5500
  library/mqtt
  )

set(srcs ${LIBRARY_SRCS})
set(requires esp_adc nvs_flash spiffs json mqtt driver esp_timer vfs esp_eth esp_wifi)

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${srcs} REQUIRES ${requires})
