// STTubeDevice.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "usb100.h"
#include "STTubeDevice.h"
#include "STTubeDeviceErr30.h"
#include "STTubeDeviceTyp30.h"
#include "STDevicesMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

#if 0
/////////////////////////////////////////////////////////////////////////////
// CSTTubeDeviceApp

BEGIN_MESSAGE_MAP(CSTTubeDeviceApp, CWinApp)
	//{{AFX_MSG_MAP(CSTTubeDeviceApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSTTubeDeviceApp construction

CSTTubeDeviceApp::CSTTubeDeviceApp()
{
	m_pMgr=NULL;
}
 
/////////////////////////////////////////////////////////////////////////////
// The one and only CSTTubeDeviceApp object

CSTTubeDeviceApp theApp;

BOOL CSTTubeDeviceApp::InitInstance() 
{
	m_pMgr=new CSTDevicesManager();
	return CWinApp::InitInstance();
}

int CSTTubeDeviceApp::ExitInstance() 
{
	if (m_pMgr)
		delete m_pMgr;
	m_pMgr=NULL;

	return CWinApp::ExitInstance();
}
#endif

CSTDevicesManager manager;

/////////////////////////////////////////////////////////////////////////////
// Exported functions bodies

extern "C" {

DWORD WINAPI STDevice_Open(LPSTR szDevicePath, 
							LPHANDLE phDevice, 
							LPHANDLE phUnPlugEvent)
{
	CString sSymbName;

	if (!sSymbName)
		return STDEVICE_BADPARAMETER;
	
	sSymbName=szDevicePath;
	if (sSymbName.IsEmpty())
		return STDEVICE_BADPARAMETER;

	return manager.Open(sSymbName, phDevice, phUnPlugEvent);

	return STDEVICE_MEMORY;
}

DWORD WINAPI STDevice_Close(HANDLE hDevice)
{
	return manager.Close(hDevice);
}

DWORD WINAPI STDevice_OpenPipes(HANDLE hDevice)
{
	return manager.OpenPipes(hDevice);
}

DWORD WINAPI STDevice_ClosePipes(HANDLE hDevice)
{
	return manager.ClosePipes(hDevice);
}

DWORD WINAPI STDevice_GetStringDescriptor(HANDLE hDevice, 
								   UINT nIndex, 
								   LPSTR szString, 
								   UINT nStringLength)
{
	CString sString;
	DWORD nRet=STDEVICE_MEMORY;

	// Check parameters we use here. Others are checked in the manager class
	if (!szString)
		return STDEVICE_BADPARAMETER;

	nRet=manager.GetStringDescriptor(hDevice, nIndex, sString);

	if (nRet==STDEVICE_NOERROR)
		strncpy(szString, (LPCSTR)sString, nStringLength);
	return nRet;
}

DWORD WINAPI STDevice_GetDeviceDescriptor(HANDLE hDevice, 
								   PUSB_DEVICE_DESCRIPTOR pDesc)
{
	return manager.GetDeviceDescriptor(hDevice, pDesc);
}

DWORD WINAPI STDevice_GetNbOfConfigurations(HANDLE hDevice, PUINT pNbOfConfigs)
{
	return manager.GetNbOfConfigurations(hDevice, pNbOfConfigs);
}

DWORD WINAPI STDevice_GetConfigurationDescriptor(HANDLE hDevice, 
										  UINT nConfigIdx, 
										  PUSB_CONFIGURATION_DESCRIPTOR pDesc)
{
	return manager.GetConfigurationDescriptor(hDevice, nConfigIdx, pDesc);
}

DWORD WINAPI STDevice_GetNbOfInterfaces(HANDLE hDevice, 
								 UINT nConfigIdx, 
								 PUINT pNbOfInterfaces)
{
	return manager.GetNbOfInterfaces(hDevice, nConfigIdx, pNbOfInterfaces);
}

DWORD WINAPI STDevice_GetNbOfAlternates(HANDLE hDevice, 
								 UINT nConfigIdx, 
								 UINT nInterfaceIdx, 
								 PUINT pNbOfAltSets)
{
	return manager.GetNbOfAlternates(hDevice, nConfigIdx, nInterfaceIdx, pNbOfAltSets);
}

DWORD WINAPI STDevice_GetInterfaceDescriptor(HANDLE hDevice, 
									  UINT nConfigIdx, 
									  UINT nInterfaceIdx, 
									  UINT nAltSetIdx, 
									  PUSB_INTERFACE_DESCRIPTOR pDesc)
{
	return manager.GetInterfaceDescriptor(hDevice, nConfigIdx, nInterfaceIdx, nAltSetIdx, pDesc);
}

DWORD WINAPI STDevice_GetNbOfEndPoints(HANDLE hDevice, 
								UINT nConfigIdx, 
								UINT nInterfaceIdx, 
								UINT nAltSetIdx, 
								PUINT pNbOfEndPoints)
{
	return manager.GetNbOfEndPoints(hDevice, nConfigIdx, nInterfaceIdx, nAltSetIdx, pNbOfEndPoints);
}

DWORD WINAPI STDevice_GetEndPointDescriptor(HANDLE hDevice, 
									 UINT nConfigIdx, 
									 UINT nInterfaceIdx, 
									 UINT nAltSetIdx, 
									 UINT nEndPointIdx, 
									 PUSB_ENDPOINT_DESCRIPTOR pDesc)
{
	return manager.GetEndPointDescriptor(hDevice, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pDesc);
}

DWORD WINAPI STDevice_GetNbOfDescriptors(HANDLE hDevice, 
										BYTE nLevel,
										BYTE nType,
										UINT nConfigIdx, 
										UINT nInterfaceIdx, 
										UINT nAltSetIdx, 
										UINT nEndPointIdx, 
										PUINT pNbOfDescriptors)
{
	return manager.GetNbOfDescriptors(hDevice, nLevel,
									  nType,
									  nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, 
									  pNbOfDescriptors);

	return STDEVICE_MEMORY;
}

DWORD WINAPI STDevice_GetDescriptor(HANDLE hDevice, 
								    BYTE nLevel,
									BYTE nType,
									UINT nConfigIdx, 
									UINT nInterfaceIdx, 
									UINT nAltSetIdx, 
									UINT nEndPointIdx, 
									UINT nIdx,
									PBYTE pDesc,
									UINT nDescSize)
{
	return manager.GetDescriptor(hDevice, nLevel,
								 nType, 
								 nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, nIdx,
								 pDesc, nDescSize);
}

DWORD WINAPI STDevice_SelectCurrentConfiguration(HANDLE hDevice, 
										  UINT nConfigIdx, 
										  UINT nInterfaceIdx, 
										  UINT nAltSetIdx)
{
	return manager.SelectCurrentConfiguration(hDevice, nConfigIdx, nInterfaceIdx, nAltSetIdx);
}

DWORD WINAPI STDevice_SetDefaultTimeOut(HANDLE hDevice, DWORD nTimeOut)
{
	return manager.SetDefaultTimeOut(hDevice, nTimeOut);
}

DWORD WINAPI STDevice_SetMaxNumInterruptInputBuffer(HANDLE hDevice,
													WORD nMaxNumInputBuffer)
{
	return manager.SetMaxNumInterruptInputBuffer(hDevice, nMaxNumInputBuffer);
}

DWORD WINAPI STDevice_GetMaxNumInterruptInputBuffer(HANDLE hDevice,
													PWORD pMaxNumInputBuffer)
{
	return manager.GetMaxNumInterruptInputBuffer(hDevice, pMaxNumInputBuffer);
}

DWORD WINAPI STDevice_SetSuspendModeBehaviour(HANDLE hDevice, BOOL Allow)
{
	return manager.SetSuspendModeBehaviour(hDevice, Allow);
}

DWORD WINAPI STDevice_EndPointControl(HANDLE hDevice, 
							   UINT nEndPointIdx, 
							   UINT nOperation)
{
	return manager.EndPointControl(hDevice, nEndPointIdx, nOperation);
}

DWORD WINAPI STDevice_Reset(HANDLE hDevice)
{
	return manager.Reset(hDevice);
}

DWORD WINAPI STDevice_ControlPipeRequest(HANDLE hDevice, PCNTRPIPE_RQ pRequest,
										 PBYTE pData)
{
	return manager.ControlPipeRequest(hDevice, pRequest, pData);
}

DWORD WINAPI STDevice_Read(HANDLE hDevice, 
				    UINT nEndPointIdx,
					PBYTE pBuffer, 
					PUINT pSize, 
					DWORD nTimeOut)
{
	return manager.Read(hDevice, nEndPointIdx, pBuffer, pSize, nTimeOut);
}

DWORD WINAPI STDevice_Write(HANDLE hDevice, 
				    UINT nEndPointIdx,
					 PBYTE pBuffer, 
					 PUINT pSize, 
					 DWORD nTimeOut)
{
	return manager.Write(hDevice, nEndPointIdx, pBuffer, pSize, nTimeOut);
}

}