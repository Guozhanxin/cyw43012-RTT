/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-07-25     11714        the first version
 * 2024-05-22     hpmicro      Upgraded WHD to 3.1, fix 'wifi disc' issue, fix 5G Hz
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "[wifi_ifx]"
#define DBG_LEVEL DBG_WARNING
#define DBG_COLOR
#include <rtdbg.h>

#include <FreeRTOS.h>
#include <task.h>

/* Wi-Fi driver header files */
#include <whd_wifi_api.h>
#include "whd_network_types.h"
#include "cy_network_buffer.h"
#include "whd_buffer_api.h"
#include "whd_wlioctl.h"
#include "whd_int.h"
#include <cybsp_wifi.h>

#define EAPOL_PACKET_TYPE                        (0x888E)

struct ifx_wifi
{
    /* inherit from ethernet device */
    struct rt_wlan_device *wlan;
    /* spi transfer layer handle */
    whd_interface_t whd_if;
};

static struct ifx_wifi wifi_sta, wifi_ap;
static rt_bool_t inited = RT_FALSE;
static whd_scan_result_t scan_result;

rt_inline struct ifx_wifi *_GET_DEV(struct rt_wlan_device *wlan)
{
    if (wlan == wifi_sta.wlan)
    {
        return &wifi_sta;
    }
    if (wlan == wifi_ap.wlan)
    {
        return &wifi_ap;
    }
    return RT_NULL;
}

static whd_security_t get_security(rt_wlan_security_t security)
{
    /* security type */
    switch (security)
    {
    case SECURITY_OPEN:
        return WHD_SECURITY_OPEN;
    case SECURITY_WEP_PSK:
        return WHD_SECURITY_WEP_PSK;
    case SECURITY_WEP_SHARED:
        return WHD_SECURITY_WEP_SHARED;
    case SECURITY_WPA_TKIP_PSK:
        return WHD_SECURITY_WPA_TKIP_PSK;
    case SECURITY_WPA_AES_PSK:
        return WHD_SECURITY_WPA_AES_PSK;
    case SECURITY_WPA2_AES_PSK:
        return WHD_SECURITY_WPA2_AES_PSK;
    case SECURITY_WPA2_TKIP_PSK:
        return WHD_SECURITY_WPA2_TKIP_PSK;
    case SECURITY_WPA2_MIXED_PSK:
        return WHD_SECURITY_WPA2_MIXED_PSK;
    case SECURITY_WPS_SECURE:
        return WHD_SECURITY_WPS_SECURE;
    default:
        return WHD_SECURITY_UNKNOWN;
    }
}

static int _ifx_scan_info2rtt(whd_scan_result_t *result_ptr, struct rt_wlan_info *wlan_info)
{
    /* security type */
    switch (result_ptr->security)
    {
    case WHD_SECURITY_OPEN:
        wlan_info->security = SECURITY_OPEN;
        break;
    case WHD_SECURITY_WEP_PSK:
        wlan_info->security = SECURITY_WEP_PSK;
        break;
    case WHD_SECURITY_WEP_SHARED:
        wlan_info->security = SECURITY_WEP_SHARED;
        break;
    case WHD_SECURITY_WPA_TKIP_PSK:
        wlan_info->security = SECURITY_WPA_TKIP_PSK;
        break;
    case WHD_SECURITY_WPA_AES_PSK:
        wlan_info->security = SECURITY_WPA_AES_PSK;
        break;
    case WHD_SECURITY_WPA2_AES_PSK:
        wlan_info->security = SECURITY_WPA2_AES_PSK;
        break;
    case WHD_SECURITY_WPA2_TKIP_PSK:
        wlan_info->security = SECURITY_WPA2_TKIP_PSK;
        break;
    case WHD_SECURITY_WPA2_MIXED_PSK:
        wlan_info->security = SECURITY_WPA2_MIXED_PSK;
        break;
    case WHD_SECURITY_WPS_SECURE:
        wlan_info->security = SECURITY_WPS_SECURE;
        break;
    default:
        wlan_info->security = SECURITY_UNKNOWN;
        break;
    }
    /* 2.4G/5G */
    switch (result_ptr->band)
    {
    case WHD_802_11_BAND_5GHZ:
        wlan_info->band = RT_802_11_BAND_5GHZ;
        break;
    case WHD_802_11_BAND_2_4GHZ:
        wlan_info->band = RT_802_11_BAND_2_4GHZ;
        break;
    default:
        wlan_info->band = RT_802_11_BAND_UNKNOWN;
        break;
    }
    /* maximal data rate */
    wlan_info->datarate = result_ptr->max_data_rate * 1000UL;
    /* radio channel */
    wlan_info->channel = result_ptr->channel;
    /* signal strength */
    wlan_info->rssi = result_ptr->signal_strength;
    /* ssid */
    rt_strncpy(wlan_info->ssid.val, result_ptr->SSID.value, RT_WLAN_SSID_MAX_LENGTH);
    wlan_info->ssid.len =
            result_ptr->SSID.length > RT_WLAN_SSID_MAX_LENGTH ? RT_WLAN_SSID_MAX_LENGTH : result_ptr->SSID.length;

    /* hwaddr */
    rt_strncpy(wlan_info->bssid, result_ptr->BSSID.octet, RT_WLAN_BSSID_MAX_LENGTH);
    wlan_info->hidden = RT_TRUE;
    return RT_EOK;
}
#define SCAN_BSSI_ARR_MAX 30

#define CMP_MAC( a, b )  (((((unsigned char*)a)[0])==(((unsigned char*)b)[0]))&& \
                          ((((unsigned char*)a)[1])==(((unsigned char*)b)[1]))&& \
                          ((((unsigned char*)a)[2])==(((unsigned char*)b)[2]))&& \
                          ((((unsigned char*)a)[3])==(((unsigned char*)b)[3]))&& \
                          ((((unsigned char*)a)[4])==(((unsigned char*)b)[4]))&& \
                          ((((unsigned char*)a)[5])==(((unsigned char*)b)[5])))
static whd_mac_t mac_addr_arr[SCAN_BSSI_ARR_MAX];
rt_uint8_t current_bssid_arr_length = 0;

bool scan_bssi_has(whd_mac_t *BSSID)
{
    whd_mac_t *mac_iter = NULL;
    /* Check for duplicate SSID */
    for (mac_iter = mac_addr_arr; (mac_iter < mac_addr_arr + current_bssid_arr_length); ++mac_iter)
    {
        if (CMP_MAC(mac_iter->octet, BSSID->octet))
        {
            /* The scanned result is a duplicate; just return */
            return true;
        }
    }
    /* If scanned Wi-Fi is not a duplicate then populate the array */
    if (current_bssid_arr_length < SCAN_BSSI_ARR_MAX)
    {
        memcpy(&mac_iter->octet, &BSSID->octet, sizeof(whd_mac_t));
        current_bssid_arr_length++;
    }
    return false;
}

void scan_callback(whd_scan_result_t **result_ptr, void *user_data, whd_scan_status_t status)
{
    whd_scan_result_t *whd_scan_result = *result_ptr;
    uint32_t scan_status = status;

    /* Check if we don't have a scan result to send to the user */
    if ((result_ptr == NULL) || (*result_ptr == NULL))
    {
        /* Check for scan complete */
        if (status == WHD_SCAN_COMPLETED_SUCCESSFULLY || status == WHD_SCAN_ABORTED)
        {
            /* Notify scan complete */
            rt_wlan_dev_indicate_event_handle(wifi_sta.wlan, RT_WLAN_DEV_EVT_SCAN_DONE, 0);
        }
        return;
    }

    if (whd_scan_result->SSID.length != 0)
    {
        /* parse scan report event data */
        struct rt_wlan_buff buff;
        struct rt_wlan_info wlan_info;
        if (scan_bssi_has(&whd_scan_result->BSSID) == false)
        {
            _ifx_scan_info2rtt(whd_scan_result, &wlan_info);

            buff.data = &wlan_info;
            buff.len = sizeof(struct rt_wlan_info);

            /* indicate scan report event */
            rt_wlan_dev_indicate_event_handle(wifi_sta.wlan, RT_WLAN_DEV_EVT_SCAN_REPORT, &buff);
        }
    }
    return;
}

void cy_network_process_ethernet_data(whd_interface_t iface, whd_buffer_t buf)
{
    LOG_D("get wlan data");
    uint8_t *data = whd_buffer_get_current_piece_data_pointer(iface->whd_driver, buf);
    uint16_t ethertype;
    struct netif *net_interface = NULL;

    if (iface->role == WHD_STA_ROLE)
    {

    }
    else if (iface->role == WHD_AP_ROLE)
    {

    }
    else
    {
        cy_buffer_release(buf, WHD_NETWORK_RX);
        return;
    }

    ethertype = (uint16_t) (data[12] << 8 | data[13]);
    if (ethertype == EAPOL_PACKET_TYPE)
    {
        LOG_D("EAPOL_PACKET_TYPE");
    }
    else
    {
        /* If the interface is not yet set up, drop the packet */
        LOG_D("Send data up to wlan");
        rt_wlan_dev_report_data(wifi_sta.wlan, data, whd_buffer_get_current_piece_size(iface->whd_driver, buf));
        cy_buffer_release(buf, WHD_NETWORK_RX);
    }
}

static whd_netif_funcs_t _netif_if =
{
    .whd_network_process_ethernet_data = cy_network_process_ethernet_data,
};
static rt_err_t wlan_init(struct rt_wlan_device *wlan)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    if (inited == RT_FALSE)
    {
        inited = RT_TRUE;
        /* Initialize the Wi-Fi device, Wi-Fi transport*/
        cybsp_wifi_init_primary_extended(&(_GET_DEV(wlan)->whd_if), NULL, NULL, NULL, &_netif_if);
        RT_ASSERT(result == 0 && "cy_wcm_init failed...!\n");
    }
    if (wlan == wifi_ap.wlan)
    {
        LOG_D("softap init");
        cybsp_wifi_init_secondary(&(_GET_DEV(wlan)->whd_if), NULL);
    }
    return RT_EOK;
}
static rt_err_t wlan_scan(struct rt_wlan_device *wlan, struct rt_scan_info *scan_info)
{
    current_bssid_arr_length = 0;
    memset(mac_addr_arr, sizeof(whd_mac_t) * SCAN_BSSI_ARR_MAX, 0);
    whd_wifi_scan(_GET_DEV(wlan)->whd_if, WHD_SCAN_TYPE_ACTIVE, WHD_BSS_TYPE_ANY,
    NULL, NULL, NULL, NULL, scan_callback, &scan_result, NULL);
    return RT_EOK;
}
static rt_err_t wlan_scan_stop(struct rt_wlan_device *wlan)
{
    whd_wifi_stop_scan(_GET_DEV(wlan)->whd_if);
    return RT_EOK;
}

static rt_err_t wlan_join(struct rt_wlan_device *wlan, struct rt_sta_info *sta_info)
{
    uint32_t res;
    whd_ssid_t ssid;

    strncpy(ssid.value, sta_info->ssid.val, sta_info->ssid.len);
    ssid.length = sta_info->ssid.len;
    ssid.value[sta_info->ssid.len] = '\0';

    /** Join to Wi-Fi AP **/
    res = whd_wifi_join(_GET_DEV(wlan)->whd_if, &ssid, WHD_SECURITY_WPA2_AES_PSK, sta_info->key.val, sta_info->key.len);

    if (res == WHD_SUCCESS)
    {
        rt_wlan_dev_indicate_event_handle(wifi_sta.wlan, RT_WLAN_DEV_EVT_CONNECT, 0);
    }
    else
    {
        rt_wlan_dev_indicate_event_handle(wifi_sta.wlan, RT_WLAN_DEV_EVT_CONNECT_FAIL, 0);
    }

    return RT_EOK;
}

int _wlan_send(struct rt_wlan_device *wlan, void *buff, int len)
{
    whd_buffer_t buffer;
    if (whd_wifi_is_ready_to_transceive(_GET_DEV(wlan)->whd_if) != WHD_SUCCESS)
    {
        return -RT_ERROR;
    }
    if (whd_host_buffer_get(_GET_DEV(wlan)->whd_if->whd_driver, &buffer, WHD_NETWORK_TX, len + WHD_LINK_HEADER, 1000) != 0)
    {
        LOG_D("err whd_host_buffer_get failed\n");
        return -RT_ERROR;
    }

    whd_buffer_add_remove_at_front(_GET_DEV(wlan)->whd_if->whd_driver, &buffer, WHD_LINK_HEADER);
    memcpy(whd_buffer_get_current_piece_data_pointer(_GET_DEV(wlan)->whd_if->whd_driver, buffer), buff, len);

    whd_network_send_ethernet_data(_GET_DEV(wlan)->whd_if, buffer);

    return RT_EOK;
}

rt_err_t wlan_mode(struct rt_wlan_device *wlan, rt_wlan_mode_t mode)
{
    switch (mode)
    {
    case RT_WLAN_STATION:
        LOG_D("wlan_mode RT_WLAN_STATION\n");
        break;
    case RT_WLAN_AP:
        LOG_D("wlan_mode RT_WLAN_AP\n");
        break;
    }

    return RT_EOK;
}

rt_err_t wlan_softap(struct rt_wlan_device *wlan, struct rt_ap_info *ap_info)
{
    uint32_t res;
    whd_interface_t *ifx = &(_GET_DEV(wlan)->whd_if);
    LOG_D("wlan_softap");

    res = whd_wifi_init_ap(*ifx, (whd_ssid_t*)&ap_info->ssid, get_security(ap_info->security), (const uint8_t *) ap_info->key.val,
            ap_info->key.len, ap_info->channel);
    res = whd_wifi_start_ap(*ifx);
    if (res == WHD_SUCCESS)
    {
        LOG_D("ap start ok");
        rt_wlan_dev_indicate_event_handle(wifi_ap.wlan, RT_WLAN_DEV_EVT_AP_START, 0);
    }
    else
    {
        rt_wlan_dev_indicate_event_handle(wifi_ap.wlan, RT_WLAN_DEV_EVT_AP_STOP, 0);
    }

    return RT_EOK;

}
rt_err_t wlan_disconnect(struct rt_wlan_device *wlan)
{
    LOG_D("wlan_disconnect");
    uint32_t ret = whd_wifi_leave(_GET_DEV(wlan)->whd_if);

    if (ret == WHD_SUCCESS)
    {
        rt_wlan_dev_indicate_event_handle(wlan, RT_WLAN_DEV_EVT_DISCONNECT, RT_NULL);
    }

    return RT_EOK;
}
rt_err_t wlan_ap_stop(struct rt_wlan_device *wlan)
{
    LOG_D("wlan_ap_stop");
    whd_wifi_stop_ap(_GET_DEV(wlan)->whd_if);
    return RT_EOK;
}
rt_err_t wlan_ap_deauth(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    whd_mac_t _mac;
    LOG_D("wlan_ap_deauth");
    memcpy(&_mac, mac, sizeof(whd_mac_t));
    whd_wifi_deauth_sta(_GET_DEV(wlan)->whd_if, &_mac, 0);
    return RT_EOK;
}
int wlan_get_rssi(struct rt_wlan_device *wlan)
{
    int32_t rssi;
    whd_wifi_get_rssi(_GET_DEV(wlan)->whd_if, &rssi);
    return rssi;
}
rt_err_t wlan_set_channel(struct rt_wlan_device *wlan, int channel)
{
    LOG_D("wlan_set_channel");
    whd_wifi_set_channel(_GET_DEV(wlan)->whd_if, channel);
    return RT_EOK;
}
int wlan_get_channel(struct rt_wlan_device *wlan)
{
    uint32_t channel;
    LOG_D("wlan_get_channel");
    whd_wifi_get_channel(_GET_DEV(wlan)->whd_if, &channel);
    return channel;
}
rt_err_t wlan_set_mac(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    whd_mac_t _mac;
    memcpy(&_mac, mac, sizeof(whd_mac_t));
    whd_wifi_set_mac_address(_GET_DEV(wlan)->whd_if, _mac);
    return RT_EOK;
}
rt_err_t wlan_get_mac(struct rt_wlan_device *wlan, rt_uint8_t mac[])
{
    whd_mac_t *_mac = (whd_mac_t *) mac;
    if (whd_wifi_get_mac_address(_GET_DEV(wlan)->whd_if, _mac) == WHD_SUCCESS)
    {
        LOG_D("WLAN MAC Address : %02X:%02X:%02X:%02X:%02X:%02X", _mac->octet[0], _mac->octet[1], _mac->octet[2],
                _mac->octet[3], _mac->octet[4], _mac->octet[5]);
    }
    return RT_EOK;
}
const static struct rt_wlan_dev_ops ops =
{
    .wlan_init          = wlan_init,
    .wlan_mode          = wlan_mode,
    .wlan_scan          = wlan_scan,
    .wlan_join          = wlan_join,
    .wlan_softap        = wlan_softap,
    .wlan_disconnect    = wlan_disconnect,
    .wlan_ap_stop       = wlan_ap_stop,
    .wlan_ap_deauth     = wlan_ap_deauth,
    .wlan_scan_stop     = wlan_scan_stop,
    .wlan_get_rssi      = wlan_get_rssi,
    .wlan_set_channel   = wlan_set_channel,
    .wlan_get_channel   = wlan_get_channel,
    .wlan_set_mac       = wlan_set_mac,
    .wlan_get_mac       = wlan_get_mac,
    .wlan_send          = _wlan_send,
};

int wifi_ifx_init(void)
{
    static struct rt_wlan_device wlan_sta, wlan_ap;
    rt_err_t ret;
    wifi_sta.wlan = &wlan_sta;
    wifi_ap.wlan = &wlan_ap;

    /* register wlan device for ap */
    ret = rt_wlan_dev_register(&wlan_ap, RT_WLAN_DEVICE_AP_NAME, &ops, 0, &wifi_ap);
    if (ret != RT_EOK)
    {
        return ret;
    }

    /* register wlan device for sta */
    ret = rt_wlan_dev_register(&wlan_sta, RT_WLAN_DEVICE_STA_NAME, &ops, 0, &wifi_sta);
    if (ret != RT_EOK)
    {
        return ret;
    }

}
INIT_DEVICE_EXPORT(wifi_ifx_init);
