#include "ww5500.h"

#include <time.h>
#include "esp_random.h"
//#include "w5500.h"
//#include "esp_eth_mac_w5500



#define ETH_MISO 16
#define ETH_MOSI 17
#define ETH_CS 18
#define ETH_HOST (spi_host_device_t) 1
#define ETH_SCLK 5
#define ETH_INT 35 
#define ETH_CLOCK_MHZ 8
#define ETH_SPI_PHY_RST0_GPIO 19
#define ETH_SPI_PHY_ADDR0 1


void WW5500::reset(void) 
{
    //esp_err_t tmp;
    ESP_LOGE("ETHERNET","Ethernet RESET");
    /*
   esp_eth_stop(eth_handle);
   
   gpio_set_level((gpio_num_t)ETH_SPI_PHY_RST0_GPIO,0);
   vTaskDelay(50/portTICK_PERIOD_MS);
   gpio_set_level((gpio_num_t)ETH_SPI_PHY_RST0_GPIO,1);  
   vTaskDelay(50/portTICK_PERIOD_MS);

    mac_config();
    phy_config();

    eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    tmp = esp_eth_driver_install(&eth_config, &eth_handle);
    if(tmp!=ESP_OK) printf("ETH DRIVER HATA");

    set_macid();
    enc28j60_set_phy_duplex(phy, ETH_DUPLEX_FULL);
    set_ip();

    tmp = esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
    if(tmp!=ESP_OK) printf("ATTACH HATA");

   esp_eth_start(eth_handle);
   enc28j60_set_phy_duplex(phy, ETH_DUPLEX_FULL);
   */
   
}

typedef struct {
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
}spi_eth_module_config_t;

esp_err_t WW5500::start(home_network_config_t cnf, home_global_config_t *gcnf)
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
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {};
    cfg_spi.base = &esp_netif_config;
    cfg_spi.stack = ESP_NETIF_NETSTACK_DEFAULT_ETH;

    esp_netif_t *eth_netif_spi = {};
    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];
        itoa(0, num_str, 10);
        strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
        strcat(strcpy(if_desc_str, "eth"), num_str);
        esp_netif_config.if_key = if_key_str;
        esp_netif_config.if_desc = if_desc_str;
        esp_netif_config.route_prio = 30 ;
        
    eth_netif_spi = esp_netif_new(&cfg_spi);
    set_ip(eth_netif_spi);

    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

    spi_device_handle_t spi_handle = {};
    spi_bus_config_t buscfg = {};
        buscfg.miso_io_num = ETH_MISO;
        buscfg.mosi_io_num = ETH_MOSI;
        buscfg.sclk_io_num = ETH_SCLK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config;
    spi_eth_module_config.int_gpio = ETH_INT;
    spi_eth_module_config.phy_addr = ETH_SPI_PHY_ADDR0;
    spi_eth_module_config.phy_reset_gpio = ETH_SPI_PHY_RST0_GPIO;
    spi_eth_module_config.spi_cs_gpio = ETH_CS;

    esp_eth_mac_t *mac_spi;
    esp_eth_phy_t *phy_spi;
    esp_eth_handle_t eth_handle_spi = { NULL };
    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 16;
    devcfg.address_bits = 8;
    devcfg.mode = 0;
    devcfg.clock_speed_hz = 8 * 1000 * 1000,
    devcfg.queue_size = 20;

    devcfg.spics_io_num = spi_eth_module_config.spi_cs_gpio;

    ESP_ERROR_CHECK(spi_bus_add_device(ETH_HOST, &devcfg, &spi_handle));
        // w5500 ethernet driver is based on spi driver
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_HOST, &devcfg);

        // Set remaining GPIO numbers and configuration used by the SPI module
    w5500_config.int_gpio_num = ETH_INT;
    phy_config_spi.phy_addr = spi_eth_module_config.phy_addr;
    phy_config_spi.reset_gpio_num = ETH_SPI_PHY_RST0_GPIO;

    mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
    phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);
    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));

    set_macid(mac_spi);
    

    tmp = esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi));
    if(tmp!=ESP_OK) return tmp;


    tmp = esp_eth_start(eth_handle_spi);
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


esp_err_t WW5500::set_ip(esp_netif_t * netif)
{
    esp_err_t tmp=ESP_OK;
    
    if (netif)
    {
        memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        if (mConfig.ipstat==STATIC_IP)
            {  
                esp_netif_dhcpc_stop(netif);
                info_t.ip.addr = esp_ip4addr_aton((const char *)mConfig.ip);
                info_t.netmask.addr = esp_ip4addr_aton((const char *)mConfig.netmask);
                info_t.gw.addr = esp_ip4addr_aton((const char *)mConfig.gateway);
                ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr((const char *)mConfig.dns), ESP_NETIF_DNS_MAIN));
                ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr((const char *)mConfig.backup_dns), ESP_NETIF_DNS_BACKUP));
                ESP_LOGI("W5500","Static IP");
            } else ESP_LOGI("W5500","Dynamic IP");
            esp_netif_set_ip_info(netif, &info_t);
    }
    return tmp;
}

void WW5500::set_macid(esp_eth_mac_t *handle)
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

    //ESP_ERROR_CHECK(esp_eth_ioctl(handle, ETH_CMD_S_MAC_ADDR,  aaa)); 
    handle->set_addr(handle, (uint8_t *) &aaa); 
/*
    ESP_ERROR_CHECK(esp_eth_ioctl(handle, ETH_CMD_S_MAC_ADDR, (uint8_t[]) {
            0x02, 0x00, 0x00, 0x12, 0x34, 0x56 
        }));
*/
    if (gConfig->random_mac)
        ESP_LOGE("W5500", "NEW MAC : %02X %02X %2X %02X %02X %02X",aaa[0],aaa[1],aaa[2],aaa[3],aaa[4],aaa[5]);
}

void WW5500::mac_config(void)
{
   // eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
 //   mac_config.smi_mdc_gpio_num = -1;  
 //   mac_config.smi_mdio_gpio_num = -1;
   // mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);  
}

void WW5500::phy_config(void)
{
   // eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
   // phy_config.autonego_timeout_ms = 0; 
   // phy_config.reset_gpio_num = -1; 
   // phy = esp_eth_phy_new_enc28j60(&phy_config);
}

esp_err_t WW5500::spi_config(void)
{
    esp_err_t tmp=ESP_OK;
    return tmp;
}

esp_err_t WW5500::set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}