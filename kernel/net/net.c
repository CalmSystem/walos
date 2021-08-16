#include "net.h"
#include "init.h"
#include "dhcp.h"
#include "timeouts.h"
#include "dns.h"
#include "netif/ethernetif_priv.h"

static struct netif* current_netif = NULL;
static ip_addr_t current_ip = {IPADDR_ANY}, current_dns = {IPADDR_ANY};
void if_change(struct netif* some_netif) {
    (void)some_netif;
    if (netif_default) {
        if (current_netif != netif_default) {
            syslogf((syslog_ctx){"NET", "If\0", DEBUG}, "Default is %d", netif_default->num);
            current_netif = netif_default;
        }
        if (!ip_addr_cmp(&current_ip, &netif_default->ip_addr)) {
            // "If\0xxx\0"
            char sub[8] = {0};
            sub[0] = 'I';
            sub[1] = 'f';
            snprintf(sub + 3, 3, "%d", netif_default->num);
            syslogf((syslog_ctx){"NET", sub, INFO}, "Ip is %s/%d",
                    ipaddr_ntoa(&netif_default->ip_addr),
                    __builtin_popcountl(netif_default->netmask.addr));
            syslogf((syslog_ctx){"NET", sub, DEBUG}, "Gateway Ip is %s",
                    ipaddr_ntoa(&netif_default->gw));
            ip_addr_copy(current_ip, netif_default->ip_addr);
        }
    } else if (current_netif) {
        syslogf((syslog_ctx){"NET", "If\0", WARN}, "Lost default %d", current_netif->num);
        current_netif = NULL;
    }
    if (!ip_addr_cmp(&current_dns, dns_getserver(0))) {
        syslogf((syslog_ctx){"NET", "DNS\0", INFO}, "Uses %s",
                ipaddr_ntoa(dns_getserver(0)));
        ip_addr_copy(current_ip, *dns_getserver(0));
    }
}

void net_pull() {
#if !LWIP_NETIF_STATUS_CALLBACK
    if_change(NULL);
#endif
    if (netif_default)
        ethernetif_input(netif_default);

    sys_check_timeouts();
}

void net_init(bool auto_dhcp) {
    ethernetif_status_callback = if_change;
    if (auto_dhcp) ethernetif_up_callback = dhcp_start;
    lwip_init();
    syslogs((syslog_ctx){"NET", NULL, INFO}, "Initialized");
}
