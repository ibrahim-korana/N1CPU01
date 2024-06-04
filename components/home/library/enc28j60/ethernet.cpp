
#include "ethernet.h"

#include <time.h>
#include "esp_random.h"
#include <netdb.h>



#define ETH_MISO 16
#define ETH_MOSI 17
#define ETH_CS 18
#define ETH_HOST (spi_host_device_t) 1
#define ETH_SCLK 5
#define ETH_INT 35 
#define ETH_CLOCK_MHZ 8
#define ETH_SPI_PHY_RST0_GPIO 19
#define ETH_SPI_PHY_ADDR0 1


void Ethernet::reset(void) 
{

}

esp_err_t Ethernet::start(home_network_config_t cnf, home_global_config_t *gcnf)
{     
    esp_err_t tmp = ESP_OK;
    mConfig = cnf;
    gConfig = gcnf;
    Event = xEventGroupCreate();
    xEventGroupClearBits(Event, ETH_CONNECTED_BIT | ETH_FAIL_BIT);

    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<ETH_SPI_PHY_RST0_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)ETH_SPI_PHY_RST0_GPIO, 1);


    gpio_install_isr_service(0);
    tmp=esp_netif_init();
    if(tmp!=ESP_OK) return tmp;

    //Netif create
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&netif_cfg);

    set_ip(); 

    spi_bus_config_t buscfg = {};
        buscfg.miso_io_num = ETH_MISO;
        buscfg.mosi_io_num = ETH_MOSI;
        buscfg.sclk_io_num = ETH_SCLK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t spi_devcfg = {};
        spi_devcfg.mode = 0;
        spi_devcfg.clock_speed_hz = ETH_CLOCK_MHZ * 1000 * 1000;
        spi_devcfg.spics_io_num = ETH_CS;
        spi_devcfg.queue_size = 20;
        spi_devcfg.cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(ETH_CLOCK_MHZ);

    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(ETH_HOST, &spi_devcfg);
    enc28j60_config.int_gpio_num = ETH_INT;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));   

    set_macid(mac);

    // ENC28J60 Errata #1 check
    if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && ETH_CLOCK_MHZ < 8) {
        ESP_LOGE("ETHERNET", "SPI frequency must be at least 8 MHz for chip revision less than 5");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    eth_duplex_t duplex = ETH_DUPLEX_FULL;
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &duplex));

    tmp = esp_eth_start(eth_handle);
    if(tmp!=ESP_OK) return tmp;
   
    uxBits =  xEventGroupWaitBits(Event,
    		ETH_CONNECTED_BIT | ETH_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            xTicksToWait);
    if (uxBits==0) return ESP_FAIL;
    ld.state = 0;
    ESP_ERROR_CHECK(esp_event_post(LED_EVENTS, LED_EVENTS_ETH, &ld, sizeof(led_events_data_t), portMAX_DELAY));
    xEventGroupClearBits(Event, ETH_CONNECTED_BIT | ETH_FAIL_BIT);

    return tmp; 
}    

esp_err_t Ethernet::set_dns(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

esp_err_t Ethernet::set_ip(void)
{
    esp_err_t tmp=ESP_OK;
    
    if (eth_netif)
    {
        memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        if (mConfig.ipstat==STATIC_IP)
            {  
                esp_netif_dhcpc_stop(eth_netif);
                info_t.ip.addr = esp_ip4addr_aton((const char *)mConfig.ip);
                info_t.netmask.addr = esp_ip4addr_aton((const char *)mConfig.netmask);
                info_t.gw.addr = esp_ip4addr_aton((const char *)mConfig.gateway);
                esp_netif_set_ip_info(eth_netif, &info_t);
                ESP_ERROR_CHECK(set_dns(eth_netif, ipaddr_addr((const char *)mConfig.dns), ESP_NETIF_DNS_MAIN));
                ESP_ERROR_CHECK(set_dns(eth_netif, ipaddr_addr((const char *)mConfig.backup_dns), ESP_NETIF_DNS_BACKUP));
            }           
    }
    return tmp;
}

void Ethernet::set_macid(esp_eth_mac_t *mc)
{
uint8_t aaa[] =  {
        0x0A, 0xAF, 0x08, 0x23, 0x00, 0x00
    };

    if (gConfig->random_mac)
      {
        srand ( esp_random() );
        aaa[0] = 0x0A;
        aaa[1] = rand();
        aaa[2] = rand();
        if (aaa[1]==aaa[2]) aaa[2]=rand();
        aaa[3] = rand();   
        if (aaa[3]==aaa[2] || aaa[3]==aaa[1] )  aaa[3]=rand();
        gConfig->rawmac[0] = aaa[0];    
        gConfig->rawmac[1] = aaa[1];
        gConfig->rawmac[2] = aaa[2];
        gConfig->rawmac[3] = aaa[3];
      } else {
        aaa[0] = gConfig->rawmac[0];
        aaa[1] = gConfig->rawmac[1];
        aaa[2] = gConfig->rawmac[2];
        aaa[3] = gConfig->rawmac[3];
      }  
    aaa[4] = gConfig->blok;
    aaa[5] = gConfig->daire;
    mc->set_addr(mc, aaa);
     
    if (gConfig->random_mac)
        ESP_LOGE("ETHERNET", "NEW MAC : %02X %02X %2X %02X %02X %02X",aaa[0],aaa[1],aaa[2],aaa[3],aaa[4],aaa[5]);
}


