// library.cpp : Defines the exported functions for the DLL application.
#include "pch.h"
#include <windows.h>
//#include "stdafx.h"
#include "stdio.h"
#include "string.h"

#ifdef __cplusplus 
extern "C" {
#endif

	struct OperationInfo
	{
		HANDLE hFile;
		DWORD NumberOfBytes;
		CHAR  buf[100];
		DWORD  pos_in_file;
		DWORD  pos_out_file;
		OVERLAPPED Overlapped;
	};

	__declspec(dllexport) BOOL read(OperationInfo *info)
	{
		BOOL readResult;
		DWORD NumberOfBytesTrasferred;
		info->Overlapped.Offset = info->pos_in_file;

		ReadFile(info->hFile, info->buf, info->NumberOfBytes, NULL, &info->Overlapped);
		WaitForSingleObject(info->Overlapped.hEvent, INFINITE);
		readResult = GetOverlappedResult(info->hFile, &info->Overlapped, &NumberOfBytesTrasferred, FALSE);

		if (readResult) info->pos_in_file = info->pos_in_file + NumberOfBytesTrasferred;
		return readResult;
	}
	__declspec(dllexport) BOOL write(OperationInfo *info)
	{
		BOOL writeResult;
		DWORD NumberOfBytesTrasferred;
		info->Overlapped.Offset = info->pos_out_file;

		WriteFile(info->hFile, info->buf, info->Overlapped.InternalHigh, NULL, &info->Overlapped);
		WaitForSingleObject(info->Overlapped.hEvent, INFINITE);
		writeResult = GetOverlappedResult(info->hFile, &info->Overlapped, &NumberOfBytesTrasferred, FALSE);

		if (writeResult) info->pos_out_file = info->pos_out_file + NumberOfBytesTrasferred;
		return writeResult;
	}

#ifdef __cplusplus
}
#endif