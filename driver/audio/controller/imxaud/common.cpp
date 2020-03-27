/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

Abstract:
    Implementation of the AdapterCommon class.  All one-time init code belongs here.

*/

//4127: conditional expression is constant
//4595: illegal inline operator
#pragma warning (disable : 4127)
#pragma warning (disable : 4595)


#include "imx_audio.h"
#include "simple.h"
#include "minwavertstream.h"

#include "soc.h"

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CAdapterCommon
//   

class CAdapterCommon : 
    public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown    
{
    private:
        CSoc                    m_Soc;
        PSERVICEGROUP           m_pServiceGroupWave;
        PDEVICE_OBJECT          m_pDeviceObject;
        PDEVICE_OBJECT          m_pPhysicalDeviceObject;
        WDFDEVICE               m_WdfDevice;            // Wdf device. 
        WDFIOTARGET             m_RhHubIoTarget;
        DEVICE_POWER_STATE      m_PowerState;  
        PPORTCLSETWHELPER       m_pPortClsEtwHelper;

        static LONG             m_AdapterInstances;     // # of adapter objects.

        DWORD                   m_dwIdleRequests;

        CM_PARTIAL_RESOURCE_DESCRIPTOR   m_InterruptDescriptor;

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
        ~CAdapterCommon();

        //=====================================================================
        // Default IAdapterPowerManagement
        IMP_IAdapterPowerManagement;

        //=====================================================================
        // IAdapterCommon methods      

        STDMETHODIMP_(NTSTATUS) Init
        (   
            _In_  PDEVICE_OBJECT  DeviceObject,
            _In_  PRESOURCELIST   ResourceList
        );

        STDMETHODIMP_(PDEVICE_OBJECT)   GetDeviceObject(VOID);
        
        STDMETHODIMP_(PDEVICE_OBJECT)   GetPhysicalDeviceObject(VOID);
        
        STDMETHODIMP_(WDFDEVICE)        GetWdfDevice(VOID);

        STDMETHODIMP_(VOID)             SetWaveServiceGroup
        (   
            _In_  PSERVICEGROUP   ServiceGroup
        );

        STDMETHODIMP_(NTSTATUS) WriteEtwEvent 
        ( 
            _In_ EPcMiniportEngineEvent    MiniportEventType,
            _In_ ULONGLONG      Data1,
            _In_ ULONGLONG      Data2,
            _In_ ULONGLONG      Data3,
            _In_ ULONGLONG      Data4
        );

        STDMETHODIMP_(VOID)     SetEtwHelper 
        ( 
            _In_ PPORTCLSETWHELPER PortClsEtwHelper
        );
        
        STDMETHODIMP_(NTSTATUS) InstallSubdevice
        ( 
            _In_opt_        PIRP                                    Irp,
            _In_            PWSTR                                   Name,
            _In_            REFGUID                                 PortClassId,
            _In_            REFGUID                                 MiniportClassId,
            _In_opt_        PFNCREATEMINIPORT                       MiniportCreate,
            _In_opt_        PVOID                                   DeviceContext,
            _In_            PENDPOINT_MINIPAIR                      MiniportPair,
            _In_opt_        PRESOURCELIST                           ResourceList,
            _In_            REFGUID                                 PortInterfaceId,
            _Out_opt_       PUNKNOWN                              * OutPortInterface,
            _Out_opt_       PUNKNOWN                              * OutPortUnknown,
            _Out_opt_       PUNKNOWN                              * OutMiniportUnknown
        );
        
        STDMETHODIMP_(NTSTATUS) UnregisterSubdevice
        (
            _In_opt_ PUNKNOWN               UnknownPort
        );
        
        STDMETHODIMP_(NTSTATUS) ConnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );
        
        STDMETHODIMP_(NTSTATUS) DisconnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );
        
        STDMETHODIMP_(NTSTATUS) InstallEndpointFilters
        (
            _In_opt_    PIRP                Irp, 
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PVOID               DeviceContext,
            _Out_opt_   PUNKNOWN *          UnknownTopology,
            _Out_opt_   PUNKNOWN *          UnknownWave
        );
        
        STDMETHODIMP_(NTSTATUS) RemoveEndpointFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PUNKNOWN            UnknownTopology,
            _In_opt_    PUNKNOWN            UnknownWave
        );

        STDMETHODIMP_(NTSTATUS) GetFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _Out_opt_   PUNKNOWN            *UnknownTopologyPort,
            _Out_opt_   PUNKNOWN            *UnknownTopologyMiniport,
            _Out_opt_   PUNKNOWN            *UnknownWavePort,
            _Out_opt_   PUNKNOWN            *UnknownWaveMiniport
        );

        STDMETHODIMP_(NTSTATUS) SetIdlePowerManagement
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_        BOOL                Enabled
        );

        STDMETHODIMP_(NTSTATUS) RegisterStream
        (
            _In_        CMiniportWaveRTStream* Stream,
                        eDeviceType DeviceType
        );

        STDMETHODIMP_(NTSTATUS) UnregisterStream
        (

            _In_        CMiniportWaveRTStream* Stream,
                        eDeviceType DeviceType
        );

        STDMETHODIMP_(NTSTATUS) StartDma
        (
            _In_        CMiniportWaveRTStream* Stream
        );

        STDMETHODIMP_(NTSTATUS) PauseDma
        (
            _In_        CMiniportWaveRTStream* Stream
        );

        STDMETHODIMP_(NTSTATUS) StopDma
        (
            _In_        CMiniportWaveRTStream* Stream
        );


        //=====================================================================
        // friends
        friend NTSTATUS         NewAdapterCommon
        ( 
            _Out_       PUNKNOWN *              Unknown,
            _In_        REFCLSID                RefClsId,
            _In_opt_    PUNKNOWN                UnknownOuter,
            _When_((PoolType & NonPagedPoolMustSucceed) != 0,
                __drv_reportError("Must succeed pool allocations are forbidden. "
                        "Allocation failures cause a system crash"))
            _In_        POOL_TYPE               PoolType 
        );

        STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) GetInterruptResourceDescriptor(VOID);

    private:

    LIST_ENTRY m_SubdeviceCache;

    NTSTATUS GetCachedSubdevice
    (
        _In_ PWSTR Name,
        _Out_opt_ PUNKNOWN *OutUnknownPort,
        _Out_opt_ PUNKNOWN *OutUnknownMiniport
    );

    NTSTATUS CacheSubdevice
    (
        _In_ PWSTR Name,
        _In_ PUNKNOWN UnknownPort,
        _In_ PUNKNOWN UnknownMiniport
    );
    
    NTSTATUS RemoveCachedSubdevice
    (
        _In_ PWSTR Name
    );

    NTSTATUS InitializeMux(ULONG SsiPort, ULONG ExternalPort);

    _Must_inspect_result_
    __drv_requiresIRQL(PASSIVE_LEVEL)
    NTSTATUS
    QueryResourceHubBiosDescriptor
    (
        __in LONGLONG ConnectionId,
        __out __deref __drv_allocatesMem(Mem)
        PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER *BiosDescriptorOut
    );

    _Must_inspect_result_
    __drv_requiresIRQL(PASSIVE_LEVEL)
    NTSTATUS
    OpenResourceHubTarget();

    VOID FillFifos();

    VOID DrainFifos();

    VOID DisableInterrupts();

    VOID EnableInterrupts();

    VOID StartDma();

    VOID StopDma();

    BOOLEAN HandleInterrupt();


};

typedef struct _MINIPAIR_UNKNOWN
{
    LIST_ENTRY              ListEntry;
    WCHAR                   Name[MAX_PATH];
    PUNKNOWN                PortInterface;
    PUNKNOWN                MiniportInterface;
    PADAPTERPOWERMANAGEMENT PowerInterface;
} MINIPAIR_UNKNOWN;

//
// Used to implement the singleton pattern.
//
LONG  CAdapterCommon::m_AdapterInstances = 0;


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
NewAdapterCommon
(
    PUNKNOWN    *Unknown,
    REFCLSID    RefClsId,
    PUNKNOWN    UnknownOuter,
    POOL_TYPE   PoolType 
)
/*++

Routine Description:

    Creates a new CAdapterCommon

Arguments:

    Unknown - adapter interface

    RefClsId - class id

    UnknownOuter - 

    PoolType - memory pool tag to use

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(RefClsId);

    ASSERT(Unknown);

    NTSTATUS ntStatus;

    InterlockedIncrement(&CAdapterCommon::m_AdapterInstances);

    //
    // Allocate an adapter object.
    //
    CAdapterCommon *p = new(PoolType, MINADAPTER_POOLTAG) CAdapterCommon(UnknownOuter);
    if (p == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_ERROR, ("NewAdapterCommon failed, 0x%x", ntStatus));
        goto Done;
    }

    // 
    // Success.
    //
    *Unknown = PUNKNOWN((PADAPTERCOMMON)(p));
    (*Unknown)->AddRef(); 
    ntStatus = STATUS_SUCCESS; 

Done:    
    return ntStatus;
} 

//=============================================================================
#pragma code_seg("PAGE")
CAdapterCommon::~CAdapterCommon
( 
    VOID 
)
/*++

Routine Description:

    Destructor for CAdapterCommon.

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::~CAdapterCommon]"));

    SAFE_RELEASE(m_pPortClsEtwHelper);
    SAFE_RELEASE(m_pServiceGroupWave);
 
    if (m_WdfDevice)
    {
        WdfObjectDelete(m_WdfDevice);
        m_WdfDevice = NULL;
    }

    InterlockedDecrement(&CAdapterCommon::m_AdapterInstances);
    ASSERT(CAdapterCommon::m_AdapterInstances == 0);
} 

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)
CAdapterCommon::GetDeviceObject
(
    VOID
)
/*++

Routine Description:

    Returns the device object

Arguments:

    None

Return Value:

    Device object of the adapter

--*/
{
    PAGED_CODE();
    
    return m_pDeviceObject;
} 

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)
CAdapterCommon::GetPhysicalDeviceObject
(
    VOID
)
/*++

Routine Description:

    Returns the PDO.

Arguments:

    None

Return Value:

    PDO

--*/
{
    PAGED_CODE();
    
    return m_pPhysicalDeviceObject;
} 

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(WDFDEVICE)
CAdapterCommon::GetWdfDevice
(
    VOID
)
/*++

Routine Description:

    Returns the associated WDF miniport device. Note that this is NOT an audio
    miniport. The WDF miniport device is the WDF device associated with the
    adapter.

Arguments:

    None

Return Value:

    WDFDEVICE

--*/
{
    PAGED_CODE();
    
    return m_WdfDevice;
} 

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
CAdapterCommon::Init
( 
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PRESOURCELIST ResourceList
)
/*++

Routine Description:

    Initialize adapter common object.

Arguments:

    DeviceObject - pointer to the device object

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::Init]"));

    ASSERT(DeviceObject);

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    ULONG                           index       = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor, interruptDescriptor;
    PSAI_REGISTERS pSaiRegisters;

    m_pServiceGroupWave     = NULL;
    m_pDeviceObject         = DeviceObject;
    m_pPhysicalDeviceObject = NULL;
    m_WdfDevice             = NULL;
    m_RhHubIoTarget         = NULL;
    m_PowerState            = PowerDeviceD0;
    m_pPortClsEtwHelper     = NULL;
    InitializeListHead(&m_SubdeviceCache);

    //
    // Get the PDO.
    //
    ntStatus = PcGetPhysicalDeviceObject(DeviceObject, &m_pPhysicalDeviceObject);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("PcGetPhysicalDeviceObject failed, 0x%x", ntStatus)),
        Done);

    //
    // Create a WDF miniport to represent the adapter. Note that WDF miniports 
    // are NOT audio miniports. An audio adapter is associated with a single WDF
    // miniport. This driver uses WDF to simplify the handling of the Bluetooth
    // SCO HFP Bypass interface.
    //
    ntStatus = WdfDeviceMiniportCreate( WdfGetDriver(),
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        DeviceObject,           // FDO
                                        NULL,                   // Next device.
                                        NULL,                   // PDO
                                       &m_WdfDevice);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("WdfDeviceMiniportCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Map resources
    // 

    //
    // Find the interrupt resource descriptor for use by the soc class.
    //
    interruptDescriptor = ResourceList->FindTranslatedEntry(CmResourceTypeInterrupt, 0);
    ASSERT(interruptDescriptor);

    if (interruptDescriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    for (index = 0; index < ResourceList->NumberOfEntries(); index++)
    {
        descriptor = ResourceList->FindTranslatedEntry(CmResourceTypeMemory, index);
        if (descriptor != NULL)
        {
            if (descriptor->u.Memory.Length >= sizeof(SaiRegisters))
            {
                pSaiRegisters = (PSAI_REGISTERS) MmMapIoSpaceEx(descriptor->u.Memory.Start, 
                                                                               sizeof(SaiRegisters), 
                                                                               PAGE_READWRITE | PAGE_NOCACHE);
                ASSERT(pSaiRegisters);
                if (pSaiRegisters == NULL)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    goto Done;
                }
                m_Soc.InitSsiBlock(pSaiRegisters, interruptDescriptor, m_pPhysicalDeviceObject);
            }
        }
    }

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::WriteEtwEvent
( 
    EPcMiniportEngineEvent  MiniportEventType,
    ULONGLONG               Data1,
    ULONGLONG               Data2,
    ULONGLONG               Data3,
    ULONGLONG               Data4
)
/*++

Routine Description:

    Logs an miniport ETW event.

Arguments:

    MiniportEventType - event type
    
    Data1 - first event parameter
    
    Data2 - second event paramter
    
    Data3 - third event parameter
    
    Data4 - fourth event parameter

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_pPortClsEtwHelper)
    {
        ntStatus = m_pPortClsEtwHelper->MiniportWriteEtwEvent( MiniportEventType, Data1, Data2, Data3, Data4) ;
    }
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(VOID)
CAdapterCommon::SetEtwHelper
( 
    _In_ PPORTCLSETWHELPER PortClsEtwHelper
)
/*++

Routine Description:

    Set the ETW helper.

Arguments:

    PortClsEtwHelper - ETW helper

Return Value:

    None

--*/
{
    PAGED_CODE();
    
    SAFE_RELEASE(m_pPortClsEtwHelper);

    m_pPortClsEtwHelper = PortClsEtwHelper;

    if (m_pPortClsEtwHelper)
    {
        m_pPortClsEtwHelper->AddRef();
    }
} 

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP
CAdapterCommon::NonDelegatingQueryInterface
( 
    REFIID  Interface,
    PVOID   *Object 
)
/*++

Routine Description:

    QueryInterface routine for AdapterCommon

Arguments:

    Interface - interface id

    Object - object

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(VOID)
CAdapterCommon::SetWaveServiceGroup
( 
    PSERVICEGROUP ServiceGroup 
)
/*++

Routine Description:

    Set the service group.

Arguments:

    ServiceGroup - service group info

Return Value:

    None

--*/
{
    PAGED_CODE();
 
    DPF_ENTER(("[CAdapterCommon::SetWaveServiceGroup]"));
    
    SAFE_RELEASE(m_pServiceGroupWave);

    m_pServiceGroupWave = ServiceGroup;

    if (m_pServiceGroupWave)
    {
        m_pServiceGroupWave->AddRef();
    }
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
STDMETHODIMP_(VOID)
CAdapterCommon::PowerChangeState
( 
    POWER_STATE NewState 
)
/*++

Routine Description:

    Handle power state changes.

Arguments:

    NewState - The requested, new power state for the device. 

Return Value:

    None

Note:
  From MSDN:

  To assist the driver, PortCls will pause any active audio streams prior to calling
  this method to place the device in a sleep state. After calling this method, PortCls
  will unpause active audio streams, to wake the device up. Miniports can opt for 
  additional notification by utilizing the IPowerNotify interface.

  The miniport driver must perform the requested change to the device's power state 
  before it returns from the PowerChangeState call. If the miniport driver needs to 
  save or restore any device state before a power-state change, the miniport driver 
  should support the IPowerNotify interface, which allows it to receive advance warning
  of any such change. Before returning from a successful PowerChangeState call, the 
  miniport driver should cache the new power state.

  While the miniport driver is in one of the sleep states (any state other than 
  PowerDeviceD0), it must avoid writing to the hardware. The miniport driver must cache
  any hardware accesses that need to be deferred until the device powers up again. If
  the power state is changing from one of the sleep states to PowerDeviceD0, the 
  miniport driver should perform any deferred hardware accesses after it has powered up
  the device. If the power state is changing from PowerDeviceD0 to a sleep state, the 
  miniport driver can perform any necessary hardware accesses during the PowerChangeState
  call before it powers down the device.

  While powered down, a miniport driver is never asked to create a miniport driver object
  or stream object. PortCls always places the device in the PowerDeviceD0 state before
  calling the miniport driver's NewStream method.
  
--*/
{
    DPF_ENTER(("[CAdapterCommon::PowerChangeState]"));

    // Notify all registered miniports of a power state change
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            pRecord->PowerInterface->PowerChangeState(NewState);
        }
    }

    // is this actually a state change??
    //
    if (NewState.DeviceState != m_PowerState)
    {
        // switch on new state
        //
        switch (NewState.DeviceState)
        {
            case PowerDeviceD0:
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                m_PowerState = NewState.DeviceState;

                DPF
                ( 
                    D_VERBOSE, 
                    ("Entering D%u", ULONG(m_PowerState) - ULONG(PowerDeviceD0)) 
                );

                break;
    
            default:
            
                DPF(D_VERBOSE, ("Unknown Device Power State"));
                break;
        }
    }
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryDeviceCapabilities
( 
    PDEVICE_CAPABILITIES PowerDeviceCaps 
)
/*++

Routine Description:

    Called at startup to get the caps for the device.  This structure provides 
    the system with the mappings between system power state and device power 
    state.  This typically will not need modification by the driver.         

Arguments:

    PowerDeviceCaps - The device's capabilities. 

Return Value:

    NT status code

--*/
{
    DPF_ENTER(("[CAdapterCommon::QueryDeviceCapabilities]"));

    if (PowerDeviceCaps->Size != sizeof(DEVICE_CAPABILITIES))
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg()
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryPowerChangeState
( 
    POWER_STATE NewStateQuery 
)
/*++

Routine Description:

    Query to see if the device can change to this power state 

Arguments:

    NewStateQuery - The requested, new power state for the device

Return Value:

    NT status code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    DPF_ENTER(("[CAdapterCommon::QueryPowerChangeState]"));

    // query each miniport for it's power state, we're finished if even one indicates
    // it cannot go to this power state.
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && NT_SUCCESS(status); le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            status = pRecord->PowerInterface->QueryPowerChangeState(NewStateQuery);
        }
    }

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallSubdevice
( 
    PIRP               Irp,
    PWSTR              Name,
    REFGUID            PortClassId,
    REFGUID            MiniportClassId,
    PFNCREATEMINIPORT  MiniportCreate,
    PVOID              DeviceContext,
    PENDPOINT_MINIPAIR MiniportPair,
    PRESOURCELIST      ResourceList,
    REFGUID            PortInterfaceId,
    PUNKNOWN           *OutPortInterface,
    PUNKNOWN           *OutPortUnknown,
    PUNKNOWN           *OutMiniportUnknown
)
{
/*++

Routine Description:

    This function creates and registers a subdevice consisting of a port       
    driver, a minport driver and a set of resources bound together.  It will   
    also optionally place a pointer to an interface on the port driver in a    
    specified location before initializing the port driver.  This is done so   
    that a common ISR can have access to the port driver during 
    initialization, when the ISR might fire.                                   

Arguments:

    Irp - pointer to the irp object.

    Name - name of the miniport. Passes to PcRegisterSubDevice
 
    PortClassId - port class id. Passed to PcNewPort.

    MiniportClassId - miniport class id. Passed to PcNewMiniport.

    MiniportCreate - pointer to a miniport creation function. If NULL, 
                     PcNewMiniport is used.
                     
    DeviceContext - deviceType specific.

    MiniportPair - endpoint configuration info.    

    ResourceList - pointer to the resource list.

    PortInterfaceId - GUID that represents the port interface.
       
    OutPortInterface - pointer to store the port interface

    OutPortUnknown - pointer to store the unknown port interface.

    OutMiniportUnknown - pointer to store the unknown miniport interface

Return Value:

    NT status code

--*/
    PAGED_CODE();

    DPF_ENTER(("[InstallSubDevice %S]", Name));

    ASSERT(Name != NULL);
    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS                    ntStatus;
    PPORT                       port            = NULL;
    PUNKNOWN                    miniport        = NULL;
    PADAPTERCOMMON              adapterCommon   = NULL;

    adapterCommon = PADAPTERCOMMON(this);

    // Create the port driver object
    //
    ntStatus = PcNewPort(&port, PortClassId);

    // Create the miniport object
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (MiniportCreate)
        {
            ntStatus = 
                MiniportCreate
                ( 
                    &miniport,
                    MiniportClassId,
                    NULL,
                    NonPagedPoolNx,
                    adapterCommon,
                    DeviceContext,
                    MiniportPair
                );
        }
        else
        {
            ntStatus = 
                PcNewMiniport
                (
                    (PMINIPORT *) &miniport, 
                    MiniportClassId
                );
        }
    }

    // Init the port driver and miniport in one go.
    //
    if (NT_SUCCESS(ntStatus))
    {
#pragma warning(push)
        // IPort::Init's annotation on ResourceList requires it to be non-NULL.  However,
        // for dynamic devices, we may no longer have the resource list and this should
        // still succeed.
        //
#pragma warning(disable:6387)
        ntStatus = 
            port->Init
            ( 
                m_pDeviceObject,
                Irp,
                miniport,
                adapterCommon,
                ResourceList 
            );
#pragma warning (pop)

        if (NT_SUCCESS(ntStatus))
        {
            // Register the subdevice (port/miniport combination).
            //
            ntStatus = 
                PcRegisterSubdevice
                ( 
                    m_pDeviceObject,
                    Name,
                    port 
                );
        }
    }

    // Deposit the port interfaces if it's needed.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (OutPortUnknown)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    IID_IUnknown,
                    (PVOID *)OutPortUnknown 
                );
        }

        if (OutPortInterface)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    PortInterfaceId,
                    (PVOID *) OutPortInterface 
                );
        }

        if (OutMiniportUnknown)
        {
            ntStatus = 
                miniport->QueryInterface
                ( 
                    IID_IUnknown,
                    (PVOID *)OutMiniportUnknown 
                );
        }

    }

    if (port)
    {
        port->Release();
    }

    if (miniport)
    {
        miniport->Release();
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::UnregisterSubdevice
(
    PUNKNOWN UnknownPort
)
/*++

Routine Description:

    Unregisters and releases the specified subdevice.

Arguments:

    UnknownPort - Wave or topology port interface.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::UnregisterSubdevice]"));

    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS                ntStatus            = STATUS_SUCCESS;
    PUNREGISTERSUBDEVICE    unregisterSubdevice = NULL;
    
    if (NULL == UnknownPort)
    {
        return ntStatus;
    }

    //
    // Get the IUnregisterSubdevice interface.
    //
    ntStatus = UnknownPort->QueryInterface( 
        IID_IUnregisterSubdevice,
        (PVOID *)&unregisterSubdevice);

    //
    // Unregister the port object.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = unregisterSubdevice->UnregisterSubdevice(
            m_pDeviceObject,
            UnknownPort);

        //
        // Release the IUnregisterSubdevice interface.
        //
        unregisterSubdevice->Release();
    }
    
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::ConnectTopologies
(
    PUNKNOWN                   UnknownTopology,
    PUNKNOWN                   UnknownWave,
    PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

    Connects the bridge pins between the wave and mixer topologies.

Arguments:

    None

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::ConnectTopologies]"));
    
    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS        ntStatus            = STATUS_SUCCESS;

    //
    // register wave <=> topology connections
    // This will connect bridge pins of wave and topology
    // miniports.
    //
    for (ULONG i = 0; i < PhysicalConnectionCount && NT_SUCCESS(ntStatus); i++)
    {
    
        switch(PhysicalConnections[i].eType)
        {
            case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    ( 
                        m_pDeviceObject,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology,
                        UnknownWave,
                        PhysicalConnections[i].ulWave
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, ("ConnectTopologies: PcRegisterPhysicalConnection(render) failed, 0x%x", ntStatus));
                }
                break;
            case CONNECTIONTYPE_WAVE_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    ( 
                        m_pDeviceObject,
                        UnknownWave,
                        PhysicalConnections[i].ulWave,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, ("ConnectTopologies: PcRegisterPhysicalConnection(capture) failed, 0x%x", ntStatus));
                }
                break;
        }
    }    

    //
    // Cleanup in case of error.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        // disconnect all connections on error, ignore error code because not all
        // connections may have been made
        DisconnectTopologies(UnknownTopology, UnknownWave, PhysicalConnections, PhysicalConnectionCount);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::DisconnectTopologies
(
    PUNKNOWN                   UnknownTopology,
    PUNKNOWN                   UnknownWave,
    PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

    Disconnects the bridge pins between the wave and mixer topologies.

Arguments:

    None

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::DisconnectTopologies]"));
    
    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS                        ntStatus                        = STATUS_SUCCESS;
    NTSTATUS                        ntStatus2                       = STATUS_SUCCESS;
    PUNREGISTERPHYSICALCONNECTION   unregisterPhysicalConnection    = NULL;

    //
    // Get the IUnregisterPhysicalConnection interface
    //
    ntStatus = UnknownTopology->QueryInterface( 
        IID_IUnregisterPhysicalConnection,
        (PVOID *)&unregisterPhysicalConnection);
    
    if (NT_SUCCESS(ntStatus))
    { 
        for (ULONG i = 0; i < PhysicalConnectionCount; i++)
        {
            switch(PhysicalConnections[i].eType)
            {
                case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology,
                            UnknownWave,
                            PhysicalConnections[i].ulWave
                        );

                    if (!NT_SUCCESS(ntStatus))
                    {
                        DPF(D_TERSE, ("DisconnectTopologies: UnregisterPhysicalConnection(render) failed, 0x%x", ntStatus));
                    }
                    break;
                case CONNECTIONTYPE_WAVE_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownWave,
                            PhysicalConnections[i].ulWave,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology
                        );
                    if (!NT_SUCCESS(ntStatus2))
                    {
                        DPF(D_TERSE, ("DisconnectTopologies: UnregisterPhysicalConnection(capture) failed, 0x%x", ntStatus2));
                    }
                    break;
            }

            // cache and return the first error encountered, as it's likely the most relevent
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = ntStatus2;
            }
        }    
    }
    
    //
    // Release the IUnregisterPhysicalConnection interface.
    //
    SAFE_RELEASE(unregisterPhysicalConnection);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
CAdapterCommon::GetCachedSubdevice
(
    PWSTR Name,
    PUNKNOWN *OutUnknownPort,
    PUNKNOWN *OutUnknownMiniport
)
/*++

Routine Description:

    Get cached sub device objects.

Arguments:

    Name - name of the subdevice

    OutUnknownPort - sub device object

    OutUnknownMiniport - miniport object

Return Value:

    NT status code

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CAdapterCommon::GetCachedSubdevice]"));

    // search list, return interface to device if found, fail if not found
    PLIST_ENTRY le = NULL;
    BOOL bFound = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bFound; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            if (OutUnknownPort)
            {
                *OutUnknownPort = pRecord->PortInterface;
                (*OutUnknownPort)->AddRef();
            }

            if (OutUnknownMiniport)
            {
                *OutUnknownMiniport = pRecord->MiniportInterface;
                (*OutUnknownMiniport)->AddRef();
            }

            bFound = TRUE;
        }
    }

    return bFound?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}



//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
CAdapterCommon::CacheSubdevice
(
    PWSTR Name,
    PUNKNOWN UnknownPort,
    PUNKNOWN UnknownMiniport
)
/*++

Routine Description:

    Cache sub device objects.

Arguments:

    Name - name of the subdevice

    OutUnknownPort - sub device object

    OutUnknownMiniport - miniport object

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::CacheSubdevice]"));

    // add the item with this name/interface to the list
    NTSTATUS         ntStatus       = STATUS_SUCCESS;
    MINIPAIR_UNKNOWN *pNewSubdevice = NULL;

    pNewSubdevice = new(NonPagedPoolNx, MINADAPTER_POOLTAG) MINIPAIR_UNKNOWN;

    if (!pNewSubdevice)
    {
        DPF(D_TERSE, ("Insufficient memory to cache subdevice"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        memset(pNewSubdevice, 0, sizeof(MINIPAIR_UNKNOWN));

        pNewSubdevice->PortInterface = UnknownPort;
        pNewSubdevice->PortInterface->AddRef();

        pNewSubdevice->MiniportInterface = UnknownMiniport;
        pNewSubdevice->MiniportInterface->AddRef();

        // cache the IAdapterPowerManagement interface (if available) from the filter. Some endpoints,
        // like FM and cellular, have their own power requirements that we must track. If this fails,
        // it just means this filter doesn't do power management.
        UnknownMiniport->QueryInterface(IID_IAdapterPowerManagement, (PVOID *)&(pNewSubdevice->PowerInterface));

        ntStatus = RtlStringCchCopyW(pNewSubdevice->Name, SIZEOF_ARRAY(pNewSubdevice->Name), Name);
        if (NT_SUCCESS(ntStatus))
        {
            InsertTailList(&m_SubdeviceCache, &pNewSubdevice->ListEntry);
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (pNewSubdevice)
        {
            delete pNewSubdevice;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
NTSTATUS
CAdapterCommon::RemoveCachedSubdevice
(
    PWSTR Name
)
/*++

Routine Description:

    Delete sub device from cache.

Arguments:

    Name - name of the subdevice

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::RemoveCachedSubdevice]"));

    // search list, remove the entry from the list

    PLIST_ENTRY le = NULL;
    BOOL bRemoved = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bRemoved; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            // grab the next entry, befor removal
            PLIST_ENTRY leNext = le->Flink;

            SAFE_RELEASE(pRecord->PortInterface);
            SAFE_RELEASE(pRecord->MiniportInterface);
            SAFE_RELEASE(pRecord->PowerInterface);
            memset(pRecord->Name, 0, sizeof(pRecord->Name));
            RemoveEntryList(le);
            bRemoved = TRUE;
            delete le;
            le = leNext;
        }
    }

    return bRemoved?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}



//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallEndpointFilters
(
    PIRP                Irp, 
    PENDPOINT_MINIPAIR  MiniportPair,
    PVOID               DeviceContext,
    PUNKNOWN *          UnknownTopology,
    PUNKNOWN *          UnknownWave
)
/*++

Routine Description:

    Install endpoint filters.

Arguments:

    Irp - IRP of the device install
    
    MiniportPair - miniport toplogy and wave definition

    DeviceContext - device context

    UnknownTopology - topology interface

    UnknownWave - wave interface

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::InstallEndpointFilters]"));
    
    NTSTATUS            ntStatus            = STATUS_SUCCESS;
    PUNKNOWN            unknownTopology     = NULL;
    PUNKNOWN            unknownWave         = NULL;
    BOOL                bTopologyCreated    = FALSE;
    BOOL                bWaveCreated        = FALSE;


    if (UnknownTopology)
    {
        *UnknownTopology = NULL;
    }

    if (UnknownWave)
    {
        *UnknownWave = NULL;
    }

    ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopology, NULL);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownTopology)
    {
        PUNKNOWN unknownMiniportTopology = NULL;

        bTopologyCreated = TRUE;

        // Install IMXWAV topology miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->TopoName, // make sure this name matches with IMXWAV.<TopoName>.szPname in the inf's [Strings] section
                                    CLSID_PortTopology,
                                    CLSID_PortTopology, 
                                    MiniportPair->TopoCreateCallback,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortTopology,
                                    NULL,
                                    &unknownTopology,
                                    &unknownMiniportTopology
                                    );
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->TopoName, unknownTopology, unknownMiniportTopology);
        }

        SAFE_RELEASE(unknownMiniportTopology);
    }

    ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWave, NULL);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownWave)
    {
        PUNKNOWN unknownMiniportWave     = NULL;

        bWaveCreated = TRUE;

        // Install IMXWAV wave miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->WaveName, // make sure this name matches with IMXWAV.<WaveName>.szPname in the inf's [Strings] section
                                    CLSID_PortWaveRT,
                                    CLSID_PortWaveRT,   
                                    MiniportPair->WaveCreateCallback,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortWaveRT,
                                    NULL, 
                                    &unknownWave,
                                    &unknownMiniportWave
                                    );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->WaveName, unknownWave, unknownMiniportWave);
        }

        SAFE_RELEASE(unknownMiniportWave);
    }

    if (unknownTopology && unknownWave)
    {
        //
        // register wave <=> topology connections
        // This will connect bridge pins of wave and topology
        // miniports.
        //
        ntStatus = ConnectTopologies(
            unknownTopology,
            unknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Set output parameters.
        //
        if (UnknownTopology != NULL && unknownTopology != NULL)
        {
            unknownTopology->AddRef();
            *UnknownTopology = unknownTopology;
        }
        
        if (UnknownWave != NULL && unknownWave != NULL)
        {
            unknownWave->AddRef();
            *UnknownWave = unknownWave;
        }
    }
    else
    {
        if (bTopologyCreated && unknownTopology != NULL)
        {
            UnregisterSubdevice(unknownTopology);
            RemoveCachedSubdevice(MiniportPair->TopoName);
        }
            
        if (bWaveCreated && unknownWave != NULL)
        {
            UnregisterSubdevice(unknownWave);
            RemoveCachedSubdevice(MiniportPair->WaveName);
        }
    }
   
    SAFE_RELEASE(unknownTopology);
    SAFE_RELEASE(unknownWave);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::RemoveEndpointFilters
(
    PENDPOINT_MINIPAIR  MiniportPair,
    PUNKNOWN            UnknownTopology,
    PUNKNOWN            UnknownWave
)
/*++

Routine Description:

    Uninstall endpoint filters.

Arguments:

    MiniportPair - miniport toplogy and wave definition

    UnknownTopology - topology interface

    UnknownWave - wave interface

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::RemoveEndpointFilters]"));
    
    NTSTATUS    ntStatus   = STATUS_SUCCESS;
    
    if (UnknownTopology != NULL && UnknownWave != NULL)
    {
        ntStatus = DisconnectTopologies(
            UnknownTopology,
            UnknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_VERBOSE, ("RemoveEndpointFilters: DisconnectTopologies failed: 0x%x", ntStatus));
        }
    }

        
    RemoveCachedSubdevice(MiniportPair->WaveName);

    ntStatus = UnregisterSubdevice(UnknownWave);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, ("RemoveEndpointFilters: UnregisterSubdevice(wave) failed: 0x%x", ntStatus));
    }

    RemoveCachedSubdevice(MiniportPair->TopoName);

    ntStatus = UnregisterSubdevice(UnknownTopology);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, ("RemoveEndpointFilters: UnregisterSubdevice(topology) failed: 0x%x", ntStatus));
    }

    //
    // All Done.
    //
    ntStatus = STATUS_SUCCESS;
    
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::GetFilters
(
    PENDPOINT_MINIPAIR  MiniportPair,
    PUNKNOWN *          UnknownTopologyPort,
    PUNKNOWN *          UnknownTopologyMiniport,
    PUNKNOWN *          UnknownWavePort,
    PUNKNOWN *          UnknownWaveMiniport
)
/*++

Routine Description:

    Get filter information.

Arguments:

    MiniportPair - miniport toplogy and wave definition

    UnknownTopology - topology interface

    UnknownTopologyMiniport - topology miniport

    UnknownWave - wave interface

    UnknownWaveMiniport - wave miniport

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::GetFilters]"));
    
    NTSTATUS    ntStatus   = STATUS_SUCCESS; 
    PUNKNOWN            unknownTopologyPort     = NULL;
    PUNKNOWN            unknownTopologyMiniport = NULL;
    PUNKNOWN            unknownWavePort         = NULL;
    PUNKNOWN            unknownWaveMiniport     = NULL;

    // if the client requested the topology filter, find it and return it
    if (UnknownTopologyPort != NULL || UnknownTopologyMiniport != NULL)
    {
        ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopologyPort, &unknownTopologyMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownTopologyPort)
            {
                *UnknownTopologyPort = unknownTopologyPort;
            }

            if (UnknownTopologyMiniport)
            {
                *UnknownTopologyMiniport = unknownTopologyMiniport;
            }
        }
    }

    // if the client requested the wave filter, find it and return it
    if (NT_SUCCESS(ntStatus) && (UnknownWavePort != NULL || UnknownWaveMiniport != NULL))
    {
        ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWavePort, &unknownWaveMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownWavePort)
            {
                *UnknownWavePort = unknownWavePort;
            }

            if (UnknownWaveMiniport)
            {
                *UnknownWaveMiniport = unknownWaveMiniport;
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
_Use_decl_annotations_
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::SetIdlePowerManagement
(
  PENDPOINT_MINIPAIR  MiniportPair,
  BOOL                Enabled
)
/*++

Routine Description:

    Set idle power managment.

Arguments:

    MiniportPair - miniport toplogy and wave definition

    Enabled - flag to enable idle power management

Return Value:

    NT status code

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[CAdapterCommon::SetIdlePowerManagement]"));

    NTSTATUS      ntStatus   = STATUS_SUCCESS; 
    IUnknown      *pUnknown = NULL;
    PPORTCLSPOWER pPortClsPower = NULL;
    // refcounting disable requests. Each miniport is responsible for calling this in pairs,
    // disable on the first request to disable, enable on the last request to enable.

    // make sure that we always call SetIdlePowerManagment using the IPortClsPower
    // from the requesting port, so we don't cache a reference to a port
    // indefinitely, preventing it from ever unloading.
    ntStatus = GetFilters(MiniportPair, NULL, NULL, &pUnknown, NULL);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = 
            pUnknown->QueryInterface
            (
                IID_IPortClsPower,
                (PVOID*) &pPortClsPower
            );
    }

    if (NT_SUCCESS(ntStatus))
    {
        if (Enabled)
        {
            m_dwIdleRequests--;

            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, TRUE);
            }
        }
        else
        {
            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, FALSE);
            }

            m_dwIdleRequests++;
        }
    }

    SAFE_RELEASE(pUnknown);
    SAFE_RELEASE(pPortClsPower);

    return ntStatus;
}

_Must_inspect_result_
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
CAdapterCommon::QueryResourceHubBiosDescriptor(
    __in LONGLONG ConnectionId,
    __out __deref __drv_allocatesMem(Mem)
        PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER *BiosDescriptorOut
    )

/*++

Routine Description:

    Stolen from GpiopQueryResourceHubBiosDescriptor.

    This routine queries the BIOS descriptor corresponding to the supplied
    GSIV or (Controller ID, Perhiperal ID, Connection ID) tuple from the
    Resource Hub. If the caller does not specify a resource hub handle, then
    this routine will optionally open one on behalf of the caller.

Arguments:

    ConnectionId - Supplies 64-bit Connection ID value.

    BiosDescriptorOut - Supplies a pointer that receives the buffer containing
        the BIOS descriptor. The caller is responsible for freeing the buffer.

Return Value:

    NTSTATUS code.

--*/

{

    //
    // Assume the device is defined a maximum of 12 levels deep and each level
    // needs 5 characters including the separator ("ABCD.")
    //

#define INITIAL_ACPI_STRING_SIZE (64)

    ULONG_PTR BytesReturned;
    PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER DescriptorBuffer;
    RH_QUERY_CONNECTION_PROPERTIES_INPUT_BUFFER InputBuffer;
    WDF_MEMORY_DESCRIPTOR InputMemoryDescriptor;
    ULONG Length;
    WDF_MEMORY_DESCRIPTOR OutputMemoryDescriptor;
    NTSTATUS Status;

    PAGED_CODE();

    DescriptorBuffer = NULL;

    //
    // Open a handle to the resource hub if it has not been opened already.
    //

    if (m_RhHubIoTarget == NULL) {
        Status = OpenResourceHubTarget();
        if (!NT_SUCCESS(Status)) {
            goto QueryResourceHubBiosDescriptorEnd;
        }
    }

    ASSERT(m_RhHubIoTarget != NULL);

    //
    // Set up a WDF memory descriptor for the input and output parameters.
    //

    RtlZeroMemory(&InputBuffer, sizeof(InputBuffer));
    InputBuffer.Version = RH_QUERY_CONNECTION_PROPERTIES_INPUT_VERSION;
    Length =
        FIELD_OFFSET(RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER,
                          ConnectionProperties) +
        sizeof(PNP_GPIO_INTERRUPT_IO_DESCRIPTOR) +
        INITIAL_ACPI_STRING_SIZE;


        InputBuffer.QueryType = ConnectionIdType;

        NT_ASSERT(sizeof(InputBuffer.u.ConnectionId) == sizeof(ConnectionId));

        RtlCopyMemory(&InputBuffer.u.ConnectionId,
                      &ConnectionId,
                      sizeof(InputBuffer.u.ConnectionId));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&InputMemoryDescriptor,
                                      &InputBuffer,
                                      sizeof(InputBuffer));

    //
    // Query the BIOS descriptor from the Resource hub.
    //

    do {

        DescriptorBuffer = (PRH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER) 
                               ExAllocatePoolWithTag(PagedPool, Length, RHHUB_POOLTAG);

        if (DescriptorBuffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryResourceHubBiosDescriptorEnd;
        }

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&OutputMemoryDescriptor,
                                          DescriptorBuffer,
                                          Length);

        //
        // Send the request to query the size of the BIOS descriptor.
        //

        Status = WdfIoTargetSendIoctlSynchronously(
                     m_RhHubIoTarget,
                     NULL,
                     IOCTL_RH_QUERY_CONNECTION_PROPERTIES,
                     &InputMemoryDescriptor,
                     &OutputMemoryDescriptor,
                     NULL,
                     &BytesReturned);

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            Length = DescriptorBuffer->PropertiesLength +
                     FIELD_OFFSET(RH_QUERY_CONNECTION_PROPERTIES_OUTPUT_BUFFER,
                                  ConnectionProperties);

            ExFreePoolWithTag(DescriptorBuffer, RHHUB_POOLTAG);
            DescriptorBuffer = NULL;
        }

    } while (Status == STATUS_BUFFER_TOO_SMALL);

QueryResourceHubBiosDescriptorEnd:
    if (!NT_SUCCESS(Status)) {
        if (DescriptorBuffer != NULL) {
            ExFreePoolWithTag(DescriptorBuffer, RHHUB_POOLTAG);
        }

    } else {
        *BiosDescriptorOut = DescriptorBuffer;
    }

    return Status;
}


_Must_inspect_result_
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
CAdapterCommon::OpenResourceHubTarget()
/*++

Routine Description:

    Originally stolen from GpiopOpenResourceHubTarget.

    This routine sets up an IO target to the Resource Hub and registers the
    GPIO device with it supplying the BIOS name <--> NT device name mapping.
    This is required for the Resource hub to be able to forward requests to
    this device.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFIOTARGET IoTarget;
    WDF_IO_TARGET_OPEN_PARAMS OpenParameters;
    UNICODE_STRING ResourceHubName;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Create an IO target to the Resource hub.
    //

    IoTarget = NULL;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_WdfDevice;
    Status = WdfIoTargetCreate(m_WdfDevice, &Attributes, &IoTarget);
    if (!NT_SUCCESS(Status)) {
        goto OpenResourceHubTargetEnd;
    }

    RtlInitUnicodeString(&ResourceHubName, RESOURCE_HUB_DEVICE_NAME);
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParameters,
                                                &ResourceHubName,
                                                STANDARD_RIGHTS_ALL);

    //
    // Open the Resource hub IO target.
    //

    Status = WdfIoTargetOpen(IoTarget, &OpenParameters);
    if (!NT_SUCCESS(Status)) {
        goto OpenResourceHubTargetEnd;
    }

    m_RhHubIoTarget = IoTarget;

    //
    // N.B. The IO target does not need to be closed as it would have been set
    //      within the device extension if it was opened successfully.
    //

OpenResourceHubTargetEnd:
    if (!NT_SUCCESS(Status) && (IoTarget != NULL)) {
        WdfObjectDelete(IoTarget);
    }

    return Status;
}

NTSTATUS 
CAdapterCommon::RegisterStream
(
    _In_        CMiniportWaveRTStream* Stream,
                eDeviceType            DeviceType
)
{
    return m_Soc.RegisterStream(Stream, DeviceType);
}

NTSTATUS 
CAdapterCommon::UnregisterStream
(
    _In_        CMiniportWaveRTStream* Stream,
                eDeviceType            DeviceType
)
{
    return m_Soc.UnregisterStream(Stream, DeviceType);
}

NTSTATUS
CAdapterCommon::StartDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    return m_Soc.StartDma(Stream);
}

NTSTATUS
CAdapterCommon::StopDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    return m_Soc.StopDma(Stream);
}

NTSTATUS
CAdapterCommon::PauseDma
(
    _In_        CMiniportWaveRTStream* Stream
)
{
    return m_Soc.PauseDma(Stream);    
}



