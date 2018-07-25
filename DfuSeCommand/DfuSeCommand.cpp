/******************** (C) COPYRIGHT 2018 STMicroelectronics ********************
* Company            : STMicroelectronics
* Author             : MCD Application Team
* Description        : Device Firmware Upgrade Extension Command Line demo
* Version            : V1.4.0
* Date               : 01-June-2018
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************
* FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE
* "SLA0044.txt"
*******************************************************************************/



// DfuSeCommand.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "string.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dos.h>
#include <windows.h>
#include "cxxopts.hpp"
//#include <iostream.h>


static GUID	GUID_DFU = { 0x3fe809ab, 0xfb91, 0x4cb5, { 0xa6, 0x43, 0x69, 0x67, 0x0d, 0x52,0x36,0x6e } };
static GUID GUID_APP = { 0xcb979912, 0x5029, 0x420a, { 0xae, 0xb1, 0x34, 0xfc, 0x0a, 0x7d,0x57,0x26 } };

CTime startTime ;
CTime endTime ;
CTimeSpan elapsedTime;


int				devIndex= 0;		  /* Total Numbers of Active DFU devices*/
int				m_CurrentDevice = 0;  /* Selected DFU device*/

int				m_CurrentTarget = 0;  /* Alternate targets*/


DWORD			m_OperationCode;
CString			m_UpFileName = "";
CString			m_DownFileName= "";

struct DeviceInfo
{
	CString						m_DevPath;		  /* Symbolic Links : Max 128 devices */
	USB_DEVICE_DESCRIPTOR		m_DeviceDesc;
	PMAPPING					m_pMapping = nullptr;
	DFU_FUNCTIONAL_DESCRIPTOR	m_CurrDevDFUDesc;	
	DWORD						m_NbAlternates;       /* Total Numbers of alternates of Active DFU device*/ 
	CString						m_Prod;
	CString						m_Error;
};

DeviceInfo m_DeviceInfos[128];


HANDLE			m_BufferedImage;
HANDLE			hDle;
int             HidDev_Counter;

DFUThreadContext Context;
DWORD dwRet;
int TargetSel=m_CurrentTarget;
HANDLE hImage;


bool Verify = false;
bool Optimize = true;
bool Restart = false;
char *ptr = NULL;
char Drive[3], Dir[256], Fname[256], Ext[256];



/*******************************************************************************************/
/* Function    : FileExist                                                                 */
/* IN          : file name                                                                 */
/* OUT         : boolean                                                                   */
/* Description : verify if the given file exists                                           */
/*******************************************************************************************/
bool FileExist(LPCTSTR filename)
{
	// Data structure for FindFirstFile
	WIN32_FIND_DATA findData;

	// Clear find structure
	ZeroMemory(&findData, sizeof(findData));

	// Search the file
	HANDLE hFind = FindFirstFile( filename, &findData );
	if ( hFind == INVALID_HANDLE_VALUE )
	{
	// File not found
	return false;
	}

	// File found

	// Release find handle
	FindClose( hFind );
	hFind = NULL;

	// The file exists
	return true;
}

/*******************************************************************************************/
/* Function    : void man()                                                                */
/* IN          :                                                                           */
/* OUT         :                                                                           */
/* Description : print the manual on the standard output                                   */
/*******************************************************************************************/
void man()
{
	printf("STMicroelectronics DfuSe command line v1.4.0 \n\n");
	printf(" Usage : \n\n");
	printf(" DfuSeCommand.exe [options] [Agrument][[options] [Agrument]...] \n\n");
	
	printf("  -?                   (Show this help) \n");

	printf("  -c                   (Connect to a DFU device )\n");
	printf("     --de  device      : Number of the device target, by default 0 \n");
	printf("     --al  target      : Number of the alternate target, by default 0 \n");

	printf("  -u                   (Upload flash contents to a .dfu file )\n");
	printf("     --fn  file_name   : full path name of the file \n");

	printf("  -d                   (Download the content of a file into MCU flash) \n");
	printf("     --v               : verify after download \n");
	printf("     --o               : optimize; removes FFs data \n");
	printf("     --fn  file_name   : full path name (.dfu file) \n");

	printf("  -r                   (Reboot after completion) ");
	fflush(NULL);



}



/*******************************************************************************************/
/* Function    : Is_Option                                                                 */
/* IN          : option as string                                                          */
/* OUT         : boolean                                                                   */
/* Description : Verify if the given string present an option                              */
/*******************************************************************************************/
bool Is_Option(char* option)
{
	if (strcmp(option,"-?")==0) return true;
	else if (strcmp(option,"-c")==0) return true;
	else if (strcmp(option,"-u")==0) return true;
	else if (strcmp(option,"-d")==0) return true;
	else if (strcmp(option,"-r") == 0) return true;
	else return false;
}

/*******************************************************************************************/
/* Function    : Is_SubOption                                                              */
/* IN          : sub-option as string                                                      */
/* OUT         : boolean                                                                   */
/* Description : Verify if the given string present a sub-option                           */
/*******************************************************************************************/
bool Is_SubOption(char* suboption)
{
	if (strcmp(suboption,"--fn")==0) return true;
	else if (strcmp(suboption,"--de")==0) return true;
	else if (strcmp(suboption,"--al")==0) return true;
	else if (strcmp(suboption,"--v")==0) return true;
	else if (strcmp(suboption,"--o")==0) return true;

	else return false;
}



/*******************************************************************************************/
/* Function    : HandleError                                                               */
/* IN          : file PDFUThreadContext                                                    */
/* OUT         : None                                                                      */
/* Description : Output Message Errors													   */
/*******************************************************************************************/

void HandleError(PDFUThreadContext pContext)
{
	CString Alternate, Operation, TransferSize, LastDFUStatus, CurrentStateMachineTransition, CurrentRequest, StartAddress, EndAddress, CurrentNBlock, CurrentLength, ErrorCode, Percent;
	CString CurrentTarget;
	CString Tmp;

	CurrentTarget.Format("Target: %02i\n", m_CurrentTarget);
	switch (pContext->Operation)
	{
	case OPERATION_UPLOAD:
		Operation="Operation: UPLOAD\n";
		break;
	case OPERATION_UPGRADE:
		Operation="Operation: UPGRADE\n";
		break;
	case OPERATION_DETACH:
		Operation="Operation: DETACH\n";
		break;
	case OPERATION_RETURN:
		Operation="Operation: RETURN\n";
		break;
	}

	TransferSize.Format("TransferSize: %i\n", pContext->wTransferSize);
	
	switch (pContext->LastDFUStatus.bState)
	{
	case STATE_IDLE:
		LastDFUStatus+="DFU State: STATE_IDLE\n";
		break;
	case STATE_DETACH:
		LastDFUStatus+="DFU State: STATE_DETACH\n";
		break;
	case STATE_DFU_IDLE:
		LastDFUStatus+="DFU State: STATE_DFU_IDLE\n";
		break;
	case STATE_DFU_DOWNLOAD_SYNC:
		LastDFUStatus+="DFU State: STATE_DFU_DOWNLOAD_SYNC\n";
		break;
	case STATE_DFU_DOWNLOAD_BUSY:
		LastDFUStatus+="DFU State: STATE_DFU_DOWNLOAD_BUSY\n";
		break;
	case STATE_DFU_DOWNLOAD_IDLE:
		LastDFUStatus+="DFU State: STATE_DFU_DOWNLOAD_IDLE\n";
		break;
	case STATE_DFU_MANIFEST_SYNC:
		LastDFUStatus+="DFU State: STATE_DFU_MANIFEST_SYNC\n";
		break;
	case STATE_DFU_MANIFEST:
		LastDFUStatus+="DFU State: STATE_DFU_MANIFEST\n";
		break;
	case STATE_DFU_MANIFEST_WAIT_RESET:
		LastDFUStatus+="DFU State: STATE_DFU_MANIFEST_WAIT_RESET\n";
		break;
	case STATE_DFU_UPLOAD_IDLE:
		LastDFUStatus+="DFU State: STATE_DFU_UPLOAD_IDLE\n";
		break;
	case STATE_DFU_ERROR:
		LastDFUStatus+="DFU State: STATE_DFU_ERROR\n";
		break;
	default:
		Tmp.Format("DFU State: (Unknown 0x%02X)\n", pContext->LastDFUStatus.bState);
		LastDFUStatus+=Tmp;
		break;
	}
	switch (pContext->LastDFUStatus.bStatus)
	{
	case STATUS_OK:
		LastDFUStatus+="DFU Status: STATUS_OK\n";
		break;
	case STATUS_errTARGET:
		LastDFUStatus+="DFU Status: STATUS_errTARGET\n";
		break;
	case STATUS_errFILE:
		LastDFUStatus+="DFU Status: STATUS_errFILE\n";
		break;
	case STATUS_errWRITE:
		LastDFUStatus+="DFU Status: STATUS_errWRITE\n";
		break;
	case STATUS_errERASE:
		LastDFUStatus+="DFU Status: STATUS_errERASE\n";
		break;
	case STATUS_errCHECK_ERASE:
		LastDFUStatus+="DFU Status: STATUS_errCHECK_ERASE\n";
		break;
	case STATUS_errPROG:
		LastDFUStatus+="DFU Status: STATUS_errPROG\n";
		break;
	case STATUS_errVERIFY:
		LastDFUStatus+="DFU Status: STATUS_errVERIFY\n";
		break;
	case STATUS_errADDRESS:
		LastDFUStatus+="DFU Status: STATUS_errADDRESS\n";
		break;
	case STATUS_errNOTDONE:
		LastDFUStatus+="DFU Status: STATUS_errNOTDONE\n";
		break;
	case STATUS_errFIRMWARE:
		LastDFUStatus+="DFU Status: STATUS_errFIRMWARE\n";
		break;
	case STATUS_errVENDOR:
		LastDFUStatus+="DFU Status: STATUS_errVENDOR\n";
		break;
	case STATUS_errUSBR:
		LastDFUStatus+="DFU Status: STATUS_errUSBR\n";
		break;
	case STATUS_errPOR:
		LastDFUStatus+="DFU Status: STATUS_errPOR\n";
		break;
	case STATUS_errUNKNOWN:
		LastDFUStatus+="DFU Status: STATUS_errUNKNOWN\n";
		break;
	case STATUS_errSTALLEDPKT:
		LastDFUStatus+="DFU Status: STATUS_errSTALLEDPKT\n";
		break;
	default:
		Tmp.Format("DFU Status: (Unknown 0x%02X)\n", pContext->LastDFUStatus.bStatus);
		LastDFUStatus+=Tmp;
		break;
	}
	
	switch (pContext->CurrentRequest)
	{
	case STDFU_RQ_GET_DEVICE_DESCRIPTOR: 
		CurrentRequest+="Request: Getting Device Descriptor. \n";
		break;
	case STDFU_RQ_GET_DFU_DESCRIPTOR:
		CurrentRequest+="Request: Getting DFU Descriptor. \n";
		break;
	case STDFU_RQ_GET_STRING_DESCRIPTOR:
		CurrentRequest+="Request: Getting String Descriptor. \n";
		break;
	case STDFU_RQ_GET_NB_OF_CONFIGURATIONS:
		CurrentRequest+="Request: Getting amount of configurations. \n";
		break;
	case STDFU_RQ_GET_CONFIGURATION_DESCRIPTOR:
		CurrentRequest+="Request: Getting Configuration Descriptor. \n";
		break;
	case STDFU_RQ_GET_NB_OF_INTERFACES:
		CurrentRequest+="Request: Getting amount of interfaces. \n";
		break;
	case STDFU_RQ_GET_NB_OF_ALTERNATES:
		CurrentRequest+="Request: Getting amount of alternates. \n";
		break;
	case STDFU_RQ_GET_INTERFACE_DESCRIPTOR:
		CurrentRequest+="Request: Getting interface Descriptor. \n";
		break;
	case STDFU_RQ_OPEN:
		CurrentRequest+="Request: Opening device. \n";
		break;
	case STDFU_RQ_CLOSE:
		CurrentRequest+="Request: Closing device. \n";
		break;
	case STDFU_RQ_DETACH:
		CurrentRequest+="Request: Detach Request. \n";
		break;
	case STDFU_RQ_DOWNLOAD:
		CurrentRequest+="Request: Download Request. \n";
		break;
	case STDFU_RQ_UPLOAD:
		CurrentRequest+="Request: Upload Request. \n";
		break;
	case STDFU_RQ_GET_STATUS:
		CurrentRequest+="Request: Get Status Request. \n";
		break;
	case STDFU_RQ_CLR_STATUS:
		CurrentRequest+="Request: Clear Status Request. \n";
		break;
	case STDFU_RQ_GET_STATE:
		CurrentRequest+="Request: Get State Request. \n";
		break;
	case STDFU_RQ_ABORT:
		CurrentRequest+="Request: Abort Request. \n";
		break;
	case STDFU_RQ_SELECT_ALTERNATE:
		CurrentRequest+="Request: Selecting target. \n";
		break;
	case STDFU_RQ_AWAITINGPNPUNPLUGEVENT:
		CurrentRequest+="Request: Awaiting UNPLUG EVENT. \n";
		break;
	case STDFU_RQ_AWAITINGPNPPLUGEVENT:
		CurrentRequest+="Request: Awaiting PLUG EVENT. \n";
		break;
	case STDFU_RQ_IDENTIFYINGDEVICE:
		CurrentRequest+="Request: Identifying device after reenumeration. \n";
		break;
	default:
		Tmp.Format("Request: (unknown 0x%08X). \n", pContext->CurrentRequest);
		CurrentRequest+=Tmp;
		break;
	}
	
	CurrentNBlock.Format("CurrentNBlock: 0x%04X\n", pContext->CurrentNBlock);
	CurrentLength.Format("CurrentLength: 0x%04X\n", pContext->CurrentLength);
	Percent.Format("Percent: %i%%\n", pContext->Percent);
	
	switch (pContext->ErrorCode)
	{
	case STDFUPRT_NOERROR:
		ErrorCode="Error Code: no error (!)\n";
		break;
	case STDFUPRT_UNABLETOLAUNCHDFUTHREAD:
		ErrorCode="Error Code: Unable to launch operation (Thread problem)\n";
		break;
	case STDFUPRT_DFUALREADYRUNNING:
		ErrorCode="Error Code: DFU already running\n";
		break;
	case STDFUPRT_BADPARAMETER:
		ErrorCode="Error Code: Bad parameter\n";
		break;
	case STDFUPRT_BADFIRMWARESTATEMACHINE:
		ErrorCode="Error Code: Bad state machine in firmware\n";
		break;
	case STDFUPRT_UNEXPECTEDERROR:
		ErrorCode="Error Code: Unexpected error\n";
		break;
	case STDFUPRT_DFUERROR:
		ErrorCode="Error Code: DFU error\n";
		break;
	case STDFUPRT_RETRYERROR:
		ErrorCode="Error Code: Retry error\n";
		break;
	default:
		Tmp.Format("Error Code: Unknown error 0x%08X. \n", pContext->ErrorCode);
		ErrorCode=Tmp;
	}


	if (m_CurrentTarget>=0)
	{
		Tmp.Format("\nTarget %02i: %s", m_CurrentTarget, ErrorCode);
		//m_Progress.SetWindowText(Tmp);
		printf(Tmp);
		fflush(NULL);
	}
	else
	//m_Progress.SetWindowText(ErrorCode);
	{ printf("\n" + ErrorCode);
	  fflush(NULL);
	  }

	//AfxMessageBox(CurrentTarget+ErrorCode+Alternate+Operation+TransferSize+LastDFUStatus+CurrentStateMachineTransition+CurrentRequest+StartAddress+EndAddress+CurrentNBlock+CurrentLength+Percent);
	printf(CurrentTarget+ErrorCode+Alternate+Operation+TransferSize+LastDFUStatus+CurrentStateMachineTransition+CurrentRequest+StartAddress+EndAddress+CurrentNBlock+CurrentLength+Percent);
	fflush(NULL);


}

/*******************************************************************************************/
/* Function    : HandleTxtError                                                            */
/* IN          : LPCSTR                                                                    */
/* OUT         : None                                                                      */
/* Description : Output Message Errors													   */
/*******************************************************************************************/

void HandleTxtError(LPCSTR szTxt)
{
	CString Tmp;

	if (m_CurrentTarget>=0)
	{
		Tmp.Format("Target %02i: %s", m_CurrentTarget, szTxt);
		printf(Tmp);
		fflush(NULL);
	}
	else
	{	printf(szTxt);
		fflush(NULL);
	}
}

/*******************************************************************************************/
/* Function    : HandleTxtSuccess                                                          */
/* IN          : LPCSTR                                                                    */
/* OUT         : None                                                                      */
/* Description : Output Message 													       */
/*******************************************************************************************/
void HandleTxtSuccess(LPCSTR szTxt)
{
	CString Tmp;

	if (m_CurrentTarget>=0)
	{
		Tmp.Format("Target %02i: %s", m_CurrentTarget, szTxt);
		printf(Tmp);
	}
	else
		printf(szTxt);
		
	fflush(NULL);
}


/*******************************************************************************************/
/* Function    : LaunchUpload															   */
/* IN          : None																	   */
/* OUT         : Status                                                                    */
/* Description : Prepare to launch the Upload Thread				                       */
/*******************************************************************************************/

int LaunchUpload(void) 
{
	DFUThreadContext Context;
	DWORD dwRet;
	int TargetSel=m_CurrentTarget;
	HANDLE hImage;

	// prepare the asynchronous operation
	lstrcpy(Context.szDevLink, m_DeviceInfos[m_CurrentDevice].m_DevPath);
	Context.DfuGUID=GUID_DFU;

	Context.Operation=OPERATION_UPLOAD;
	if (m_BufferedImage)
		STDFUFILES_DestroyImage(&m_BufferedImage);
	m_BufferedImage=0;
	
	CString Name; 

	PMAPPING pMapping = m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel;

	Name= pMapping->Name;
	STDFUFILES_CreateImageFromMapping(&hImage, pMapping);
	STDFUFILES_SetImageName(hImage, (LPSTR)(LPCSTR)Name);
	STDFUFILES_FilterImageForOperation(hImage, pMapping, OPERATION_UPLOAD, FALSE);
	Context.hImage=hImage;

	startTime = CTime::GetCurrentTime();


	//printf("0 KB(0 Bytes) of %i KB(%i Bytes) \n", STDFUFILES_GetImageSize(Context.hImage)/1024,  STDFUFILES_GetImageSize(Context.hImage));


	dwRet=STDFUPRT_LaunchOperation(&Context, &m_OperationCode);
	if (dwRet!=STDFUPRT_NOERROR)
	{
		Context.ErrorCode=dwRet;
		HandleError(&Context);
		return 1;
	}
	else
	{
		return 0;
	}

}


/*******************************************************************************************/
/* Function    : LaunchUpgrade															   */
/* IN          : None																	   */
/* OUT         : Status                                                                    */
/* Description : Prepare to launch the Upgrade Thread				                       */
/*******************************************************************************************/


int LaunchUpgrade(void) 
{
	DFUThreadContext Context;
	HANDLE hFile;
	BYTE NbTargets;
	BOOL bFound=FALSE;
	DWORD dwRet;
	int i, TargetSel=m_CurrentTarget;
	HANDLE hImage;
	

	// Get the image of the selected target
	dwRet=STDFUFILES_OpenExistingDFUFile((LPSTR)(LPCSTR)m_DownFileName, &hFile, NULL, NULL, NULL, &NbTargets);
	if (dwRet==STDFUFILES_NOERROR)
	{
		for (i=0;i<NbTargets;i++)
		{
			HANDLE Image;
			BYTE Alt;

			if (STDFUFILES_ReadImageFromDFUFile(hFile, i, &Image)==STDFUFILES_NOERROR)
			{
				if (STDFUFILES_GetImageAlternate(Image, &Alt)==STDFUFILES_NOERROR)
				{
					if (Alt==TargetSel)
					{
						hImage=Image;
						bFound=TRUE;
						break;
					}
				}
				STDFUFILES_DestroyImage(&Image);
			}
		}
		STDFUFILES_CloseDFUFile(hFile);
	}
	else
	{
		Context.ErrorCode=dwRet;
		HandleError(&Context);
	}

	if (!bFound)
	{
		HandleTxtError("Unable to find data for that device/target from the file ...");
		return 1;
	}
	else
	{
		// prepare the asynchronous operation: first is erase !
		lstrcpy(Context.szDevLink, m_DeviceInfos[m_CurrentDevice].m_DevPath);
		Context.DfuGUID=GUID_DFU;
		Context.Operation=OPERATION_ERASE;
		Context.bDontSendFFTransfersForUpgrade=Optimize;
		if (m_BufferedImage)
			STDFUFILES_DestroyImage(&m_BufferedImage);
		// Let's backup our data before the filtering for erase. The data will be used for the upgrade phase
		STDFUFILES_DuplicateImage(hImage, &m_BufferedImage);
		STDFUFILES_FilterImageForOperation(m_BufferedImage, m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel, OPERATION_UPGRADE, Optimize);

		STDFUFILES_FilterImageForOperation(hImage, m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel, OPERATION_ERASE, Optimize);
		Context.hImage=hImage;

		//printf("0 KB(0 Bytes) of %i KB(%i Bytes) \n", STDFUFILES_GetImageSize(m_BufferedImage)/1024,  STDFUFILES_GetImageSize(m_BufferedImage));
	

		startTime = CTime::GetCurrentTime();

		dwRet=STDFUPRT_LaunchOperation(&Context, &m_OperationCode);
		if (dwRet!=STDFUPRT_NOERROR)
		{
			Context.ErrorCode=dwRet;
			HandleError(&Context);
			return 1;
		}
		else
		{

			return 0;
		}
	}
}

/*******************************************************************************************/
/* Function    : LaunchVerify															   */
/* IN          : None																	   */
/* OUT         : Status                                                                    */
/* Description : Prepare to launch the Verification Thread after an Upgrade			       */
/*******************************************************************************************/

int LaunchVerify(void) 
{
	DFUThreadContext Context;
	BOOL bFound=FALSE;
	DWORD dwRet;
	int i, TargetSel=m_CurrentTarget;
	HANDLE hImage;
	HANDLE hFile;
	BYTE NbTargets;

	// Get the image of the selected target
	dwRet=STDFUFILES_OpenExistingDFUFile((LPSTR)(LPCSTR)m_DownFileName, &hFile, NULL, NULL, NULL, &NbTargets);
	if (dwRet==STDFUFILES_NOERROR)
	{
		for (i=0;i<NbTargets;i++)
		{
			HANDLE Image;
			BYTE Alt;

			if (STDFUFILES_ReadImageFromDFUFile(hFile, i, &Image)==STDFUFILES_NOERROR)
			{
				if (STDFUFILES_GetImageAlternate(Image, &Alt)==STDFUFILES_NOERROR)
				{
					if (Alt==TargetSel)
					{
						hImage=Image;
						bFound=TRUE;
						break;
					}
				}
				STDFUFILES_DestroyImage(&Image);
			}
		}
		STDFUFILES_CloseDFUFile(hFile);
	}
	else
	{
		Context.ErrorCode=dwRet;
		HandleError(&Context);
	}

	if (!bFound)
	{
		HandleTxtError("Unable to find data for that target in the dfu file...");
		return 1;
	}
	else
	{
		// prepare the asynchronous operation
		lstrcpy(Context.szDevLink, m_DeviceInfos[m_CurrentDevice].m_DevPath);
		Context.DfuGUID=GUID_DFU;
		Context.Operation=OPERATION_UPLOAD;
		if (m_BufferedImage)
			STDFUFILES_DestroyImage(&m_BufferedImage);
		STDFUFILES_FilterImageForOperation(hImage, m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel, OPERATION_UPLOAD, FALSE);
		Context.hImage=hImage;
		// Let's backup our data before the upload. The data will be used after the upload for comparison
		STDFUFILES_DuplicateImage(hImage, &m_BufferedImage);

		startTime = CTime::GetCurrentTime();

		dwRet=STDFUPRT_LaunchOperation(&Context, &m_OperationCode);
		if (dwRet!=STDFUPRT_NOERROR)
		{
			Context.ErrorCode=dwRet;
			HandleError(&Context);
			return 1;
		}
		else
		{

			return 0;
		}
	}
}

void LaunchReboot()
{
	DFUThreadContext Context;
	CString Tempo;
	DWORD dwRet;
	HANDLE hImage;

	CString Name;

	// Prepare the asynchronous operation
	lstrcpy(Context.szDevLink, m_DeviceInfos[m_CurrentDevice].m_DevPath);

	Context.DfuGUID=GUID_DFU;
	Context.AppGUID=GUID_APP;
	Context.Operation=OPERATION_RETURN;
	Name = m_DeviceInfos[m_CurrentDevice].m_pMapping[TargetSel].Name;
	STDFUFILES_CreateImageFromMapping(&hImage, m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel);
	STDFUFILES_SetImageName(hImage, (LPSTR)(LPCSTR)Name);
	STDFUFILES_FilterImageForOperation(hImage, m_DeviceInfos[m_CurrentDevice].m_pMapping + TargetSel, OPERATION_RETURN, FALSE);
	Context.hImage=hImage;

	startTime = CTime::GetCurrentTime();

	dwRet=STDFUPRT_LaunchOperation(&Context, &m_OperationCode);
	if (dwRet!=STDFUPRT_NOERROR)
	{
		Context.ErrorCode=dwRet;
		HandleError(&Context);
	}
}



/*******************************************************************************************/
/* Function    : Refresh																   */
/* IN          : None																	   */
/* OUT         : Status                                                                    */
/* Description : Make a refresh on Plugged devices and Enumerated Them		               */
/*******************************************************************************************/


void Refresh()
{
 
	int i;
	char	Product[253];
	CString	String;

	HDEVINFO info;

	BOOL bSuccess=FALSE;


	// Begin with HID devices  // Commented in Version V3.0.3 and Keep it for customer usage if wanted.
	/*ReleaseHIDMemory();
	m_Enum.FindHidDevice(&m_HidDevices,&m_HidDevice_Counter);
	FindMyHIDDevice();
	for(i=0;i<int(m_HidDevice_Counter);i++)
	{
		PHID_DEVICE pWalk;

		pWalk = m_HidDevices + m_Tab_Index[i];			
		if(HidD_GetProductString(pWalk->HidDevice, Product, sizeof(Product)))
		{
			for(j=0;j<160;j+=2)
				Prod += Product[j];
		}
		else 
			Prod="(Unknown HID Device)";

		String.Format("%s",Prod);
		m_CtrlDFUDevices.AddString(String);
		m_CtrlDevices.AddString(String);
		Prod = "";
	}*/  // Commented in Version V3.0.3 and Keep it for customer usage if wanted.


	// Continue with DFU devices. DFU devices will be listed after HID ones

	GUID Guid=GUID_DFU;
	devIndex=0;


	info=SetupDiGetClassDevs(&Guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if (info!=INVALID_HANDLE_VALUE)  
	{

		SP_INTERFACE_DEVICE_DATA ifData;
		ifData.cbSize=sizeof(ifData);

		for (devIndex=0;SetupDiEnumDeviceInterfaces(info, NULL, &Guid, devIndex, &ifData);++devIndex)
		{
			DWORD needed;

			SetupDiGetDeviceInterfaceDetail(info, &ifData, NULL, 0, &needed, NULL);

			PSP_INTERFACE_DEVICE_DETAIL_DATA detail=(PSP_INTERFACE_DEVICE_DETAIL_DATA)new BYTE[needed];
			detail->cbSize=sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
			SP_DEVINFO_DATA did={sizeof(SP_DEVINFO_DATA)};
		
			if (SetupDiGetDeviceInterfaceDetail(info, &ifData, detail, needed, NULL, &did))
			{
				// Add the link to the list of all DFU devices
				m_DeviceInfos[devIndex].m_DevPath=detail->DevicePath;
			}
			else
			{
				m_DeviceInfos[devIndex].m_DevPath = "";   //m_CtrlDFUDevices.AddString("");
			}

			if (SetupDiGetDeviceRegistryProperty(info, &did, SPDRP_DEVICEDESC, NULL, (PBYTE)Product, 253, NULL))
			{ 
				m_DeviceInfos[devIndex].m_Prod = Product;
			}
			else
			{
				m_DeviceInfos[devIndex].m_Prod = "(Unnamed DFU device)";
			}
			// Add the name of the device
			//m_CtrlDevices.AddString(Prod);
			delete[] (PBYTE)detail;

			DeviceInfo& devInfo = m_DeviceInfos[devIndex];


			if (STDFU_Open((LPSTR)(LPCSTR)devInfo.m_DevPath, &hDle) == STDFU_NOERROR)   //  !!!! To be changed
			{
				if (STDFU_GetDeviceDescriptor(&hDle, &devInfo.m_DeviceDesc) == STDFU_NOERROR)
				{
					UINT Dummy1, Dummy2;
					// Get its attributes
					memset(&devInfo.m_CurrDevDFUDesc, 0, sizeof(devInfo.m_CurrDevDFUDesc));
					if (STDFU_GetDFUDescriptor(&hDle, &Dummy1, &Dummy2, &devInfo.m_CurrDevDFUDesc) == STDFUPRT_NOERROR)
					{
						if ((devInfo.m_CurrDevDFUDesc.bcdDFUVersion < 0x011A) || (devInfo.m_CurrDevDFUDesc.bcdDFUVersion >= 0x0120))
						{
							if (devInfo.m_CurrDevDFUDesc.bcdDFUVersion != 0)
							{
								m_DeviceInfos[devIndex].m_Error = "Bad DFU protocol version. Should be 1.1A";
							}
						}
						else
						{
							PUSB_DEVICE_DESCRIPTOR Desc = NULL;

							// Tries to get the mapping
							if (STDFUPRT_CreateMappingFromDevice((LPSTR)(LPCSTR)devInfo.m_DevPath, &devInfo.m_pMapping, &devInfo.m_NbAlternates) == STDFUPRT_NOERROR)
							{
								bSuccess = TRUE;
							}
							else
							{
								m_DeviceInfos[devIndex].m_Error = "Unable to find or decode device mapping... Bad Firmware";
							}
						}
					}
					else
					{
						m_DeviceInfos[devIndex].m_Error = "Unable to get DFU descriptor... Bad Firmware";
					}
				}
				else
				{
					m_DeviceInfos[devIndex].m_Error = "Unable to get descriptors... Bad Firmware";
				}
			}
			STDFU_Close(&hDle);
		}
		SetupDiDestroyDeviceInfoList(info);
	}

	if ( devIndex > 0 )
	{
		printf("%d Device(s) found : \n",devIndex );
		fflush(NULL);
		for ( i=0; i< devIndex  ; i++ )
		{
			if (m_DeviceInfos[i].m_Error == "")
			{
				printf("Device [%d]: %s, having [%d] alternate targets \n", i, (LPCSTR)m_DeviceInfos[i].m_Prod, m_DeviceInfos[i].m_NbAlternates);
			}
			else
			{
				printf("Device [%d]: %s\n", i, (LPCSTR)m_DeviceInfos[i].m_Error);
			}
			printf(" Device Path:\n");
			printf(" %s\n", (LPCSTR)m_DeviceInfos[i].m_DevPath);
		}
		printf("\n\n");
		fflush(NULL);
	}
	
	/* Device ID and Unique ID may be added by  user */
	
	/*
	
	LPBYTE m_pBuffer = (LPBYTE)malloc(64);
	memset(m_pBuffer, 0x00, 64);

	{
		DFUSTATUS DFUStatus;

		STDFU_Getstatus(&hDle, &DFUStatus);
		while (DFUStatus.bState != STATE_DFU_IDLE)
		{
			STDFU_Clrstatus(&hDle);
			STDFU_Getstatus(&hDle, &DFUStatus);
		}

		m_pBuffer[0] = 0x21;
		m_pBuffer[1] = 0x00;
		m_pBuffer[2] = 0x20;
		m_pBuffer[3] = 0x04;
		m_pBuffer[4] = 0xE0;

		STDFU_Dnload(&hDle, m_pBuffer, 0x05, 0);

		STDFU_Getstatus(&hDle, &DFUStatus);
		while (DFUStatus.bState != STATE_DFU_IDLE)
		{
			STDFU_Clrstatus(&hDle);
			STDFU_Getstatus(&hDle, &DFUStatus);
		}

		if (STDFU_Upload(&hDle, m_pBuffer, 4, 2) == STDFU_NOERROR)
		{	
			printf("Device DID :");
			fflush(NULL);
			for (i = 0; i < 4; i++)
			{
				printf("0x%02x ", m_pBuffer[i]);
				fflush(NULL);
			}
			printf("\n");
			fflush(NULL);

		}
	}

	memset(m_pBuffer, 0x00, 64);

	{
		DFUSTATUS DFUStatus;

		STDFU_Getstatus(&hDle, &DFUStatus);
		while (DFUStatus.bState != STATE_DFU_IDLE)
		{
			STDFU_Clrstatus(&hDle);
			STDFU_Getstatus(&hDle, &DFUStatus);
		}

		m_pBuffer[0] = 0x21;
		m_pBuffer[1] = 0x10;
		m_pBuffer[2] = 0x7A;
		m_pBuffer[3] = 0xFF;
		m_pBuffer[4] = 0x1F;

		STDFU_Dnload(&hDle, m_pBuffer, 0x05, 0);

		STDFU_Getstatus(&hDle, &DFUStatus);
		while (DFUStatus.bState != STATE_DFU_IDLE)
		{
			STDFU_Clrstatus(&hDle);
			STDFU_Getstatus(&hDle, &DFUStatus);
		}

		if (STDFU_Upload(&hDle, m_pBuffer, 12, 2) == STDFU_NOERROR)
		{	
			printf("Device UID :");
			fflush(NULL);
			for (i = 0; i < 12; i++)
			{
				printf("0x%02x ", m_pBuffer[i]);
				fflush(NULL);
			}
			printf("\n");
			fflush(NULL);

		}
	}

	*/
}



/*******************************************************************************************/
/* Function    : TimerProc																   */
/* IN          : Timer Inputs (HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime) 		   */
/* OUT         : None                                                                      */
/* Description : Main Function that refreshs the timer during the Operation	of Threads     */
/*******************************************************************************************/

void CALLBACK TimerProc(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime) 
{
	DFUThreadContext Context;
	CString Tmp;
	DWORD dwRet;

	if (m_OperationCode)  
	{
		endTime = CTime::GetCurrentTime();
		elapsedTime = endTime - startTime;
	
		printf( " Duration: %.2i:%.2i:%.2i", elapsedTime.GetHours(),elapsedTime.GetMinutes(),elapsedTime.GetSeconds());
		fflush(NULL);

		CString Tmp, Tmp2;

		// Get the operation status
		STDFUPRT_GetOperationStatus(m_OperationCode, &Context);
		
		if (Context.ErrorCode!=STDFUPRT_NOERROR)
		{
			STDFUPRT_StopOperation(m_OperationCode, &Context);
			if (Context.Operation==OPERATION_UPGRADE)
				m_BufferedImage=0;
			if (Context.Operation==OPERATION_UPLOAD)
			{
				if (m_BufferedImage!=0) // Verify
				{
					STDFUFILES_DestroyImage(&m_BufferedImage);
					m_BufferedImage=0;
				}
			}
			STDFUFILES_DestroyImage(&Context.hImage);

			m_OperationCode=0;
			if (Context.ErrorCode==STDFUPRT_UNSUPPORTEDFEATURE)
			{
				printf("This feature is not supported by this firmware.");
				fflush(NULL);
			}
			else
			{ 
				HandleError(&Context);
				KillTimer(hWnd, nIDEvent);
				PostQuitMessage (1) ;
			}
		}
		else
		{
			switch (Context.Operation)
			{
			case OPERATION_UPLOAD:
				{
				  Tmp.Format("\rTarget %02i: Uploading (%i%%)...", m_CurrentTarget, Context.Percent);
				  printf(Tmp);
				  fflush(NULL);
				  //printf("%i KB(%i Bytes) of %i KB(%i Bytes) \n", ((STDFUFILES_GetImageSize(Context.hImage)/1024)*Context.Percent)/100,  (STDFUFILES_GetImageSize(Context.hImage)*Context.Percent)/100, STDFUFILES_GetImageSize(Context.hImage)/1024,  STDFUFILES_GetImageSize(Context.hImage));
				 // m_DataSize.Format("%i KB(%i Bytes) of %i KB(%i Bytes)", ((STDFUFILES_GetImageSize(Context.hImage)/1024)*Context.Percent)/100,  (STDFUFILES_GetImageSize(Context.hImage)*Context.Percent)/100, STDFUFILES_GetImageSize(Context.hImage)/1024,  STDFUFILES_GetImageSize(Context.hImage));
				  break;
				}
			case OPERATION_UPGRADE:
				{
				  Tmp.Format("\rTarget %02i: Upgrading - Download Phase (%i%%)...", m_CurrentTarget, Context.Percent);
				  printf(Tmp);
				  fflush(NULL);
				  //printf("%i KB(%i Bytes) of %i KB(%i Bytes) \n", ((STDFUFILES_GetImageSize(Context.hImage)/1024)*Context.Percent)/100,  (STDFUFILES_GetImageSize(Context.hImage)*Context.Percent)/100, STDFUFILES_GetImageSize(Context.hImage)/1024,  STDFUFILES_GetImageSize(Context.hImage));
				  //m_DataSize.Format("%i KB(%i Bytes) of %i KB(%i Bytes)", ((STDFUFILES_GetImageSize(Context.hImage)/1024)*Context.Percent)/100,  (STDFUFILES_GetImageSize(Context.hImage)*Context.Percent)/100, STDFUFILES_GetImageSize(Context.hImage)/1024,  STDFUFILES_GetImageSize(Context.hImage));
				  break;
				}
			case OPERATION_DETACH:
				Tmp.Format("\rDetaching (%i%%)...", Context.Percent);
				 printf(Tmp);
				 fflush(NULL);
				break;
			case OPERATION_RETURN:
				Tmp.Format("\rLeaving DFU mode (%i%%)...", Context.Percent);
				 printf(Tmp);
				 fflush(NULL);
				break;
			default:
				Tmp.Format("\rTarget %02i: Upgrading - Erase Phase (%i%%)...", m_CurrentTarget, Context.Percent);
				printf(Tmp);
				break;
			}

			if (Context.Percent==100)
			{
				if (Context.Operation==OPERATION_ERASE)
				{
					// After the erase, relaunch the Upgrade phase !
					STDFUPRT_StopOperation(m_OperationCode, &Context);
					STDFUFILES_DestroyImage(&Context.hImage);

					Context.Operation=OPERATION_UPGRADE;
					Context.hImage=m_BufferedImage;
					dwRet=STDFUPRT_LaunchOperation(&Context, &m_OperationCode);
					if (dwRet!=STDFUPRT_NOERROR)
					{
						Context.ErrorCode=dwRet;
						HandleError(&Context);
						KillTimer(hWnd, nIDEvent);
						PostQuitMessage (0) ;
					}
				}
				else
				{
					STDFUPRT_StopOperation(m_OperationCode, &Context);
					m_OperationCode=0;
					if (Context.Operation==OPERATION_RETURN)
					{
						HandleTxtSuccess("\nSuccessfully left DFU mode !\n");
						KillTimer(hWnd, nIDEvent);
						PostQuitMessage (0) ;
					}
					if (Context.Operation==OPERATION_DETACH) 
					{
						HandleTxtSuccess("\nSuccessfully detached !\n");
						KillTimer(hWnd, nIDEvent);
						PostQuitMessage (0) ;
					}
					if (Context.Operation==OPERATION_UPLOAD)
					{
						if (m_BufferedImage==0) // This was a standard Upload
						{
							HANDLE hFile;
							PUSB_DEVICE_DESCRIPTOR Desc=NULL;

							WORD Vid, Pid, Bcd;
							auto& deviceDesk = m_DeviceInfos[m_CurrentDevice].m_DeviceDesc;

							Vid=deviceDesk.idVendor;
							Pid=deviceDesk.idProduct;
							Bcd=deviceDesk.bcdDevice;

							dwRet=STDFUFILES_CreateNewDFUFile((LPSTR)(LPCSTR)m_UpFileName, &hFile, Vid, Pid, Bcd);

							if (dwRet==STDFUFILES_NOERROR)
							{
								dwRet=STDFUFILES_AppendImageToDFUFile(hFile, Context.hImage);
								if (dwRet==STDFUFILES_NOERROR)
								{
									printf("Upload successful !\n");
									if (Restart)
									{
										LaunchReboot();
									}
									else
									{ 
										fflush(NULL);
										KillTimer(hWnd, nIDEvent);
										PostQuitMessage (0) ;
									}
								}
								else
								{
									printf("Unable to append image to DFU file...");
									fflush(NULL);
									PostQuitMessage(1);
								}
								STDFUFILES_CloseDFUFile(hFile);
							}
							else
							{
								printf("Unable to create a new DFU file...");
								fflush(NULL);
								PostQuitMessage(1);
							}
						}
						else // This was a verify
						{
							// We need to compare our Two images
							DWORD i,j;
							DWORD MaxElements;
							BOOL bDifferent=FALSE, bSuccess=TRUE;

							dwRet=STDFUFILES_GetImageNbElement(Context.hImage, &MaxElements);
							if (dwRet==STDFUFILES_NOERROR)
							{
								for (i=0;i<MaxElements;i++)
								{
									DFUIMAGEELEMENT ElementRead, ElementSource;

									memset(&ElementRead, 0, sizeof(DFUIMAGEELEMENT));
									memset(&ElementSource, 0, sizeof(DFUIMAGEELEMENT));

									// Get the Two elements
									dwRet=STDFUFILES_GetImageElement(Context.hImage, i, &ElementRead);
									if (dwRet==STDFUFILES_NOERROR)
									{
										ElementRead.Data=new BYTE[ElementRead.dwDataLength];
										dwRet=STDFUFILES_GetImageElement(Context.hImage, i, &ElementRead);
										if (dwRet==STDFUFILES_NOERROR)
										{
											ElementSource.Data=new BYTE[ElementRead.dwDataLength]; // Should be same lengh in source and read
											dwRet=STDFUFILES_GetImageElement(m_BufferedImage, i, &ElementSource);
											if (dwRet==STDFUFILES_NOERROR)
											{
												for (j=0;j<ElementRead.dwDataLength;j++)
												{
													if (ElementSource.Data[j]!=ElementRead.Data[j])
													{
														bDifferent=TRUE;
														HandleTxtError("Verify successful, but data not matching...");
														Tmp.Format("Matching not good. First Difference at address 0x%08X:\nFile  byte  is  0x%02X.\nRead byte is 0x%02X.", ElementSource.dwAddress+j, ElementSource.Data[j], ElementRead.Data[j]);
														printf(Tmp);
														fflush(NULL);
														break;
													}
												}
											}
											else
											{
												HandleTxtError("Unable to get data from source image...");
												bSuccess=FALSE;
												break;
											}
											delete[] ElementSource.Data;
											delete[] ElementRead.Data;
										}
										else
										{
											delete[] ElementRead.Data;
											HandleTxtError("Unable to get data from read image...");
											bSuccess=FALSE;
											break;
										}
									}
									else
									{
										HandleTxtError("Unable to get data from read image...");
										bSuccess=FALSE;
										break;
									}
									if (bDifferent)
										break;
								}
							}
							else
							{
								HandleTxtError("Unable to get elements from read image...");
								bSuccess=FALSE;
							}

							STDFUFILES_DestroyImage(&m_BufferedImage);
							m_BufferedImage=0;
							if (bSuccess)
							{
								if (!bDifferent)
								{
									printf("\nVerify successful !\n");
									if (Restart)
									{
										LaunchReboot();
									}
									else
									{
										fflush(NULL);
										KillTimer(hWnd, nIDEvent);
										PostQuitMessage(0);
									}
								}
								else
								{
									KillTimer(hWnd, nIDEvent);
									PostQuitMessage(1);
								}
							}
							else
							{
								KillTimer(hWnd, nIDEvent);
								PostQuitMessage(1);
							}
						}
					} 
					if (Context.Operation==OPERATION_UPGRADE)
					{
						printf("\nUpgrade successful !\n");
						fflush(NULL);
						m_BufferedImage=0;
						if (Verify)
						{
							LaunchVerify();
						}
						else if (Restart)
						{
							LaunchReboot();
						}
						else
						{ 
							KillTimer(hWnd, nIDEvent);
							PostQuitMessage (0) ;
						}
					}
				}
			}
		}
	}
}

/*******************************************************************************************/
/* Function    : OnCancel																   */
/* IN          : None		                                                               */
/* OUT         : None                                                                      */
/* Description : Cancel Operation and free memory buffers                                  */
/*******************************************************************************************/

void OnCancel() 
{
	BOOL bStop=TRUE;
	DFUThreadContext Context;

	if (m_OperationCode)
	{
		bStop=FALSE;
		if (AfxMessageBox("Operation on-going. Leave anyway ?", MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
			bStop=TRUE;
	}

	if (bStop)
	{
		if (m_OperationCode)
			STDFUPRT_StopOperation(m_OperationCode, &Context);

		STDFUFILES_DestroyImage(&Context.hImage);
		if (m_BufferedImage)
			STDFUFILES_DestroyImage(&m_BufferedImage);

	//	KillTimer(1);
		if (m_DeviceInfos[m_CurrentDevice].m_pMapping!=NULL)
			STDFUPRT_DestroyMapping(&m_DeviceInfos[m_CurrentDevice].m_pMapping);
	/*	POSITION Pos=m_DetachedDevs.GetStartPosition();
		while (Pos)
		{
			CString Key;
			void *Value;

			m_DetachedDevs.GetNextAssoc(Pos, Key, Value);
			delete (PUSB_DEVICE_DESCRIPTOR)Value;
		}
		m_DetachedDevs.RemoveAll();*/
	}
}


void ShowArgError(const cxxopts::Options& options, const std::string& message)
{
	std::cout << message << std::endl;
	std::cout << std::endl << std::endl;
	std::cout << options.help() << std::endl;
	exit(1);
}


/*******************************************************************************************/
/* Function    : main																       */
/* IN          : Arguments		                                                           */
/* OUT         : status ( 0 if sucess)                                                     */
/* Description : Main console command line demo                                            */
/*******************************************************************************************/

int main(int argc, const char* argv[])
{
	cxxopts::Options options("DfuSeCommand", "DfuSe programmer for QMK");
	options.add_options()
		("h,help", "Print this help message")
		("u,upload", "Read firmware from device into <file>", cxxopts::value<std::string>(), "<file>")
		("d,download", "Write firmware from <file> into device", cxxopts::value<std::string>(), "<file>")
		("r,restart", "Restart the device")
		("D,device", "Device path", cxxopts::value<std::string>(), "<devpath>")
		("v,verify", "Verify after download");
	try
	{
		auto result = options.parse(argc, argv);
		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			return 0;
		}
		if (result.count("restart"))
		{
			Restart = true;
		}
		if (result.count("verify"))
		{
			Verify = true;
		}

		if (result.count("upload"))
		{
			if (result.count("download"))
			{
				ShowArgError(options, "You can't specify both upload and download at the same time");
			}
			else
			{
				m_UpFileName = result["upload"].as<std::string>().c_str();
				if(!FileExist((LPCTSTR)m_UpFileName))
				{
					printf( "file %s does not exist .. Creating file\n", m_UpFileName);
					fflush(NULL);
					FILE* fp = fopen((LPCTSTR)m_UpFileName, "a+");
					fclose(fp);
				}
				else
				{
					printf( "file %s exists .. Overwriting\n", m_UpFileName);
					fflush(NULL);
				}
			}
		}
		else if (result.count("download"))
		{
			m_DownFileName = result["download"].as<std::string>().c_str();
			_splitpath(m_DownFileName,Drive,Dir,Fname,Ext);
			ptr=strupr(Ext);
			strcpy(Ext, ptr);
			if (!FileExist((LPCTSTR)m_DownFileName))
			{
				std::cout << "Error: File " << m_DownFileName << " does not exist" << std::endl;
				return 1;
			}
		}
		else if(!Restart)
		{
			ShowArgError(options, "Please specify either upload, download and/or restart");
		}

		Refresh();

		if (devIndex == 0)
		{
			std::cout << "Error: No devices found. Please, plug your DFU Device!" << std::endl;
			return 1;
		}
		else if (result.count("device"))
		{
			auto path = result["device"];
			bool found = false;
			for (int i = 0; i < devIndex; i++)
			{
				if (m_DeviceInfos[i].m_DevPath == path.as<std::string>().c_str())
				{
					found = true;
					m_CurrentDevice = i;
					break;
				}
			}
			if (!found)
			{
				std::cout << "Error: Device" << std::endl << " " << path.as<std::string>() << std::endl;
				std::cout << " is not connected" << std::endl;
				return 1;
			}
		}
		else if (devIndex > 1)
		{
			std::cout << "Error: More than one device found. Please, disconnect the rest" << std::endl;
			std::cout << "       or specify the device path!" << std::endl;
			return 1;
		}

		MSG Msg;
		UINT TimerId = SetTimer(NULL, 0, 500, (TIMERPROC) &TimerProc);
		
		if (m_UpFileName != "")
		{
			if (LaunchUpload())
			{
				return 1;
			}

		}
		else if (m_DownFileName != "")
		{
			if (LaunchUpgrade())
			{
				return 1;
			}
		}
		else if (Restart)
		{
			LaunchReboot();
		}

		while (GetMessage(&Msg, NULL, 0, 0)) 
		{
			DispatchMessage(&Msg);
		}

		OnCancel();
	}
	catch (const cxxopts::OptionException& e)
	{
		std::string err = "Invalid arguments: ";
		err += e.what();
		ShowArgError(options, err);
	}
#if 0
	
	if (argc == 1)  // wrong parameters

	{
		man();



	}
	else
	{
		int arg_index = 1;
		bool reboot = false;

		while(arg_index < argc)
		{
			if(!Is_Option(argv[arg_index])) 
			{
			   if (arg_index <= (argc - 1)) 
				  printf("bad parameter [%s] \n", argv[arg_index]);


			  printf("\n Press any key to continue ..."); 
			  getchar(); 
			  return 1;
			}

			//============================ Show the man =========================================
			if (strcmp(argv[arg_index],"-?")==0) 
			{
				man(); 
				return 0;
			}
			//=============================== connect ============================================
			else if (strcmp(argv[arg_index],"-c")==0)
			{
			   while(arg_index < argc)
			   {
				 if (arg_index< argc-1) 
					 arg_index++;
				 else 
					 break;
				 if(Is_Option(argv[arg_index])) // Set default connection settings and continue with the next option
					 break;	
					 
				 else if(Is_SubOption(argv[arg_index])) // Get connection settings
				 {
					 if (arg_index< argc-1) 
						 arg_index++;
					 else 
						 break; 			 
					 if (strcmp(argv[arg_index-1],"--de")==0) 
					 {
						 m_CurrentDevice = atoi(argv[arg_index]);//0, 1, 2 etc...) \n");
						 //arg_index--;
					 }
					 else if (strcmp(argv[arg_index-1],"--al")==0) 
					 {
						 m_CurrentTarget = atoi(argv[arg_index]);//0, 1, 2 etc...) \n");
						 //arg_index--;
					 }
				 }
				 else 
				 {
					 if (arg_index < argc - 1) 
						{printf("bad parameter [%s] \n", argv[arg_index]);
						fflush(NULL);
						}
						
					 /* TO DO:  Free device and buffers*/
					 printf("\n Press any key to continue ..."); 
					 fflush(NULL);
					 getchar(); 
					 return 1;
				 }
			   }
				// Enumerate the DFU device and Set Buffers
				Refresh();
			}
			//============================ UPLOAD ===============================================
			else if (strcmp(argv[arg_index],"-u")==0)
			{
				while(arg_index < argc)
				{
					if (arg_index< argc-1) 
						arg_index++;
					else 
						break;

					if(Is_Option(argv[arg_index])) 
						break;

					else if(Is_SubOption(argv[arg_index]))
					{
						if (arg_index< argc) 
							arg_index++;
						else 
							 break;

						if (strcmp(argv[arg_index-1],"--fn")==0) 
						{
							m_UpFileName = argv[arg_index];
							arg_index++;
						}
					}
					else 
					 {
						 if (arg_index <= (argc - 1)) 
							{ printf("bad parameter [%s] \n", argv[arg_index]);
							fflush(NULL); }

						 /* TO DO:  Free device and buffers*/

						 printf("\n Press any key to continue ..."); 
						 fflush(NULL);
						 getchar(); 
						 return 1;
					 }
			   }

			   if(!FileExist((LPCTSTR)m_UpFileName))
			   {
					  if ( m_UpFileName != "")
					  {
						  printf( "file %s does not exist .. Creating file\n", m_UpFileName);
						  fflush(NULL);
						  FILE* fp = fopen((LPCTSTR)m_UpFileName, "a+");
						  fclose(fp);
					  }
					  else 
						  
					  {
					  
						printf( "file %s does not exist .. \n", m_UpFileName);
						fflush(NULL);
						return 1;
					  }
					
			   }



			   MSG Msg;
			   UINT TimerId = SetTimer(NULL, 0, 500, (TIMERPROC) &TimerProc);

			   LaunchUpload();
			   
			   while (GetMessage(&Msg, NULL, 0, 0)) 
			   {
					DispatchMessage(&Msg);
			   }

			   //return 0;

			   OnCancel();

			}
			
			//============================ DOWNLOAD ==============================================
			else if (strcmp(argv[arg_index],"-d")==0)
			{
				while(arg_index < argc)
				{
				 if (arg_index< argc-1) 
					 arg_index++;
				 else 
					 break;

				 if(Is_Option(argv[arg_index])) 
					 break;

				 else if(Is_SubOption(argv[arg_index]))
				 {
					 if (arg_index< argc) 
						 arg_index++;
					 else 
						 break;

					 if (strcmp(argv[arg_index-1],"--v")==0) 
					 {
						 Verify = true;
						 arg_index--;
					 }
					 else if (strcmp(argv[arg_index-1],"--o")==0) 
					 {
						 Optimize = true;
						 arg_index--;
					 }
					 else if (strcmp(argv[arg_index-1],"--fn")==0) 
					 {
						 m_DownFileName = argv[arg_index];
						 _splitpath(m_DownFileName,Drive,Dir,Fname,Ext);
						 ptr=strupr(Ext);
						 strcpy(Ext, ptr);
						 arg_index++;
					 }
				 }
				 else 
				 {
					 if (arg_index <= (argc - 1)) 
						{ printf("bad parameter [%s] \n", argv[arg_index]);
						fflush(NULL);
						}

					 /* TO DO:  Free device and buffers*/

					 printf("\n Press any key to continue ..."); 
					 fflush(NULL);
					 getchar(); 
					 return 1;
				 }
			   }

			  

			  if(!FileExist((LPCTSTR)m_DownFileName))
			   {
					printf( "file does not exist %s \n", m_DownFileName);
					fflush(NULL);

					// TO DO:  Free device and buffers

					 printf("\n Press any key to continue ..."); 
					 fflush(NULL);
					 getchar(); 
					 return 1;
			   }



			   MSG Msg;
			   UINT TimerId = SetTimer(NULL, 0, 500, (TIMERPROC) &TimerProc);
			   
			   LaunchUpgrade();
			   
			   while (GetMessage(&Msg, NULL, 0, 0)) 
			   {
					DispatchMessage(&Msg);
			   }

			   OnCancel();

			   //return 0;
   

			}
			else if (strcmp(argv[arg_index], "-r") == 0)
			{
				reboot = true;
				arg_index++;
			}
			
		} // While

		if (reboot)
		{
		   MSG Msg;
		   UINT TimerId = SetTimer(NULL, 0, 500, (TIMERPROC) &TimerProc);
		   LaunchReboot();
		   while (GetMessage(&Msg, NULL, 0, 0)) 
		   {
				DispatchMessage(&Msg);
		   }
		   OnCancel();
		}

		return 0;
	}
		
			
#endif	
	return 0;
}
