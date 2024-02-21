# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.1.1.0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(N1CPU01)
spiffs_create_partition_image(storage data FLASH_IN_PROJECT)

#1.1.0.3
  # Auto MAC ozelliği Eklendi
  # dns server eklendi
  # MQTT Will mesajlarına location ve tarih eklendi 
#1.1.0.4
  # Açılıştaki Ethernet ERROR hatası kaldırıldı
  # Açılıştaki Mqtt ERROR hatası kaldırıldı
  # Error gelirse ve ekran kapalı ise Ekranın açılması sağlandı. 
  # MQTT ve Ethernet errorlar Ekranda priority ile yazdırıldı (Eth öncelikli)
  # Açılışta RTC Aktif ve doğru zaman içeriyorsa ESP zamanı RTC ye eşitlenerek  
     # >> SNTP nin internet baglantısı yapması engellendi
  # Açılış ekranına eth aksiyonları eklendi (Hızlı geçtiği için zor görünür)
#1.1.0.5
  # Konsol ping sistemi eklendi.
  # Ethernet.h devreye alındı. Ethernet reset ilave edildi.
#1.0.0.6
  # Ping reset counter eklendi. Web sayfasından izlenebilir.         
#1.1.0.7
  # OTA Upgrade sistemi eklendi. (Remote upgrade)
#1.1.0.8
  # Test Upgrade 
  # web sayfasına firmware version eklendi 
  # ping sistem için başlangıç gecikmesi eklendi
#1.1.0.9
  # Test Upgrade  
#1.1.0.10
  # Ethetnet netif spi moda alınıp netif e route priority eklendi
  # Web sayfasına time sync eklendi. Web sayfasında Time sync işaretlendiğinde 
     # Tarih ve zaman sntp üzerinden update edilir. 
#1.1.0.11
  # MQTT server url üzerinden login olacak şekilde düzenlendi. 
#1.1.0.12
  # MQTT logları açılarak MQTT server URI üzerinden login olacak şekilde düzenlendi
  # Tusa basıldığında Ekran açılma problemi çözüldü
#1.1.0.13
  # main.cpp ye icemqtt.com.tr için dnsquery eklendir ve loglara yazdırıldı. 
  # loglar yönlendirildi

#1.1.1.0
  # IDF 5.1.2 ye yükseltildi.  