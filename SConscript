from building import *
import os

cwd = GetCurrentDir()
group = []
src = ["wifi_ifx_cyw43012.c"]
path = [cwd]

src += ["wifi-host-driver/WiFi_Host_Driver/resources/clm/COMPONENT_43012/43012C0-mfgtest_clm_blob.c",
        "wifi-host-driver/WiFi_Host_Driver/resources/clm/COMPONENT_43012/43012C0_clm_blob.c",
        "wifi-host-driver/WiFi_Host_Driver/resources/firmware/COMPONENT_43012/43012C0-mfgtest_bin.c",
        "wifi-host-driver/WiFi_Host_Driver/resources/firmware/COMPONENT_43012/43012C0_bin.c",
        "wifi-host-driver/WiFi_Host_Driver/resources/resource_imp/whd_resources.c",
        "wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/whd_bus.c",
        "wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/whd_bus_common.c",
        "wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/whd_bus_m2m_protocol.c",
        "wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/whd_bus_sdio_protocol.c",
        "wifi-host-driver/WiFi_Host_Driver/src/bus_protocols/whd_bus_spi_protocol.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_ap.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_buffer_api.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_cdc_bdc.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_chip.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_chip_constants.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_clm.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_debug.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_events.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_logging.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_management.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_proto.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_network_if.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_resource_if.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_sdpcm.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_thread.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_utils.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_wifi.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_wifi_api.c",
        "wifi-host-driver/WiFi_Host_Driver/src/whd_wifi_p2p.c"]

path += [
        cwd + "/wifi-host-driver/WiFi_Host_Driver/inc",
        cwd + "/wifi-host-driver/WiFi_Host_Driver",
        cwd + "/wifi-host-driver",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/clm/COMPONENT_43012",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/clm",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/firmware/COMPONENT_43012",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/firmware",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/resource_imp",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/src/bus_protocols",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/src",
        cwd + "/wifi-host-driver/WiFi_Host_Driver/src/include"]
if GetDepend('COMPONENT_CYWL6302'):
    path += [cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012/COMPONENT_CYWL6302"]
elif GetDepend('COMPONENT_AW_AM497'):
    path += [cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012/COMPONENT_AW-AM497"]
elif GetDepend('COMPONENT_CYSBSYS_RP01'):
    path += [cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012/COMPONENT_CYSBSYS-RP01"]
elif GetDepend('COMPONENT_WM_BAC_CYW_50'):
    path += [cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012/COMPONENT_WM-BAC-CYW-50"]
else:
    path += [cwd + "/wifi-host-driver/WiFi_Host_Driver/resources/nvram/COMPONENT_43012/COMPONENT_MURATA-1LV"]

src += ["abstraction-rtos/source/COMPONENT_FREERTOS/cyabs_freertos_common.c",
        "abstraction-rtos/source/COMPONENT_FREERTOS/cyabs_freertos_helpers.c",
        "abstraction-rtos/source/COMPONENT_FREERTOS/cyabs_rtos_dsram.c",
        "abstraction-rtos/source/COMPONENT_FREERTOS/cyabs_rtos_freertos.c",
        "abstraction-rtos/source/cy_worker_thread.c"]

path += [
        cwd + "/abstraction-rtos/include",
        cwd + "/abstraction-rtos",
        cwd + "/abstraction-rtos/include/COMPONENT_FREERTOS"]

src += ["whd-bsp-integration/COMPONENT_LWIP/cy_network_buffer_lwip.c",
        "whd-bsp-integration/cybsp_wifi.c"]
path += [cwd + "/whd-bsp-integration"]
group = DefineGroup('cyw43012', src, depend = ['RT_USING_WIFI'], CPPPATH = path)
Return('group')
