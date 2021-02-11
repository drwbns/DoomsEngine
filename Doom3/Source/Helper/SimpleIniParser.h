#pragma once

#include <vector>
#include <utility>
#include <string>
#include <variant>
#include <unordered_map>
#include <type_traits>
#include "../Core/Core.h"




class IniData
{
public:
	using ValueType = typename  std::variant<std::string, int, double>;
	using VariableType = typename  std::pair<std::string, ValueType>;
	
private:
	std::unordered_map<std::string, std::vector<VariableType> > data{};
public:

	

	void AddSection(const std::string& section);
	void InsertVariable(const std::string& section, const VariableType& variable);

	template <typename T>
	T GetValue(const std::string& section, const std::string& variableKey) const
	{
		for (auto& variable : this->data.at(section))
		{
			if (variable.first == variableKey)
				return std::get<T>(variable.second);
		}

		return NULL;

	}
};

class SimpleIniParser
{
public:
	static IniData ParseIniFile(std::string fileDirectory);

	
};