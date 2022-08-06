/** @file
*
*  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Soc.h>

// The total number of descriptors, including the final "end-of-table" descriptor.
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS 12

// DDR attributes
#define DDR_ATTRIBUTES_CACHED           ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED         ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

#define RK3588_EXTRA_SYSTEM_MEMORY_BASE  (FixedPcdGet64(PcdLcdDdrFrameBufferBase) + FixedPcdGet64(PcdLcdDdrFrameBufferSize))
#define RK3588_EXTRA_SYSTEM_MEMORY_SIZE  0xA0000000

#define RK3588_DISPLAY_FB_BASE           (FixedPcdGet64(PcdLcdDdrFrameBufferBase))
#define RK3588_DISPLAY_FB_SIZE           (FixedPcdGet64(PcdLcdDdrFrameBufferSize))

STATIC struct RK3588ReservedMemory {
  EFI_PHYSICAL_ADDRESS         Offset;
  EFI_PHYSICAL_ADDRESS         Size;
} RK3588ReservedMemoryBuffer [] = {
  { 0x00000000, 0x00200000 },    // ATF
  { 0x08400000, 0x01000000 },    // TEE OS
  { 0xF0000000, 0x10000000 },    // REG
//  { 0x0740F000, 0x00001000 },    // MAILBOX
//  { 0x21F00000, 0x00100000 },    // PSTORE/RAMOOPS
//  { 0x3E000000, 0x02000000 }     // TEE OS
};

STATIC
UINT64
EFIAPI
RK3588InitMemorySize (
  IN VOID
  )
{
  //UINT32               Data;
  UINT64               MemorySize;

  //Data = MmioRead32 (MDDRC_AXI_BASE + AXI_REGION_MAP);
  MemorySize = SIZE_8GB;//RK3588_REGION_SIZE(Data);
  return MemorySize;
}

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
  UINTN                         Index = 0, Count, ReservedTop;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  EFI_PEI_HOB_POINTERS          NextHob;
  EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;
  UINT64                        ResourceLength;
  EFI_PHYSICAL_ADDRESS          ResourceTop;
  UINT64                        MemorySize, AdditionalMemorySize;

  MemorySize = RK3588InitMemorySize ();
  if (MemorySize == 0) {
    MemorySize = PcdGet64 (PcdSystemMemorySize);
  }

  ResourceAttributes = (
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED
  );

  // Create initial Base Hob for system memory.
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    ResourceAttributes,
    PcdGet64 (PcdSystemMemoryBase),
    PcdGet64 (PcdSystemMemorySize)
  );

  NextHob.Raw = GetHobList ();
  Count = sizeof (RK3588ReservedMemoryBuffer) / sizeof (struct RK3588ReservedMemory);
  while ((NextHob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, NextHob.Raw)) != NULL) {
    if (Index >= Count) {
      break;
    }
    if ((NextHob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
        (RK3588ReservedMemoryBuffer[Index].Offset >= NextHob.ResourceDescriptor->PhysicalStart) &&
        ((RK3588ReservedMemoryBuffer[Index].Offset + RK3588ReservedMemoryBuffer[Index].Size) <=
         NextHob.ResourceDescriptor->PhysicalStart + NextHob.ResourceDescriptor->ResourceLength)) {
      ResourceAttributes = NextHob.ResourceDescriptor->ResourceAttribute;
      ResourceLength = NextHob.ResourceDescriptor->ResourceLength;
      ResourceTop = NextHob.ResourceDescriptor->PhysicalStart + ResourceLength;
      ReservedTop = RK3588ReservedMemoryBuffer[Index].Offset + RK3588ReservedMemoryBuffer[Index].Size;

      // Create the System Memory HOB for the reserved buffer
      BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_RESERVED,
                                  EFI_RESOURCE_ATTRIBUTE_PRESENT,
                                  RK3588ReservedMemoryBuffer[Index].Offset,
                                  RK3588ReservedMemoryBuffer[Index].Size);
      // Update the HOB
      NextHob.ResourceDescriptor->ResourceLength = RK3588ReservedMemoryBuffer[Index].Offset - NextHob.ResourceDescriptor->PhysicalStart;

      // If there is some memory available on the top of the reserved memory then create a HOB
      if (ReservedTop < ResourceTop) {
        BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY,
                                    ResourceAttributes,
                                    ReservedTop,
                                    ResourceTop - ReservedTop);
      }
      Index++;
    }
    NextHob.Raw = GET_NEXT_HOB (NextHob);
  }

  AdditionalMemorySize = MemorySize - PcdGet64 (PcdSystemMemorySize);
  if (AdditionalMemorySize >= SIZE_1GB) {
    // Declared the additional memory
    ResourceAttributes =
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED;

    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      ResourceAttributes,
      RK3588_EXTRA_SYSTEM_MEMORY_BASE,
      RK3588_EXTRA_SYSTEM_MEMORY_SIZE);
  }

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
    return;
  }

  CacheAttributes = DDR_ATTRIBUTES_CACHED;

  Index = 0;

  // RK3588 SOC peripherals
  VirtualMemoryTable[Index].PhysicalBase    = RK3588_PERIPH_BASE;
  VirtualMemoryTable[Index].VirtualBase     = RK3588_PERIPH_BASE;
  VirtualMemoryTable[Index].Length          = RK3588_PERIPH_SZ;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE;
#if 0
   BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    0xFE2B0000,
    0x00100000
  );

  BuildMemoryAllocationHob (
    0xFE2B0000,
    0x00100000,
    EfiRuntimeServicesData
  );
#endif

  //PCIe 64 BAR space
  VirtualMemoryTable[++Index].PhysicalBase    = 0x940000000;
  VirtualMemoryTable[Index].VirtualBase     = 0x940000000;
  VirtualMemoryTable[Index].Length          = 0x100000000 + 0x1400000;
  VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_NONSECURE_DEVICE;

  // DDR - predefined 1GB size
  VirtualMemoryTable[++Index].PhysicalBase  = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].VirtualBase     = PcdGet64 (PcdSystemMemoryBase);
  VirtualMemoryTable[Index].Length          = PcdGet64 (PcdSystemMemorySize);
  VirtualMemoryTable[Index].Attributes      = CacheAttributes;

  // If DDR capacity is 2GB size, append a new entry to fill the gap.
  if (AdditionalMemorySize >= SIZE_1GB) {
    VirtualMemoryTable[++Index].PhysicalBase = RK3588_EXTRA_SYSTEM_MEMORY_BASE;
    VirtualMemoryTable[Index].VirtualBase    = RK3588_EXTRA_SYSTEM_MEMORY_BASE;
    VirtualMemoryTable[Index].Length         = RK3588_EXTRA_SYSTEM_MEMORY_SIZE;
    VirtualMemoryTable[Index].Attributes     = CacheAttributes;
  }

  // display framebuffer reserved memory
  CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  if (AdditionalMemorySize >= SIZE_1GB) {
    VirtualMemoryTable[++Index].PhysicalBase = RK3588_DISPLAY_FB_BASE;
    VirtualMemoryTable[Index].VirtualBase    = RK3588_DISPLAY_FB_BASE;
    VirtualMemoryTable[Index].Length         = RK3588_DISPLAY_FB_SIZE;
    VirtualMemoryTable[Index].Attributes     = CacheAttributes;
  }

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase  = 0;
  VirtualMemoryTable[Index].VirtualBase     = 0;
  VirtualMemoryTable[Index].Length          = 0;
  VirtualMemoryTable[Index].Attributes      = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}
