/*
 * efi-sfp.h
 *
 * UEFI Simple Network Protocol.
 *
 * Not part of the original project
 */

#ifndef __EFI_SNP_H
#define __EFI_SNP_H

#include "../efi.h"
#include "ip.h"

#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID {0xA19832B9, 0xAC25, 0x11D3, {0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}}

typedef struct _EFI_SIMPLE_NETWORK_PROTOCOL EFI_SIMPLE_NETWORK_PROTOCOL;

typedef EFI_SIMPLE_NETWORK_PROTOCOL EFI_SIMPLE_NETWORK;

typedef struct
{
	uint64_t RxTotalFrames;
	uint64_t RxGoodFrames;
	uint64_t RxUndersizeFrames;
	uint64_t RxOversizeFrames;
	uint64_t RxDroppedFrames;
	uint64_t RxUnicastFrames;
	uint64_t RxBroadcastFrames;
	uint64_t RxMulticastFrames;
	uint64_t RxCrcErrorFrames;

	uint64_t RxTotalBytes;

	uint64_t TxTotalFrames;
	uint64_t TxGoodFrames;
	uint64_t TxUndersizeFrames;
	uint64_t TxOversizeFrames;
	uint64_t TxDroppedFrames;
	uint64_t TxUnicastFrames;
	uint64_t TxBroadcastFrames;
	uint64_t TxMulticastFrames;
	uint64_t TxCrcErrorFrames;
	uint64_t TxTotalBytes;

	uint64_t Collisions;
	uint64_t UnsupportedProtocol;

	uint64_t RxDuplicatedFrames;
	uint64_t RxDecryptErrorFrames;

	uint64_t TxErrorFrames;
	uint64_t TxRetryFrames;
} EFI_NETWORK_STATISTICS;

typedef enum
{
	EfiSimpleNetworkStopped,
	EfiSimpleNetworkStarted,
	EfiSimpleNetworkInitialized,
	EfiSimpleNetworkMaxState
} EFI_SIMPLE_NETWORK_STATE;

#define EFI_SIMPLE_NETWORK_RECEIVE_UNICAST 0x01
#define EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST 0x02
#define EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST 0x04
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS 0x08
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST 0x10

#define EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT 0x01
#define EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT 0x02
#define EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT 0x04
#define EFI_SIMPLE_NETWORK_SOFTWARE_INTERRUPT 0x08

#define MAX_MCAST_FILTER_CNT 16
typedef struct
{
	uint32_t State;
	uint32_t HwAddressSize;
	uint32_t MediaHeaderSize;
	uint32_t MaxPacketSize;
	uint32_t NvRamSize;
	uint32_t NvRamAccessSize;
	uint32_t ReceiveFilterMask;
	uint32_t ReceiveFilterSetting;
	uint32_t MaxMCastFilterCount;
	uint32_t MCastFilterCount;
	EFI_MAC_ADDRESS MCastFilter[MAX_MCAST_FILTER_CNT];
	EFI_MAC_ADDRESS CurrentAddress;
	EFI_MAC_ADDRESS BroadcastAddress;
	EFI_MAC_ADDRESS PermanentAddress;
	uint8_t IfType;
	bool8_t MacAddressChangeable;
	bool8_t MultipleTxSupported;
	bool8_t MediaPresentSupported;
	bool8_t MediaPresent;
} EFI_SIMPLE_NETWORK_MODE;


typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_START)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_STOP)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_INITIALIZE)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	uintn_t ExtraRxBufferSize,
	uintn_t ExtraTxBufferSize);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RESET)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	bool8_t ExtendedVerification);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_SHUTDOWN)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RECEIVE_FILTERS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	uint32_t Enable,
	uint32_t Disable,
	bool8_t ResetMCastFilter,
	uintn_t MCastFilterCnt,
	EFI_MAC_ADDRESS *MCastFilter);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_STATION_ADDRESS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	bool8_t Reset,
	EFI_MAC_ADDRESS *New);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_STATISTICS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	bool8_t Reset,
	uintn_t *StatisticsSize,
	EFI_NETWORK_STATISTICS *StatisticsTable);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	bool8_t IPv6,
	EFI_IP_ADDRESS *IP,
	EFI_MAC_ADDRESS *MAC);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_NVDATA)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	bool8_t ReadWrite,
	uintn_t Offset,
	uintn_t BufferSize,
	void *Buffer);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_GET_STATUS)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	uint32_t *InterruptStatus,
	void **TxBuf);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_TRANSMIT)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	uintn_t HeaderSize,
	uintn_t BufferSize,
	void *Buffer,
	EFI_MAC_ADDRESS *SrcAddr,
	EFI_MAC_ADDRESS *DestAddr,
	uint16_t *Protocol);

typedef EFI_STATUS (*EFI_SIMPLE_NETWORK_RECEIVE)(
	EFI_SIMPLE_NETWORK_PROTOCOL *This,
	uintn_t *HeaderSize,
	uintn_t *BufferSize,
	void *Buffer,
	EFI_MAC_ADDRESS *SrcAddr,
	EFI_MAC_ADDRESS *DestAddr,
	uint16_t *Protocol);

#define EFI_SIMPLE_NETWORK_PROTOCOL_REVISION 0x00010000
#define EFI_SIMPLE_NETWORK_INTERFACE_REVISION EFI_SIMPLE_NETWORK_PROTOCOL_REVISION

struct _EFI_SIMPLE_NETWORK_PROTOCOL
{
	uint64_t Revision;
	EFI_SIMPLE_NETWORK_START Start;
	EFI_SIMPLE_NETWORK_STOP Stop;
	EFI_SIMPLE_NETWORK_INITIALIZE Initialize;
	EFI_SIMPLE_NETWORK_RESET Reset;
	EFI_SIMPLE_NETWORK_SHUTDOWN Shutdown;
	EFI_SIMPLE_NETWORK_RECEIVE_FILTERS ReceiveFilters;
	EFI_SIMPLE_NETWORK_STATION_ADDRESS StationAddress;
	EFI_SIMPLE_NETWORK_STATISTICS Statistics;
	EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC MCastIpToMac;
	EFI_SIMPLE_NETWORK_NVDATA NvData;
	EFI_SIMPLE_NETWORK_GET_STATUS GetStatus;
	EFI_SIMPLE_NETWORK_TRANSMIT Transmit;
	EFI_SIMPLE_NETWORK_RECEIVE Receive;
	EFI_EVENT WaitForPacket;
	EFI_SIMPLE_NETWORK_MODE *Mode;
};

#endif /* __EFI_SNP_H */
