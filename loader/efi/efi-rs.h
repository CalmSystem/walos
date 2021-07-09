/*
 * efi-rs.h
 *
 * UEFI Runtime Services.
 */

#ifndef __EFI_RS_H
#define __EFI_RS_H

#include <efi.h>
#include <efi-time.h>


#define EFI_OPTIONAL_POINTER                0x00000001

#define CAPSULE_FLAGS_PERSIST_ACROSS_RESET  0x00010000
#define CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE 0x00020000
#define CAPSULE_FLAGS_INITIATE_RESET        0x00040000


typedef enum EFI_RESET_TYPE {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef struct EFI_CAPSULE_BLOCK_DESCRIPTOR {
    uint64_t  Length;
    union {
        EFI_PHYSICAL_ADDRESS    DataBlock;
        EFI_PHYSICAL_ADDRESS    ContinuationPointer;
    } Union;
} EFI_CAPSULE_BLOCK_DESCRIPTOR;

typedef struct EFI_CAPSULE_HEADER {
    EFI_GUID    CapsuleGuid;
    uint32_t      HeaderSize;
    uint32_t      Flags;
    uint32_t      CapsuleImageSize;
} EFI_CAPSULE_HEADER;

typedef EFI_STATUS (EFIABI *EFI_GET_TIME)(EFI_TIME *Time, EFI_TIME_CAPABILITIES *Capabilities);
typedef EFI_STATUS (EFIABI *EFI_SET_TIME)(EFI_TIME *Time);
typedef EFI_STATUS (EFIABI *EFI_GET_WAKEUP_TIME)(bool8_t *Enabled, bool8_t *Pending, EFI_TIME *Time);
typedef EFI_STATUS (EFIABI *EFI_SET_WAKEUP_TIME)(bool8_t Enable, EFI_TIME *Time);
typedef EFI_STATUS (EFIABI *EFI_SET_VIRTUAL_ADDRESS_MAP)(uintn_t MemoryMapSize, uintn_t DescriptorSize, uint32_t DescriptorVersion, EFI_MEMORY_DESCRIPTOR *VirtualMap);
typedef EFI_STATUS (EFIABI *EFI_CONVERT_POINTER)(uintn_t DebugDisposition, void **Address);
typedef EFI_STATUS (EFIABI *EFI_GET_VARIABLE)(char16_t *VariableName, EFI_GUID *VendorGuid, uint32_t *Attributes, uintn_t *DataSize, void *Data);
typedef EFI_STATUS (EFIABI *EFI_GET_NEXT_VARIABLE_NAME)(uintn_t *VariableNameSize, char16_t *VariableName, EFI_GUID *VendorGuid);
typedef EFI_STATUS (EFIABI *EFI_SET_VARIABLE)(char16_t *VariableName, EFI_GUID *VendorGuid, uint32_t Attributes, uintn_t DataSize, void *Data);
typedef EFI_STATUS (EFIABI *EFI_GET_NEXT_HIGH_MONO_COUNT)(uint32_t *HighCount);
typedef EFI_STATUS (EFIABI *EFI_RESET_SYSTEM)(EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus, uintn_t DataSize, void *ResetData);
typedef EFI_STATUS (EFIABI *EFI_UPDATE_CAPSULE)(EFI_CAPSULE_HEADER **CapsuleHeaderArray, uintn_t CapsuleCount, EFI_PHYSICAL_ADDRESS ScatterGatherList);
typedef EFI_STATUS (EFIABI *EFI_QUERY_CAPSULE_CAPABILITIES)(EFI_CAPSULE_HEADER **CapsuleHeaderArray, uintn_t CapsuleCount, uint64_t *MaximumCapsuleSize, EFI_RESET_TYPE *ResetType);
typedef EFI_STATUS (EFIABI *EFI_QUERY_VARIABLE_INFO)(uint32_t Attributes, uint64_t *MaximumVariableStorageSize, uint64_t *RemainingVariableStorageSize, uint64_t *MaximumVariableSize);

typedef struct EFI_RUNTIME_SERVICES {
    EFI_TABLE_HEADER                Hdr;
    EFI_GET_TIME                    GetTime;
    EFI_SET_TIME                    SetTime;
    EFI_GET_WAKEUP_TIME             GetWakeupTime;
    EFI_SET_WAKEUP_TIME             SetWakeupTime;
    EFI_SET_VIRTUAL_ADDRESS_MAP     SetVirtualAddressMap;
    EFI_CONVERT_POINTER             ConvertPointer;
    EFI_GET_VARIABLE                GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME      GetNextVariableName;
    EFI_SET_VARIABLE                SetVariable;
    EFI_GET_NEXT_HIGH_MONO_COUNT    GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM                ResetSystem;
    EFI_UPDATE_CAPSULE              UpdateCapsule;
    EFI_QUERY_CAPSULE_CAPABILITIES  QueryCapsuleCapabilities;
    EFI_QUERY_VARIABLE_INFO         QueryVariableInfo;
} EFI_RUNTIME_SERVICES;


#endif /* __EFI_RS_H */
