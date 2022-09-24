/** @file
 *
 *  Copyright (c) 2018, Linaro Ltd. All rights reserved.
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD
 *License which accompanies this distribution.  The full text of the license may
 *be found at http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
 *IMPLIED.
 *
 **/

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/EmbeddedGpio.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PlatformBootManager.h>
#include <Protocol/QcomSmem.h>

#include "sdm845Dxe.h"

EFI_CPU_ARCH_PROTOCOL *gCpu;

VOID InitPeripherals(IN VOID)
{
  EFI_STATUS              Status;
  EFI_QCOM_SMEM_PROTOCOL *pEfiSmemProtocol = NULL;
  UINT32                  Size             = 0;
  UINTN                  *pAddr;

  // Lock the QcomWdogTimer in a cage on certain devices
#ifdef SM7125_TEST
  MmioWrite32(0x17c10008, 0x000000);
#else
  MmioWrite32(0x17980008, 0x000000);
#endif
  DEBUG((EFI_D_WARN, "\n \v The Dog has been locked in a cage :)\v"));

#ifndef SM7125_TEST
  Status = gBS->LocateProtocol(
      &gQcomSMEMProtocolGuid, NULL, (VOID **)&pEfiSmemProtocol);

  Status = pEfiSmemProtocol->SmemGetAddr(137, &Size, (VOID **)&pAddr);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "%a: SmemGetAddr failed. %r\n", __FUNCTION__, Status));
  }

  DEBUG((EFI_D_ERROR, "%a: SmemGetAddr result: 0x%p\n", __FUNCTION__, pAddr));
#endif
  // gBS->Stall(5000000);
}

/**
  Notification function of the event defined as belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group that was created in
  the entry point of the driver.

  This function is called when an event belonging to the
  EFI_END_OF_DXE_EVENT_GROUP_GUID event group is signalled. Such an
  event is signalled once at the end of the dispatching of all
  drivers (end of the so called DXE phase).

  @param[in]  Event    Event declared in the entry point of the driver whose
                       notification function is being invoked.
  @param[in]  Context  NULL
**/
STATIC
VOID OnEndOfDxe(IN EFI_EVENT Event, IN VOID *Context) {}

EFI_STATUS
EFIAPI
sdm845EntryPoint(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_EVENT  EndOfDxeEvent;

  Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  ASSERT_EFI_ERROR(Status);

  InitPeripherals();

  //
  // Create an event belonging to the "gEfiEndOfDxeEventGroupGuid" group.
  // The "OnEndOfDxe()" function is declared as the call back function.
  // It will be called at the end of the DXE phase when an event of the
  // same group is signalled to inform about the end of the DXE phase.
  // Install the INSTALL_FDT_PROTOCOL protocol.
  //
  Status = gBS->CreateEventEx(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, OnEndOfDxe, NULL,
      &gEfiEndOfDxeEventGroupGuid, &EndOfDxeEvent);
  return Status;
}
