/*
 * efi-fp.h
 *
 * EFI File Protocol.
 */

#ifndef __EFI_FP_H
#define __EFI_FP_H

#include <efi.h>


#define EFI_FILE_INFO_ID                {0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
#define EFI_FILE_SYSTEM_INFO_ID         {0x09576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}} 
#define EFI_FILE_SYSTEM_VOLUME_LABEL_ID {0xdb47d7d3, 0xfe81, 0x11d3, {0x9a, 0x35, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}

#define EFI_FILE_PROTOCOL_REVISION          0x00010000
#define EFI_FILE_PROTOCOL_REVISION2         0x00020000
#define EFI_FILE_PROTOCOL_LATEST_REVISION   EFI_FILE_PROTOCOL_REVISION2

#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

#define EFI_FILE_READ_ONLY  0x0000000000000001
#define EFI_FILE_HIDDEN     0x0000000000000002
#define EFI_FILE_SYSTEM     0x0000000000000004
#define EFI_FILE_RESERVED   0x0000000000000008
#define EFI_FILE_DIRECTORY  0x0000000000000010
#define EFI_FILE_ARCHIVE    0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037


typedef struct EFI_FILE_INFO {
    uint64_t      Size;
    uint64_t      FileSize;
    uint64_t      PhysicalSize;
    EFI_TIME    CreateTime;
    EFI_TIME    LastAccessTime;
    EFI_TIME    ModificationTime;
    uint64_t      Attribute;
    char16_t      FileName[];
} EFI_FILE_INFO;

typedef struct EFI_FILE_SYSTEM_INFO {
    uint64_t  Size;
    bool8_t ReadOnly;
    uint64_t  VolumeSize;
    uint64_t  FreeSpace;
    uint32_t  BlockSize;
    char16_t  VolumeLabel;
} EFI_FILE_SYSTEM_INFO;

typedef struct EFI_FILE_SYSTEM_VOLUME_LABEL {
    char16_t VolumeLabel[];
} EFI_FILE_SYSTEM_VOLUME_LABEL;

typedef struct EFI_FILE_IO_TOKEN {
    EFI_EVENT   Event;
    EFI_STATUS  Status;
    uintn_t       BufferSize;
    void        *Buffer;
} EFI_FILE_IO_TOKEN;


struct EFI_FILE_PROTOCOL;

typedef EFI_STATUS (*EFI_FILE_OPEN)(struct EFI_FILE_PROTOCOL *This, struct EFI_FILE_PROTOCOL **NewHandle, char16_t *FileName, uint64_t OpenMode, uint64_t Attributes);
typedef EFI_STATUS (*EFI_FILE_CLOSE)(struct EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_FILE_DELETE)(struct EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_FILE_READ)(struct EFI_FILE_PROTOCOL *This, uintn_t *BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_WRITE)(struct EFI_FILE_PROTOCOL *This, uintn_t *BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_GET_POSITION)(struct EFI_FILE_PROTOCOL *This, uint64_t *Position);
typedef EFI_STATUS (*EFI_FILE_SET_POSITION)(struct EFI_FILE_PROTOCOL *This, uint64_t Position);
typedef EFI_STATUS (*EFI_FILE_GET_INFO)(struct EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, uintn_t *BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_SET_INFO)(struct EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, uintn_t BufferSize, void *Buffer);
typedef EFI_STATUS (*EFI_FILE_FLUSH)(struct EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS (*EFI_FILE_OPEN_EX)(struct EFI_FILE_PROTOCOL *This, struct EFI_FILE_PROTOCOL **NewHandle, char16_t *FileName, uint64_t OpenMode, uint64_t Attributes, EFI_FILE_IO_TOKEN *Token);
typedef EFI_STATUS (*EFI_FILE_READ_EX)(struct EFI_FILE_PROTOCOL *This, EFI_FILE_IO_TOKEN *Token);
typedef EFI_STATUS (*EFI_FILE_WRITE_EX)(struct EFI_FILE_PROTOCOL *This, EFI_FILE_IO_TOKEN *Token);
typedef EFI_STATUS (*EFI_FILE_FLUSH_EX)(struct EFI_FILE_PROTOCOL *This, EFI_FILE_IO_TOKEN *Token);


typedef struct EFI_FILE_PROTOCOL {
    uint64_t                  Revision;
    EFI_FILE_OPEN           Open;
    EFI_FILE_CLOSE          Close;
    EFI_FILE_DELETE         Delete;
    EFI_FILE_READ           Read;
    EFI_FILE_WRITE          Write;
    EFI_FILE_GET_POSITION   GetPosition;
    EFI_FILE_SET_POSITION   SetPosition;
    EFI_FILE_GET_INFO       GetInfo;
    EFI_FILE_SET_INFO       SetInfo;
    EFI_FILE_FLUSH          Flush;
    EFI_FILE_OPEN_EX        OpenEx;
    EFI_FILE_READ_EX        ReadEx;
    EFI_FILE_WRITE_EX       WriteEx;
    EFI_FILE_FLUSH_EX       FlushEx;
} EFI_FILE_PROTOCOL;


#endif /* __EFI_FP_H */
