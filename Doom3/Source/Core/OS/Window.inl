

unsigned long long doom::os::_GetTickCount()
{
#if defined(_WIN32)
	return GetTickCount();
#elif defined(_WIN64)
	return GetTickCount64();
#endif
}

void doom::os::_Sleep(const unsigned long milliseconds)
{
	Sleep(milliseconds);
}

unsigned int doom::os::_GetCurrentProcessorNumber()
{
	return GetCurrentProcessorNumber();
}

PLATFORM_HANDLE doom::os::_GetCurrenThreadHandle()
{
	return GetCurrentThread();
}

unsigned int doom::os::_GetCurrenThreadID()
{
	return GetCurrentThreadId();
}

HANDLE doom::os::_GetCurrenProcess()
{
	return GetCurrentProcess();
}

bool doom::os::_SetProcessAffinityMask(const unsigned long long processAffinitMask)
{
	return SetProcessAffinityMask(GetCurrentProcess(), static_cast<DWORD_PTR>(processAffinitMask));
}

void doom::os::_SetThreadAffinity(const PLATFORM_HANDLE threadHandle, const unsigned long long threadAffinitMask)
{
	D_ASSERT(threadHandle != PLATFORM_INVALID_HANDLE_CONSTANT);

	SetThreadAffinityMask(threadHandle, static_cast<DWORD_PTR>(threadAffinitMask));
}


unsigned long long doom::os::_GetCurrentThreadAffinity(const PLATFORM_HANDLE threadHandle)
{
	D_ASSERT(threadHandle != PLATFORM_INVALID_HANDLE_CONSTANT);

	//https://stackoverflow.com/questions/6601862/query-thread-not-process-processor-affinity
	const unsigned long long originalMask = SetThreadAffinityMask(threadHandle, 0xFFFFFFFFFFFFFFFF);
	SetThreadAffinityMask(threadHandle, originalMask);
	return originalMask;
}

/*
unsigned int doom::os::_GetCurrentProcessorNumber()
{
	return GetCurrentProcessorNumber();
}
*/