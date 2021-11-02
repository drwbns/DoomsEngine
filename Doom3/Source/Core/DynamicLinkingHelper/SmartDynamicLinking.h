#pragma once

#include <Core.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include <DirectXTex.h>
#include <excpt.h>

#include <UI/PrintText.h>

namespace doom
{
	struct DynamicLinkingLibrary
	{
		static std::mutex LoadUnLoadDLLMutexs;

		std::string mLibraryPath;
		std::shared_ptr<void> mLibrary;

		DynamicLinkingLibrary(const std::string& libraryPath);
		DynamicLinkingLibrary(const std::string& libraryPath, const unsigned long dwFlags);

		void* LoadDynamicLinkingLibrary(const unsigned long dwFlags);
		bool UnloadDynamicLinkingLibrary();
	};

	class SmartDynamicLinking
	{
		DynamicLinkingLibrary mDynamicLinkingLibrary;

	private:

		void* _GetProcAddress(const char* const functionName);
		
	

	public :

		SmartDynamicLinking(const std::string& csharpLibraryPath);
		~SmartDynamicLinking();

		SmartDynamicLinking(const SmartDynamicLinking&);
		SmartDynamicLinking(SmartDynamicLinking&& _SmartCSharpLibrary) noexcept;
		SmartDynamicLinking& operator=(const SmartDynamicLinking&);
		SmartDynamicLinking& operator=(SmartDynamicLinking&& _SmartCSharpLibrary) noexcept;

		template <typename... Args>
		bool CallFunction(const char* functionName, Args&&... args);

		template <typename RETURN_TYPE, typename... Args>
		bool CallFunctionWithReturn(const char* functionName, RETURN_TYPE& returnValue, Args&&... args);
	};


	template <typename ... Args>
	bool SmartDynamicLinking::CallFunction(const char* const functionName, Args&&... args)
	{
		typedef void(__cdecl* functionType)(Args...);

		functionType function = reinterpret_cast<functionType>(_GetProcAddress(functionName));

		bool IsSuccess = false;
		if (function != nullptr)
		{
			try
			{
				function(std::forward<Args>(args)...);
				IsSuccess = true;
			}
			catch (const std::exception& ex) {
				D_ASSERT_LOG(false, "exception from csharp function ( %s )", ex.what());
				doom::ui::PrintText("exception from csharp function ( %s )", ex.what());
			}
			catch (const std::string& ex) {
				D_ASSERT_LOG(false, "exception from csharp function ( %s )", ex.c_str());
				doom::ui::PrintText("exception from csharp function ( %s )", ex.c_str());
			}
			catch (...) {
				D_ASSERT_LOG(false, "exception from csharp function");
				doom::ui::PrintText("exception from csharp function");
			}

		}

		return IsSuccess;
	}

	int filter(unsigned int code, struct _EXCEPTION_POINTERS* ptr);


	template <typename RETURN_TYPE, typename ... Args>
	bool SmartDynamicLinking::CallFunctionWithReturn(const char* const functionName, RETURN_TYPE& returnValue, Args&&... args)
	{
		typedef RETURN_TYPE(__cdecl* functionType)(Args...);

		functionType function = reinterpret_cast<functionType>(_GetProcAddress(functionName));

		bool IsSuccess = false;
		if(function != nullptr)
		{
			__try
			{
				returnValue = function(std::forward<Args>(args)...);
				IsSuccess = true;
			}
			__except (filter(GetExceptionCode(), GetExceptionInformation()))
			{
				//... yey!
			}
			
			
		}

		return IsSuccess;
	}
}

