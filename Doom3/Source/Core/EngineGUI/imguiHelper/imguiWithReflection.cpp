#include "imguiWithReflection.h"

#include <Core.h>

#include <unordered_map>
#include <string_view>
#include <algorithm>
#include <string.h>

#include "imguiFieldFunctionGetter.h"

#include <Reflection/ReflectionType/DClass.h>
#include <Reflection/ReflectionType/DAttributeList.h>

#include <Random.h>

#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include <Matrix2x2.h>
#include <Matrix3x3.h>
#include <Matrix4x4.h>
#include <Quaternion.h>

#include <Scene/Entity.h>

#include "imgui.h"
#include "DObject/DObject.h"

#include <vector_erase_move_lastelement/vector_swap_popback.h>

#include "imgui_internal.h"
#include "../engineGUIServer.h"

#include <EngineGUI/PrintText.h>


namespace dooms
{
	namespace ui
	{
		namespace imguiWithReflectionHelper
		{
			
			
			
			static bool isInitialized = false;

			static std::vector<void*> MultipleDrawChecker{};

			

			void OnStartDrawGUI(const reflection::DAttributeList& attributeList)
			{
				if (attributeList.GetIsReadOnly() == true)
				{
					ImGui::BeginDisabled();
				}
			}

			void OnEndDrawGUI(const char* const label, const reflection::DAttributeList& attributeList)
			{
				// this is little slow. but it's acceptable
				if (attributeList.GetIsReadOnly() == true)
				{
					ImGui::EndDisabled();

				}

				if(ImGui::IsItemHovered())
				//if(ImGui::GetFocusID() == ImGui::GetID(label))
				{
					if (const char* const tooltipStr = attributeList.GetTooltip())
					{
						ImGui::SetTooltip("Tooltip : %s", tooltipStr);
					}
				}
				
			}

			bool DrawImguiFieldFromDField(void* const object, const char* const label, const char* const typeFullName, const reflection::DAttributeList& attributeList, bool& isValueChange)
			{
				bool isSuccessToDrawGUI = false;
				
				if (attributeList.GetIsVisibleOnGUI() == true)
				{
					dooms::ui::imguiFieldFunctionGetter::IMGUI_WITH_REFLECTION_FUNC func =
						dooms::ui::imguiFieldFunctionGetter::GetImguiWithReflectionFunction(typeFullName);

					if (func != nullptr)
					{
						OnStartDrawGUI(attributeList);

						isValueChange = func(object, label, attributeList);
						isSuccessToDrawGUI = true;

						OnEndDrawGUI(label, attributeList);
					}
					else
					{
						D_DEBUG_LOG(eLogType::D_ERROR, "imguiWithReflection : Can't resolve type \" %s \"", typeFullName);
					}

				}

				return isSuccessToDrawGUI;
			}

			bool DrawImguiFieldFromDField(void* const object, const reflection::DField& dField, bool& isValueChange)
			{
				std::string fieldTypeFullName = dField.GetFieldTypeFullName();
				if (dField.GetFieldQualifier() == reflection::DField::eProperyQualifier::POINTER)
				{
					fieldTypeFullName += '*';
				}
				else if (dField.GetFieldQualifier() == reflection::DField::eProperyQualifier::REFERENCE)
				{
					fieldTypeFullName += '&';
				}

				return DrawImguiFieldFromDField(object, dField.GetFieldName(), fieldTypeFullName.c_str(), dField.GetDAttributeList(), isValueChange);
			}
			
			

			/// <summary>
			/// check DFunction can be showing on gui
			/// </summary>
			/// <param name="dFunction"></param>
			/// <returns></returns>
			bool GetIsFunctionGUIable(const reflection::DFunction& dFunction)
			{
				return (dFunction.GetIsHasReturnValue() == false) && (dFunction.GetParameterDFieldList().empty() == true);
			}

			bool DrawImguiFunctionButtonFromDFunction(void* const object, const dooms::reflection::DFunction dFunction)
			{
				bool isButtonClicked = false;

				if (GetIsFunctionGUIable(dFunction) == true)
				{
					const dooms::reflection::DAttributeList& attributeList = dFunction.GetDAttributeList();

					OnStartDrawGUI(attributeList);
					
					// when member function
					if (dFunction.GetIsMemberFunction() == true)
					{
						if (ImGui::Button(dFunction.GetFunctionName()))
						{
							const_cast<dooms::reflection::DFunction&>(dFunction).CallMemberFunctionNoReturnNoParameter(object);
							isButtonClicked = true;
						}
					}
					else
					{// when static function or glbal function
						if (ImGui::Button(dFunction.GetFunctionName()))
						{
							const_cast<dooms::reflection::DFunction&>(dFunction).CallFunctionNoReturn();
							isButtonClicked = true;
						}
					}
					
					OnEndDrawGUI(dFunction.GetFunctionName(), attributeList);
				}

				return isButtonClicked;
			}


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			/// <summary>
			/// 
			/// </summary>
			/// <param name="dClass"></param>
			/// <param name="object"></param>
			/// <param name="rawObjectName">objectType is DObject, you don't need set this. it will take name from DObject::GetDObjectName </param>
			/// <param name="objectType"></param>
			/// <returns></returns>
			bool DrawObjectGUI(const reflection::DClass& dClass, void* const object, const char* const rawObjectName, const eObjectType objectType)
			{
				D_ASSERT(dooms::ui::imguiWithReflection::GetIsIsInitialized() == true);

				bool isDObjectChanged = false;

				if (
					objectType == eObjectType::DObject ? IsValid(reinterpret_cast<dooms::DObject*>(object)) == true : true &&
					// check if DObject is already drawed to prevent infinite loop
					std::find(MultipleDrawChecker.begin(), MultipleDrawChecker.end(), object) == MultipleDrawChecker.end()
					)
				{
					MultipleDrawChecker.push_back(object);

					const std::unordered_map<std::string_view, dooms::reflection::DField>& dFieldList = dClass.GetDFieldList();

					//label
					const char* objectName = "";
					if(rawObjectName == nullptr || rawObjectName[0] == '\0')
					{
						if(objectType == eObjectType::DObject)
						{
							objectName = reinterpret_cast<dooms::DObject*>(object)->GetDObjectName().c_str();
						}
						else
						{// if rawObjectName is empty and object type is rawobject, objectname will be type full name
							objectName = dClass.GetTypeFullName();
						}
					}
					ImGui::TextColored(ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "%s", objectName);

					if (dFieldList.empty() == false)
					{
						for (auto& dFieldNode : dFieldList)
						{
							const dooms::reflection::DField& dField = dFieldNode.second;

							bool isFieldValueChanged = false;

							void* fieldRawValue = const_cast<dooms::reflection::DField&>(dField).GetRawFieldValue(object);

							const bool isGUIDrawed = dooms::ui::imguiWithReflectionHelper::DrawImguiFieldFromDField
							(
								fieldRawValue,
								dField,
								isFieldValueChanged
							);

						
							if(isGUIDrawed == false)
							{//fail to find special gui func. can't find proper gui function from map
								if (dField.GetFieldTypePrimitiveType() == reflection::DPrimitive::ePrimitiveType::CLASS)
								{
									const reflection::DClass fieldTypeDClass = dField.GetFieldTypeDClass();
									if (IsValid(reinterpret_cast<dooms::DObject*>(fieldRawValue)) == true)
									{// if type of field is child class of dooms::DObject
										isFieldValueChanged = DrawObjectGUI(fieldTypeDClass, fieldRawValue, dField.GetFieldName(), eObjectType::DObject);
									}
									else
									{
										if(dField.GetFieldQualifier() == reflection::DField::eProperyQualifier::VALUE)
										{
											isFieldValueChanged = DrawObjectGUI(fieldTypeDClass, fieldRawValue, dField.GetFieldName(), eObjectType::RawObject);
										}
									}
								}
							}


							if (objectType == eObjectType::DObject && isFieldValueChanged == true)
							{
								reinterpret_cast<dooms::DObject*>(object)->OnChangedByGUI(dField);
							}

							isDObjectChanged |= isFieldValueChanged;
						}
					}

					const std::unordered_map<std::string_view, dooms::reflection::DFunction>& dFunctionList = dClass.GetDFunctionList();
					if (dFunctionList.empty() == false)
					{
						for (auto& dFunctioNnode : dFunctionList)
						{
							const dooms::reflection::DFunction& dFunction = dFunctioNnode.second;

							DrawImguiFunctionButtonFromDFunction(object, dFunction);
						}
					}
					
					bool isGUIValueChanged;
					dooms::ui::imguiWithReflectionHelper::DrawImguiFieldFromDField(object, objectName, dClass.GetTypeFullName(), dClass.GetDAttributeList(), isGUIValueChanged);
				}


				return isDObjectChanged;
			}

			bool DrawDObjectGUI
			(
				const reflection::DClass& dClass, dooms::DObject* const dObject
			)
			{
				if(IsValid(dObject) == true)
				{
					return DrawObjectGUI(dClass, dObject, "", eObjectType::DObject);
				}
				else
				{
					return false;
				}
				
			}


			void ClearMultipleDrawChecker()
			{
				MultipleDrawChecker.clear();
			}
		}
	}
}

void dooms::ui::imguiWithReflection::Initialize()
{
	if (IsInitialized == false)
	{
		imguiFieldFunctionGetter::Initialize();

		IsInitialized = true;
	}
}




