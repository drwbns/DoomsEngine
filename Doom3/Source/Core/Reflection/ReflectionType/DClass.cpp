#include "DClass.h"

#include "../ReflectionManager.h"
#include "DAttributeList.h"

std::unordered_map<UINT32, std::vector<dooms::reflection::DField>> dooms::reflection::DClass::PropertyCacheHashMap{};
std::unordered_map<UINT32, std::vector<dooms::reflection::DFunction>> dooms::reflection::DClass::FunctionCacheHashMap{};

namespace dClassHelper
{
	const clcpp::Class* GetclcppClass(const UINT32 nameHash)
	{
		const clcpp::Database& db = dooms::reflection::ReflectionManager::GetSingleton()->GetclcppDatabase();
		const clcpp::Type* const clcppType = db.GetType(nameHash);
		D_ASSERT(clcppType != nullptr);
		D_ASSERT(clcppType->kind == clcpp::Primitive::KIND_CLASS);
		return clcppType->AsClass();
	}

	void GetDFields_Recursive(const clcpp::Class* const clcppClass, std::vector<dooms::reflection::DField>& list)
	{
		D_ASSERT(clcppClass != nullptr);

		for (UINT32 i = 0; i < clcppClass->base_types.size; i++)
		{
			if (clcppClass->base_types[i]->kind == clcpp::Primitive::Kind::KIND_CLASS)
			{
				GetDFields_Recursive(clcppClass->base_types[i]->AsClass(), list);
			}
		}
		
		list.reserve(list.size() + clcppClass->fields.size);

		for (std::ptrdiff_t i = clcppClass->fields.size ; i > 0 ; i--)
		{
			list.emplace_back(clcppClass->fields[static_cast<UINT32>(i - 1)]);
		}
	}

	// Return DProperties of passed clcpp::Class including base class's properties
	std::vector<dooms::reflection::DField> GetDFields(const clcpp::Class* const clcppClass)
	{
		D_ASSERT(clcppClass != nullptr);

		const clcpp::Class* targetclcppClasss = clcppClass;

		std::vector<dooms::reflection::DField> dFieldList {};

		// TODO : Optimization
		GetDFields_Recursive(clcppClass, dFieldList);

		// iterate base class
		return dFieldList;
	}



	void GetDFunctions_Recursive(const clcpp::Class* const clcppClass, std::vector<dooms::reflection::DFunction>& list)
	{
		D_ASSERT(clcppClass != nullptr);

		for (UINT32 i = 0; i < clcppClass->base_types.size; i++)
		{
			if (clcppClass->base_types[i]->kind == clcpp::Primitive::Kind::KIND_CLASS)
			{
				GetDFunctions_Recursive(clcppClass->base_types[i]->AsClass(), list);
			}
		}

		list.reserve(list.size() + clcppClass->methods.size);

		for (std::ptrdiff_t i = clcppClass->methods.size; i > 0; i--)
		{
			list.emplace_back(clcppClass->methods[static_cast<UINT32>(i - 1)]);
		}
	}

	// Return DProperties of passed clcpp::Class including base class's properties
	std::vector<dooms::reflection::DFunction> GetDFunctions(const clcpp::Class* const clcppClass)
	{
		D_ASSERT(clcppClass != nullptr);

		const clcpp::Class* targetclcppClasss = clcppClass;

		std::vector<dooms::reflection::DFunction> dFunctionList{};

		// TODO : Optimization
		GetDFunctions_Recursive(clcppClass, dFunctionList);

		// iterate base class
		return dFunctionList;
	}
}


dooms::reflection::DClass::DClass(dooms::DObject* const dObject)
	: DClass(dObject->GetTypeHashVlue())
{
}

dooms::reflection::DClass::DClass(const UINT32 nameHash)
	: DType(dClassHelper::GetclcppClass(nameHash)), clClass(clType->AsClass())
{
	D_ASSERT(clClass != nullptr);

}

dooms::reflection::DClass::DClass(const char* const classFullName)
	: DType(classFullName), clClass(clType->AsClass())
{
	D_ASSERT(clClass != nullptr);

}

dooms::reflection::DClass::DClass(const clcpp::Class* const clcppClass)
	: DType(clcppClass), clClass(clcppClass)
{
	D_ASSERT(clPrimitive != nullptr);
}

dooms::reflection::DClass::DClass(const clcpp::Type* const clcppType)
	:
	DType((clcppType->kind == clcpp::Primitive::Kind::KIND_CLASS) ? clcppType : nullptr),
	clClass( static_cast<const clcpp::Class*>(clPrimitive) )
{
	D_ASSERT(clPrimitive != nullptr);
	D_ASSERT(clClass != nullptr);
}


const std::vector<dooms::reflection::DField>& dooms::reflection::DClass::GetDFieldList() const
{
	// TODO : Change unordered_map to std::vector
	// TODO : Sort Field List based on field offset
	D_ASSERT(IsValid() == true);

	std::vector<dooms::reflection::DField>* propertyList = nullptr;

	auto iter = PropertyCacheHashMap.find(clPrimitive->name.hash);
	if(iter == PropertyCacheHashMap.end())
	{// if property list isn't cached
		std::vector<dooms::reflection::DField> cachedPropertyList = dClassHelper::GetDFields(clClass);
		auto result = PropertyCacheHashMap.emplace(clPrimitive->name.hash, std::move(cachedPropertyList));
		propertyList = &(result.first->second);
	}
	else
	{
		propertyList = &(iter->second);
	}

	D_ASSERT(propertyList != nullptr);

	return *propertyList;
}

const std::vector<dooms::reflection::DFunction>& dooms::reflection::DClass::GetDFunctionList() const
{
	D_ASSERT(IsValid() == true);

	//key : function short name
	std::vector<dooms::reflection::DFunction>* functionList = nullptr;

	auto iter = FunctionCacheHashMap.find(clPrimitive->name.hash);
	if (iter == FunctionCacheHashMap.end())
	{// if property list isn't cached
		std::vector<dooms::reflection::DFunction> cachedFunctionList = dClassHelper::GetDFunctions(clClass);
		auto result = FunctionCacheHashMap.emplace(clPrimitive->name.hash, std::move(cachedFunctionList));
		functionList = &(result.first->second);
	}
	else
	{
		functionList = &(iter->second);
	}

	D_ASSERT(functionList != nullptr);

	return *functionList;
}

bool dooms::reflection::DClass::GetDField(const char* const fieldName, dooms::reflection::DField& dProperty) const
{
	D_ASSERT(fieldName != nullptr);
	D_ASSERT_LOG(std::string_view(fieldName).find("::") == std::string::npos, "Please pass short name to DClass::GetDField");

	const std::vector<dooms::reflection::DField>& propertyList = GetDFieldList();

	bool isSuccess = false;
	
	auto iter = std::find_if(propertyList.begin(), propertyList.end(), [fieldName](const reflection::DField& dField) -> bool {return strcmp(dField.GetFieldName(), fieldName) == 0; });
	if(iter != propertyList.end())
	{
		dProperty = *iter;
		isSuccess = true;
	}

	D_ASSERT_LOG(isSuccess == true, "Fail to find Field ( %s ) from ( %s )", fieldName, GetTypeFullName());

	return isSuccess;
}

bool dooms::reflection::DClass::GetDFunction(const char* const functionName, dooms::reflection::DFunction& dFunction) const
{
	D_ASSERT(functionName != nullptr);
	D_ASSERT_LOG(std::string_view(functionName).find("::") == std::string::npos, "Please pass short name to DClass::GetDFunction");

	const std::vector<dooms::reflection::DFunction>& functionList = GetDFunctionList();

	bool isSuccess = false;

	auto iter = std::find_if(functionList.begin(), functionList.end(), [functionName](const dooms::reflection::DFunction& dFunction) -> bool {return strcmp(dFunction.GetFunctionName(), functionName) == 0; });
	if (iter != functionList.end())
	{
		dFunction = *iter;
		isSuccess = true;
	}

	D_ASSERT_LOG(isSuccess == true, "Fail to find Function ( %s ) from ( %s )", functionName, GetTypeFullName());

	return isSuccess;
}

dooms::reflection::DAttributeList dooms::reflection::DClass::GetDAttributeList() const
{
	std::vector<dooms::reflection::DAttribute> attributeList;

	D_ASSERT(clClass != nullptr);

	attributeList.reserve(clClass->attributes.size);
	for (UINT32 i = 0; i < clClass->attributes.size; i++)
	{
		attributeList.emplace_back(clClass->attributes[i]);
	}

	return std::move(attributeList);
}

