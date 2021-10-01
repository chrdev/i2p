// i2p is an img2pdf launcher.
/*
BSD Zero Clause License

Copyright (c) [2021] [chrdev]

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef UNICODE
#define UNICODE
#endif // !UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // !_UNICODE

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // CommandLineToArgvW

#include <stdbool.h> // bool
#include <stdlib.h> // _countof
#include <stdio.h> // wprintf_s

static const wchar_t kDefaultExtensions[] = L" *.jpg *.jpeg *.png *.gif *.tif *.tiff *.jp2 *.jpa *.jpm *.jpx";

static void showHelp(void) {
	const wchar_t text[] =
		L"Feeds \"DIR /ON /A-D-H\" result to img2pdf.\n"
		L"\n"
		L"i2p output_file_name [input_ext1 input_ext2 ... | *] [img2pdf arguments]\n"
		L"\n"
		L"Examples:\n"
		L"  i2p manual jpg png -D\n"
		L"    Generate manual.pdf from *.jpg and *.png files and pass -D to img2pdf\n"
		L"\n"
		L"  i2p scan\n"
		L"    Generate scan.pdf from default image types, which are\n"
		L"   %s\n"
		L"\n"
		L"Caution:\n"
		L"  The target pdf file can be overwritten by img2pdf without prompt!\n";
	wprintf_s(text, kDefaultExtensions);
}

static void errorExit(const wchar_t* extra) {
	DWORD lastError = GetLastError();
	wchar_t* errorMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lastError, 0, (LPWSTR)&errorMsg, 0, NULL);
	wprintf_s(L"Error #%u: %s  - %s\n", lastError, errorMsg, extra);
	// No LocalFree because we are exiting process.
	ExitProcess(1);
}

typedef struct Args {
	wchar_t listFileName[16];
	wchar_t outputFileName[MAX_PATH];
	wchar_t dirCmd[2048];
	wchar_t img2pdfCmd[2048];
}Args;

static inline int makeDirCmd(Args* args, int argBegin, int argc, wchar_t** argv) {
	static const wchar_t kCmd[] = L"cmd.exe /A /C dir /B /ON /A-D-H";

	wchar_t* buffer = args->dirCmd;
	rsize_t cch = _countof(((Args*)0)->dirCmd);
	
	wcscpy_s(buffer, cch, kCmd);

	if (argBegin >= argc || argv[argBegin][0] == L'-') {
		wcscat_s(buffer, cch, kDefaultExtensions);
	}
	else {
		for (; argBegin < argc; ++argBegin) {
			switch (argv[argBegin][0]) {
			case L'-':
				return argBegin;
			case L'*':
				wcscat_s(buffer, cch, kDefaultExtensions);
				continue;
			default:
				wcscat_s(buffer, cch, L" *.");
				wcscat_s(buffer, cch, argv[argBegin]);
				continue;
			}
		}
	}
	
	return argBegin;
}

static inline void fillPassthrough(wchar_t* dest, size_t cch, int argBegin, int argc, wchar_t** argv) {
	for (int i = argBegin; i < argc; ++i) {
		wcsncat_s(dest, cch, L" ", 1);
		wcscat_s(dest, cch, argv[i]);
	}
}

static inline void makeImg2pdfCmd(Args* args, int passthroughArgBegin, int argc, wchar_t** argv) {
	const wchar_t kPattern[] = L"img2pdf.exe --from-file %s -o %s";
	wchar_t* buf = args->img2pdfCmd;
	size_t cch = _countof(((Args*)0)->img2pdfCmd);
	_swprintf_c(buf, cch, kPattern, args->listFileName, args->outputFileName);
	fillPassthrough(buf, cch, passthroughArgBegin, argc, argv);
}

static inline bool parseArgs(Args* args) {
	bool result = false;

	int argc;
	wchar_t** argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (!argv) errorExit(L"CommandLineToArgvW");

	if (argc < 2 || argv[1][0] == L'-' || argv[1][0] == L'/') goto cleanup;

	_swprintf_c(args->listFileName, _countof(((Args*)0)->listFileName), L"i2p%08x", GetTickCount());
	_swprintf_c(args->outputFileName, _countof(((Args*)0)->outputFileName), L"%s.pdf", argv[1]);

	int argIndex = makeDirCmd(args, 2, argc, argv);
	makeImg2pdfCmd(args, argIndex, argc, argv);

	result = true;

cleanup:;
	LocalFree(argv);
	return result;
}

static inline void createPipes(HANDLE* inR, HANDLE* inW, HANDLE* outR, HANDLE* outW, HANDLE* errR, HANDLE* errW) {
	SECURITY_ATTRIBUTES sa = {
		.nLength = sizeof(SECURITY_ATTRIBUTES),
		.lpSecurityDescriptor = NULL,
		.bInheritHandle = TRUE
	};

	if (!CreatePipe(inR, inW, &sa, 0))
		errorExit(L"Stdin CreatePipe");
	if (!SetHandleInformation(*inW, HANDLE_FLAG_INHERIT, 0))
		errorExit(L"Stdin SetHandleInformation");
	if (!CreatePipe(outR, outW, &sa, 0))
		errorExit(L"Stdout CreatePipe");
	if (!SetHandleInformation(*outR, HANDLE_FLAG_INHERIT, 0))
		errorExit(L"Stdout SetHandleInformation");
	if (!CreatePipe(errR, errW, &sa, 0))
		errorExit(L"Stderr CreatePipe");
	if (!SetHandleInformation(*errR, HANDLE_FLAG_INHERIT, 0))
		errorExit(L"Stderr SetHandleInformation");
}

static inline void runDir(wchar_t* cmd, HANDLE inR, HANDLE outW, HANDLE errW) {
	STARTUPINFO si = {
		.cb = sizeof(STARTUPINFO),
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdInput = inR,
		.hStdOutput = outW,
		.hStdError = errW,
	};
	PROCESS_INFORMATION pi;
	BOOL ok = CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if (!ok) errorExit(L"cmd.exe CreateProcess");

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(inR);
	CloseHandle(outW);
	CloseHandle(errW);
}

static inline DWORD getCchToWrite(const char* begin, const char* end, const char** next, bool* hasSep) {
	*hasSep = false;
	DWORD result = 0;
	for (*next = begin; *next < end; ++*next) {
		switch (**next) {
		case '\r':
			continue;
		case '\n':
			*hasSep = true;
			++*next;
			return result;
		default:
			++result;
			continue;
		}
	}
	return result;
}

// ANSI in, ANSI out
static inline void crlfToNul(HANDLE out, HANDLE in) {
	char buffer[512];
	for (;;) {
		DWORD cb;
		BOOL ok = ReadFile(in, buffer, sizeof(buffer), &cb, NULL);
		if (!ok || !cb) break;

		for (char *begin = buffer, *end = buffer + cb, *next; begin < end; begin = next) {
			bool hasSep;
			DWORD cchToWrite = getCchToWrite(begin, end, (const char**)&next, &hasSep);
			DWORD cch;
			if (cchToWrite) {
				ok = WriteFile(out, begin, cchToWrite, &cch, NULL);
				if (!ok) errorExit(L"List file WriteFile");
			}
			if (hasSep) {
				ok = WriteFile(out, "\0", 1, &cch, NULL);
				if (!ok) errorExit(L"List file WriteFile");
			}
		}
	}
}

static inline void createListFile(const wchar_t* fileName, HANDLE input, HANDLE errInput) {
	for (DWORD cb; ; Sleep(50)) {
		PeekNamedPipe(errInput, NULL, 0, 0, &cb, NULL);
		if (cb) {
			wprintf_s(L"No Input File\n");
			ExitProcess(1);
		}
		PeekNamedPipe(input, NULL, 0, 0, &cb, NULL);
		if (cb) break;
	}
	HANDLE h = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (h == INVALID_HANDLE_VALUE) errorExit(L"List file CreateFile");
	crlfToNul(h, input);
	CloseHandle(h);
}

static inline void runImg2pdfAndWait(wchar_t* cmd) {
	STARTUPINFO si = {
		.cb = sizeof(STARTUPINFO),
	};
	PROCESS_INFORMATION pi;
	BOOL ok = CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!ok) errorExit(L"img2pdf.exe CreateProcess");
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
}

void wmain(void) {
	Args args;
	if (!parseArgs(&args)) {
		showHelp();
		ExitProcess(1);
	}

	HANDLE inR, inW, outR, outW, errR, errW;
	createPipes(&inR, &inW, &outR, &outW, &errR, &errW);
	runDir(args.dirCmd, inR, outW, errW);
	createListFile(args.listFileName, outR, errR);
	CloseHandle(inW);
	CloseHandle(outR);
	CloseHandle(errR);

	runImg2pdfAndWait(args.img2pdfCmd);
	DeleteFile(args.listFileName);

	ExitProcess(0);
}
