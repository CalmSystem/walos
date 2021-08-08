#include "net.h"
#include "init.h"
#include "dhcp.h"
#include "timeouts.h"
#include "netif/ethernetif_priv.h"

void net_init(bool auto_dhcp) {
    if (auto_dhcp) ethernetif_up_callback = dhcp_start;
    lwip_init();
    syslogs((syslog_ctx){"NET", NULL, INFO}, "Initialized");
}

void net_pull() {
    static ip_addr_t current_ip = {IPADDR_ANY};
    if (netif_default) {
        ethernetif_input(netif_default);
        if (!ip_addr_cmp(&current_ip, &netif_default->ip_addr)) {
            // "If\0xxx\0"
            char sub[8] = {0};
            sub[0] = 'I';
            sub[1] = 'f';
            snprintf(sub+3, 3, "%d", netif_default->num);
            syslogf((syslog_ctx){"NET", sub, INFO}, "Got Ip %s",
                ipaddr_ntoa(&netif_default->ip_addr));
            ip_addr_copy(current_ip, netif_default->ip_addr);
        }
    } else if(!ip_addr_isany(&current_ip)) {
        syslogs((syslog_ctx){"NET", "If\0", WARN}, "Lost default");
        ip_addr_set_any(false, &current_ip);
    }
    sys_check_timeouts();
}
