/*
 * efi-pbcp.h
 *
 * Pxe Base Code Protocol and Pxe Base Code Callback Protocol.
 */

#ifndef __EFI_PBCP_H
#define __EFI_PBCP_H

#include "../efi.h"


// first definitions for PXE_BASE_CODE_PROTOCOL constants
#define EFI_PXE_BASE_CODE_PROTOCOL_GUID {0x03C4E603, 0xAC28, 0x11d3, {0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}}

#define EFI_PXE_BASE_CODE_MAX_IPCNT         8
#define EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES   8
#define EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES 8

#define EFI_PXE_BASE_CODE_BOOT_LAYER_INITIAL 0


// now definitions for PXE_BASE_CODE_CALLBACK_PROTOCOL constants
#define EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_GUID {0x245DCA21, 0xFB7B, 0x11d3, {0x8F, 0x01, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

#define EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_REVISION 0x00010000


// types for the PXE_BASE_CODE_PROTOCOL
struct EFI_PXE_BASE_CODE_PROTOCOL;

typedef struct {
    uint8_t Addr[32];
} EFI_MAC_ADDRESS;

typedef struct {
    uint8_t Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct {
    uint8_t Addr[16];
} EFI_IPv6_ADDRESS;

typedef union {
    uint32_t Addr[4];
    EFI_IPv4_ADDRESS v4;
    EFI_IPv6_ADDRESS v6;
} EFI_IP_ADDRESS;

typedef uint16_t EFI_PXE_BASE_CODE_UDP_PORT;

typedef struct {
    uint16_t          Type;
    bool8_t         AcceptAnyResponse;
    uint8_t           _reserved;
    EFI_IP_ADDRESS  IpAddr;
} EFI_PXE_BASE_CODE_SRVLIST;

typedef struct {
    bool8_t        UseMCast;
    bool8_t        UseBCast;
    bool8_t        UseUCast;
    bool8_t        MustUseList;
    EFI_IP_ADDRESS ServerMCastIp;
    uint16_t         IpCnt;
    EFI_PXE_BASE_CODE_SRVLIST SrvList[EFI_PXE_BASE_CODE_MAX_IPCNT];
} EFI_PXE_BASE_CODE_DISCOVER_INFO;

typedef struct {
    EFI_IP_ADDRESS             MCastIp;
    EFI_PXE_BASE_CODE_UDP_PORT CPort;
    EFI_PXE_BASE_CODE_UDP_PORT SPort;
    uint16_t                     ListenTimeout;
    uint16_t                     TransmitTimeout;
} EFI_PXE_BASE_CODE_MTFTP_INFO;

typedef struct {
    uint8_t Filters;
    uint8_t IpCnt;
    uint16_t _reserved;
    EFI_IP_ADDRESS IpList[EFI_PXE_BASE_CODE_MAX_IPCNT];
} EFI_PXE_BASE_CODE_IP_FILTER;

typedef struct {
    EFI_IP_ADDRESS  IpAddr;
    EFI_MAC_ADDRESS MacAddr;
} EFI_PXE_BASE_CODE_ARP_ENTRY;

typedef struct {
    EFI_IP_ADDRESS  IpAddr;
    EFI_IP_ADDRESS  SubnetMask;
    EFI_IP_ADDRESS  GwAddr;
} EFI_PXE_BASE_CODE_ROUTE_ENTRY;

typedef struct {
    uint8_t  BootpOpcode;
    uint8_t  BootpHwType;
    uint8_t  BootpHwAddrLen;
    uint8_t  BootpGateHops;
    uint32_t BootpIdent;
    uint16_t BootpSeconds;
    uint16_t BootpFlags;
    uint8_t  BootpCiAddr[4];
    uint8_t  BootpYiAddr[4];
    uint8_t  BootpSiAddr[4];
    uint8_t  BootpGiAddr[4];
    uint8_t  BootpHwAddr[16];
    uint8_t  BootpSrvName[64];
    uint8_t  BootpBootFile[128];
    uint32_t DhcpMagik;
    uint8_t  DhcpOptions[56];
} EFI_PXE_BASE_CODE_DHCPV4_PACKET;

typedef struct {
    uint32_t MessageType: 8;
    uint32_t TransactionId: 24;
    uint8_t  DhcpOptions[1024];
} EFI_PXE_BASE_CODE_DHCPV6_PACKET;

typedef union {
    uint8_t                           Raw[1472];
    EFI_PXE_BASE_CODE_DHCPV6_PACKET Dhcpv6;
    EFI_PXE_BASE_CODE_DHCPV4_PACKET Dhcpv4;
} EFI_PXE_BASE_CODE_PACKET;

typedef struct {
    uint8_t Type;
    uint8_t Code;
    uint16_t Checksum;
    union {
        uint32_t _reserved;
        uint32_t Mtu;
        uint32_t Pointer;
        struct {
            uint16_t Identifier;
            uint16_t Sequence;
        } Echo;
    } u;
    uint8_t Data[494];
} EFI_PXE_BASE_CODE_ICMP_ERROR;

typedef struct {
    uint8_t ErrorCode;
    char8_t ErrorString[127];
} EFI_PXE_BASE_CODE_TFTP_ERROR;

typedef struct {
    bool8_t                       Started;
    bool8_t                       IPv6Available;
    bool8_t                       IPv6Supported;
    bool8_t                       UsingIPv6;
    bool8_t                       BisSupported;
    bool8_t                       BisDetected;
    bool8_t                       AutoArp;
    bool8_t                       SendGUID;
    bool8_t                       DhcpDiscoverValid;
    bool8_t                       DhcpAckReceived;
    bool8_t                       ProxyOfferReceived;
    bool8_t                       PxeDiscoverValid;
    bool8_t                       PxeReplyReceived;
    bool8_t                       PxeBisReplyReceived;
    bool8_t                       IcmpErrorReceived;
    bool8_t                       TftpErrorReceived;
    bool8_t                       MakeCallbacks;
    uint8_t                         TTL;
    uint8_t                         ToS;
    EFI_IP_ADDRESS                StationIp;
    EFI_IP_ADDRESS                SubnetMask;
    EFI_PXE_BASE_CODE_PACKET      DhcpDiscover;
    EFI_PXE_BASE_CODE_PACKET      DhcpAck;
    EFI_PXE_BASE_CODE_PACKET      ProxyOffer;
    EFI_PXE_BASE_CODE_PACKET      PxeDiscover;
    EFI_PXE_BASE_CODE_PACKET      PxeReply;
    EFI_PXE_BASE_CODE_PACKET      PxeBisReply;
    EFI_PXE_BASE_CODE_IP_FILTER   IpFilter;
    uint32_t                        ArpCacheEntries;
    EFI_PXE_BASE_CODE_ARP_ENTRY   ArpCache[EFI_PXE_BASE_CODE_MAX_ARP_ENTRIES];
    uint32_t                        RouteTableEntries;
    EFI_PXE_BASE_CODE_ROUTE_ENTRY RouteTable[EFI_PXE_BASE_CODE_MAX_ROUTE_ENTRIES];
    EFI_PXE_BASE_CODE_ICMP_ERROR  IcmpError;
    EFI_PXE_BASE_CODE_TFTP_ERROR  TftpError;
} EFI_PXE_BASE_CODE_MODE;

typedef enum {
    EFI_PXE_BASE_CODE_TFTP_FIRST,
    EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
    EFI_PXE_BASE_CODE_TFTP_READ_FILE,
    EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,
    EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY,
    EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE,
    EFI_PXE_BASE_CODE_MTFTP_READ_FILE,
    EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY,
    EFI_PXE_BASE_CODE_MTFTP_LAST
} EFI_PXE_BASE_CODE_TFTP_OPCODE;

typedef EFI_STATUS (*EFI_PXE_BASE_CODE_START) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, bool8_t UseIpv6);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_STOP) (struct EFI_PXE_BASE_CODE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_DHCP) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, bool8_t SortOffers);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_DISCOVER) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, uint16_t Type, uint16_t* Layer, bool8_t UseBis, EFI_PXE_BASE_CODE_DISCOVER_INFO* Info);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_MTFTP) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, EFI_PXE_BASE_CODE_TFTP_OPCODE Operation, void* Buffer, bool8_t Overwrite, uint64_t* BufferSize, uintn_t* BlockSize, EFI_IP_ADDRESS* ServerIp, char8_t* Filename, EFI_PXE_BASE_CODE_MTFTP_INFO* Info, bool8_t DontUseBuffer);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_UDP_WRITE) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, uint16_t OpFlags, EFI_IP_ADDRESS* DestIp, EFI_PXE_BASE_CODE_UDP_PORT* DestPort, EFI_IP_ADDRESS* GatewayIp, EFI_IP_ADDRESS* SrcIp, EFI_PXE_BASE_CODE_UDP_PORT* SrcPort, uintn_t* HeaderSize, void* HeaderPtr, uintn_t* BufferSize, void* BufferPtr);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_UDP_READ) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, uint16_t OpFlags, EFI_IP_ADDRESS* DestIp, EFI_PXE_BASE_CODE_UDP_PORT* DestPort, EFI_IP_ADDRESS* SrcIp, EFI_PXE_BASE_CODE_UDP_PORT* SrcPort, uintn_t* HeaderSize, void* HeaderPtr, uintn_t* BufferSize, void* BufferPtr);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_SET_IP_FILTER) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, EFI_PXE_BASE_CODE_IP_FILTER* NewFilter);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_ARP) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, EFI_IP_ADDRESS* IpAddr, EFI_MAC_ADDRESS* MacAddr);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_SET_PARAMETERS) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, bool8_t* NewAutoArp, bool8_t* NewSendGUID, uint8_t* NewTTL, uint8_t* NewToS, bool8_t* NewMakeCallback);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_SET_STATION_IP) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, EFI_IP_ADDRESS* NewStationIp, EFI_IP_ADDRESS* NewSubnetMask);
typedef EFI_STATUS (*EFI_PXE_BASE_CODE_SET_PACKETS) (struct EFI_PXE_BASE_CODE_PROTOCOL *This, bool8_t* NewDhcpDiscoverValid, bool8_t* NewDhcpAckReceived, bool8_t* NewProxyOfferReceived, bool8_t* NewPxeOfferValid, bool8_t* NewPxeReplyReceived, bool8_t* NewPxeBisReplyReceived, EFI_PXE_BASE_CODE_PACKET* NewDhcpDiscover, EFI_PXE_BASE_CODE_PACKET* NewDhcpAck, EFI_PXE_BASE_CODE_PACKET* NewProxyOffer, EFI_PXE_BASE_CODE_PACKET* NewPxeOffer, EFI_PXE_BASE_CODE_PACKET* NewPxeReply, EFI_PXE_BASE_CODE_PACKET* NewPxeBisReply);

typedef struct EFI_PXE_BASE_CODE_PROTOCOL {
    uint64_t Revision;
    EFI_PXE_BASE_CODE_START Start;
    EFI_PXE_BASE_CODE_STOP Stop;
    EFI_PXE_BASE_CODE_DHCP Dhcp;
    EFI_PXE_BASE_CODE_DISCOVER Discover;
    EFI_PXE_BASE_CODE_MTFTP Mtftp;
    EFI_PXE_BASE_CODE_UDP_WRITE UdpWrite;
    EFI_PXE_BASE_CODE_UDP_READ UdpRead;
    EFI_PXE_BASE_CODE_SET_IP_FILTER SetIpFilter;
    EFI_PXE_BASE_CODE_ARP Arp;
    EFI_PXE_BASE_CODE_SET_PARAMETERS SetParameters;
    EFI_PXE_BASE_CODE_SET_STATION_IP SetStationIp;
    EFI_PXE_BASE_CODE_SET_PACKETS SetPackets;
    EFI_PXE_BASE_CODE_MODE *Mode;
} EFI_PXE_BASE_CODE_PROTOCOL;

typedef enum {
    EFI_PXE_BASE_CODE_CALLBACK_STATUS_FIRST,
    EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
    EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT,
    EFI_PXE_BASE_CODE_CALLBACK_STATUS_LAST
} EFI_PXE_BASE_CODE_CALLBACK_STATUS;

typedef enum {
    EFI_PXE_BASE_CODE_FUNCTION_FIRST,
    EFI_PXE_BASE_CODE_FUNCTION_DHCP,
    EFI_PXE_BASE_CODE_FUNCTION_DISCOVER,
    EFI_PXE_BASE_CODE_FUNCTION_MTFTP,
    EFI_PXE_BASE_CODE_FUNCTION_UDP_WRITE,
    EFI_PXE_BASE_CODE_FUNCTION_UDP_READ,
    EFI_PXE_BASE_CODE_FUNCTION_ARP,
    EFI_PXE_BASE_CODE_FUNCTION_IGMP,
    EFI_PXE_BASE_CODE_PXE_FUNCTION_LAST
} EFI_PXE_BASE_CODE_FUNCTION;


// types for the PXE_BASE_CODE_CALLBACK_PROTOCOL

struct EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL;

typedef EFI_PXE_BASE_CODE_CALLBACK_STATUS (*EFI_PXE_CALLBACK) (struct EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL* This, EFI_PXE_BASE_CODE_FUNCTION Function, bool8_t Received, uint32_t PacketLen, EFI_PXE_BASE_CODE_PACKET* Packet);

typedef struct EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL {
    uint64_t Revision;
    EFI_PXE_CALLBACK Callback;
} EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL;

#endif /* __EFI_PBCP_H */
