#include <w/main>
#include <w/net.h>

W_FN_(hw, netif_cnt, w_size, ())

#define W_NETIF_STATE_NO_MEDIA (1<<2)
#define W_NETIF_STATE_STOPPED 0
#define W_NETIF_STATE_STARTED 1
#define W_NETIF_STATE_INITIALIZED 2
W_FN(hw, netif_info, w_res, (w_size idx, uint32_t* state, w_mac_addr* local, w_size local_sz, w_mac_addr* broadcast, w_size broadcast_sz, uint32_t* packet_sz), {ST_LEN, ST_PTR, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_PTR})

#define ARP_ETHER_PROTO_TYPE 0x0806
#define IPV4_ETHER_PROTO_TYPE 0x0800
#define IPV6_ETHER_PROTO_TYPE 0x86DD
W_FN(hw, netif_transmit, w_res, (w_size idx, uint16_t protocol, const w_mac_addr* src, w_size src_sz, const w_mac_addr* dst, w_size dst_sz, const w_ciovec* iov, w_size iocnt), {ST_LEN, ST_VAL, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_CIO, ST_LEN})
W_FN(hw, netif_receive, w_res, (w_size idx, uint16_t* protocol, w_mac_addr* src, w_size src_sz, w_mac_addr* dst, w_size dst_sz, w_iovec* iov, w_size iocnt, w_size* read), {ST_LEN, ST_PTR, ST_ARR, ST_LEN, ST_ARR, ST_LEN, ST_BIO, ST_LEN, ST_PTR})

static inline void log_hex(uint64_t val, uint8_t len) {
	char buf[len];
	for (int i = len-1; i >= 0; i--) {
		buf[i] = '0' + (val & 0xF);
		if (buf[i] > '9') buf[i] += 'a'-'9'-1;
		val >>= 4;
	}
	const w_ciovec iov = {buf, len};
	sys_log(WL_NOTICE, &iov, 1);
}
static inline void log_mac(w_mac_addr mac) {
	char buf[sizeof(mac)*2];
	for (int j = 0; j < sizeof(mac); j++) {
		int i = (sizeof(mac) - j - 1) * 2;
		buf[i] = '0' + (mac.addr[j] & 0xF);
		if (buf[i] > '9') buf[i] += 'a'-'9'-1;
		i--;
		buf[i] = '0' + (mac.addr[j] >> 4);
		if (buf[i] > '9') buf[i] += 'a'-'9'-1;
	}
	const w_ciovec iov = {buf, sizeof(buf)};
	sys_log(WL_NOTICE, &iov, 1);
}
static uint8_t s_buf[2048];
static w_iovec s_bufio[] = W_IOBUF(s_buf);

static inline void memcpy(void* dst, const void* src, w_size len) {
	for (w_size i = 0; i < len; i++) *((uint8_t*)dst + i) = *((const uint8_t*)src + i);
}

struct {
	w_size i;
	w_mac_addr local_mac, broad_mac;
	w_ip4_addr local_ip;
} s_if = {0};
static w_res net_arp(w_ip4_addr ip, w_mac_addr* out) {
	static uint8_t s_req[8 + 6*2 + 4*2] = {
		0, 1, 0x08, 0x00,
		6, 4, 0, 1,
		0, 0, 0, 0, 0, 0,
		10, 0, 2, 15,
		0, 0, 0, 0, 0, 0,
		255, 255, 255, 255
	};
	static const w_ciovec s_reqio[] = W_IOBUF(s_req);
	memcpy(s_req + 14, &s_if.local_ip, sizeof(s_if.local_ip));
	memcpy(s_req + 18, &s_if.local_mac, sizeof(s_if.local_mac));
	memcpy(s_req + 24, &ip, sizeof(ip));
	w_res err = W_EFAIL;
	for (w_size i = 0; i < 5; i++) {
		err = W_ENOTREADY;
		for (w_size j = 0; err == W_ENOTREADY && j < 5; j++)
			err = hw_netif_transmit(s_if.i, ARP_ETHER_PROTO_TYPE, 0, 0, &s_if.broad_mac, sizeof(s_if.broad_mac), s_reqio, 1);

		log_hex(0, 4);
		log_hex(err, 2);
		if (err) continue;

		err = W_ENOTREADY;
		uint16_t proto;
		w_size read;
		for (w_size j = 0; err == W_ENOTREADY && j < 99; j++)
			err = hw_netif_receive(s_if.i, &proto, 0, 0, 0, 0, s_bufio, 1, &read);

		log_hex(err, 2);
		if (err || proto != ARP_ETHER_PROTO_TYPE) continue;

		memcpy(out, s_buf + 8, sizeof(*out));
		return W_SUCCESS;
	}
	return err;
}

W_FN_HDL_(net, test, w_res, (w_res (*cb)(w_res))) {
	const w_size cnt = hw_netif_cnt();
	log_hex(cnt, sizeof(cnt)*2);
	for (w_size i = 0; i < cnt; i++) {
		s_if.i = i;
		s_if.local_ip = (w_ip4_addr){{10, 0, 2, 15}};
		uint32_t state, packet_sz;
		hw_netif_info(i, &state, &s_if.local_mac, sizeof(s_if.local_mac), &s_if.broad_mac, sizeof(s_if.broad_mac), &packet_sz);
		log_hex(state, sizeof(state)*2);
		log_hex(packet_sz, sizeof(packet_sz)*2);
		log_mac(s_if.local_mac);
		log_mac(s_if.broad_mac);

		w_mac_addr gateway_mac;
		w_res err = net_arp((w_ip4_addr){{10, 0, 2, 2}}, &gateway_mac);
		log_hex(err, 2);
		if (!err) log_mac(gateway_mac);

		log_hex(cb(err), 2);
	}
	return W_SUCCESS;
}
