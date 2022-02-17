#include "imguiFieldFunctionGetter.h"

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include <Quaternion_Scalar.h>

#include "ImGUISetting.h"
#include "Reflection/ReflectionType/DAttributeList.h"
#include "imguiWithReflection.h"
#include <Scene/Entity.h>
#include "Transform.h"
#include "magic_enum.hpp"

#include <Reflection/ReflectionType/DTemplateType.h>
#include <Reflection/ReflectionType/DEnum.h>

#include "Reflection/ReflectionType/DTemplateArgumentType.h"

#include "Reflection/TypeSpecialized/Helper/iteratorHelper.h"


namespace dooms
{
	namespace ui
	{
		namespace imguiFieldFunctionGetter
		{
			static bool IsInitialized = false;

			

			enum eImguiType
			{
				Color,
				Slider,
				Drag,
				InputText
			};

			int GetImguiFlags(const eImguiType imguiType, const reflection::DAttributeList& attributeList)
			{
				int flags = 0;


				if (attributeList.GetIsReadOnly() == true)
				{
					switch (imguiType)
					{
					case eImguiType::Color:
						flags |= ImGuiColorEditFlags_NoInputs;
						break;

					case eImguiType::Slider:
					case eImguiType::Drag:
						flags |= ImGuiSliderFlags_NoInput;
						break;

					default:
						D_ASSERT(false);
						break;
					}
				}

				return flags;
			}

			


			/// <summary>
			/// return : is value changed
			/// </summary>
		
			static std::unordered_map<std::string_view, IMGUI_WITH_REFLECTION_FUNC> ImguiSpecialDrawFuncMap{};



			bool imguiWithReflection_bool(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				//ImGui::Text("%s : %d", label, *static_cast<int*>(object));

				bool isChanged = false;

				bool isChecked = *static_cast<bool*>(object);
				if(ImGui::Checkbox(label, &isChecked))
				{
					*static_cast<bool*>(object) = isChecked;
					isChanged = true;
				}

				return isChanged;
			}


			bool imguiWithReflection_char(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const INT8 minValue = attributeList.GetMinValue<INT8>();
				const INT8 maxValue = attributeList.GetMaxValue<INT8>();

				return ImGui::DragScalar(label, ImGuiDataType_S8, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_unsigned_char(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const UINT8 minValue = attributeList.GetMinValue<UINT8>();
				const UINT8 maxValue = attributeList.GetMaxValue<UINT8>();

				return ImGui::DragScalar(label, ImGuiDataType_U8, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_short(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const INT16 minValue = attributeList.GetMinValue<INT16>();
				const INT16 maxValue = attributeList.GetMaxValue<INT16>();

				return ImGui::DragScalar(label, ImGuiDataType_S16, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_unsigned_short(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const UINT16 minValue = attributeList.GetMinValue<UINT16>();
				const UINT16 maxValue = attributeList.GetMaxValue<UINT16>();

				return ImGui::DragScalar(label, ImGuiDataType_U16, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_int(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const INT32 minValue = attributeList.GetMinValue<INT32>();
				const INT32 maxValue = attributeList.GetMaxValue<INT32>();

				return ImGui::DragScalar(label, ImGuiDataType_S32, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_unsigned_int(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const UINT32 minValue = attributeList.GetMinValue<UINT32>();
				const UINT32 maxValue = attributeList.GetMaxValue<UINT32>();

				return ImGui::DragScalar(label, ImGuiDataType_U32, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_long_long(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const INT64 minValue = attributeList.GetMinValue<INT64>();
				const INT64 maxValue = attributeList.GetMaxValue<INT64>();

				return ImGui::DragScalar(label, ImGuiDataType_S64, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_unsigned_long_long(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const UINT64 minValue = attributeList.GetMinValue<UINT64>();
				const UINT64 maxValue = attributeList.GetMaxValue<UINT64>();

				return ImGui::DragScalar(label, ImGuiDataType_U64, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_float(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const float minValue = attributeList.GetMinValue<float>();
				const float maxValue = attributeList.GetMaxValue<float>();

				return ImGui::DragScalar(label, ImGuiDataType_Float, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_double(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				//ImGui::Text("%s : %lf", label, *static_cast<double*>(object));
				const double minValue = attributeList.GetMinValue<double>();
				const double maxValue = attributeList.GetMaxValue<double>();

				return ImGui::DragScalar(label, ImGuiDataType_Double, object, imgui::DEFULAT_DRAG_SPEED, &minValue, &maxValue, 0, GetImguiFlags(eImguiType::Drag, attributeList));
			}

			bool imguiWithReflection_math_Vector2(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const FLOAT32 minValue = attributeList.GetMinValue<FLOAT32>();
				const FLOAT32 maxValue = attributeList.GetMaxValue<FLOAT32>();

				return ImGui::DragFloat2
				(
					label,
					static_cast<math::Vector2*>(object)->data(),
					imgui::DEFULAT_DRAG_SPEED,
					minValue,
					maxValue,
					"%.3f",
					GetImguiFlags(eImguiType::Slider, attributeList)
				);

			}

			bool imguiWithReflection_math_Vector3(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				if (attributeList.GetIsHasGUIType("Color"))
				{
					return ImGui::ColorEdit3(label, static_cast<math::Vector3*>(object)->data(), GetImguiFlags(eImguiType::Color, attributeList));
				}
				else
				{
					const FLOAT32 minValue = attributeList.GetMinValue<FLOAT32>();
					const FLOAT32 maxValue = attributeList.GetMaxValue<FLOAT32>();

					return ImGui::DragFloat3
					(
						label,
						static_cast<math::Vector3*>(object)->data(),
						imgui::DEFULAT_DRAG_SPEED,
						minValue,
						maxValue,
						"%.3f",
						GetImguiFlags(eImguiType::Slider, attributeList)
					);
				}

			}

			bool imguiWithReflection_math_Vector4(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				if (attributeList.GetIsHasGUIType("Color"))
				{
					return ImGui::ColorEdit4(label, static_cast<math::Vector4*>(object)->data(), GetImguiFlags(eImguiType::Color, attributeList));
				}
				else
				{
					const FLOAT32 minValue = attributeList.GetMinValue<FLOAT32>();
					const FLOAT32 maxValue = attributeList.GetMaxValue<FLOAT32>();

					return ImGui::DragFloat4
					(
						label,
						static_cast<math::Vector4*>(object)->data(),
						imgui::DEFULAT_DRAG_SPEED,
						minValue,
						maxValue,
						"%.3f",
						GetImguiFlags(eImguiType::Slider, attributeList)
					);
				}
			}

			bool imguiWithReflection_math_Matrix2x2(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				return false;
			}

			bool imguiWithReflection_math_Matrix3x3(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				return false;
			}

			bool imguiWithReflection_math_Matrix4x4(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				return false;
			}

			
			bool imguiWithReflection_math_Quaternion(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				math::Vector3 eulerAngle{ math::Quaternion::QuaternionToEulerAngle(*static_cast<math::Quaternion*>(object)) };

				eulerAngle.x = math::RADIAN_TO_DEGREE * eulerAngle.x;
				eulerAngle.y = math::RADIAN_TO_DEGREE * eulerAngle.y;
				eulerAngle.z = math::RADIAN_TO_DEGREE * eulerAngle.z;

				const FLOAT32 minValue = attributeList.GetMinValue<FLOAT32>();
				const FLOAT32 maxValue = attributeList.GetMaxValue<FLOAT32>();

				const bool isValueChanged = ImGui::DragFloat3
				(
					label,
					eulerAngle.data(),
					imgui::DEFULAT_DRAG_SPEED,
					minValue,
					maxValue,
					"%.3f",
					GetImguiFlags(eImguiType::Slider, attributeList)
				);

				// TODO : FIX THIS. DOESN'T WORK WELL..

				if (isValueChanged == true)
				{
					eulerAngle.x = math::DEGREE_TO_RADIAN * eulerAngle.x;
					eulerAngle.y = math::DEGREE_TO_RADIAN * eulerAngle.y;
					eulerAngle.z = math::DEGREE_TO_RADIAN * eulerAngle.z;

					*static_cast<math::Quaternion*>(object) = eulerAngle;
				}

				return isValueChanged;
			}
			
			bool imguiWithReflection_dooms_Entity(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				dooms::Entity* const entity = static_cast<dooms::Entity*>(object);

				if (IsValid(entity) == true)
				{
					//ImGui::SetNextItemOpen(true);
					if (ImGui::TreeNode(entity->GetComponent<Transform>()->GetDObjectName().c_str()))
					{
						imguiWithReflectionHelper::DrawObjectGUI(entity->GetComponent<Transform>()->GetDClass(), entity->GetComponent<Transform>(), "");
						ImGui::TreePop();
					}

					const std::vector<dooms::Component*>& allComponents = entity->GetAllComponents();

					for (dooms::Component* component : allComponents)
					{
						if (component)
						{
							if(ImGui::TreeNode(component->GetDObjectName().c_str()))
							{
								imguiWithReflectionHelper::DrawObjectGUI(component->GetDClass(), component, "");
								ImGui::TreePop();
							}
						}
					}
				}

				return false;
			}

			bool imguiWithReflection_std_string(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				D_ASSERT(object != nullptr);

				std::string* stdString = static_cast<std::string*>(object);

				char charBuffer[100];

				if (stdString->empty() == false)
				{
					strncpy_s(charBuffer, stdString->c_str(), stdString->size() + 1);
				}
				else
				{
					charBuffer[0] = '\0';
				}


				bool isTextEdited = ImGui::InputText(label, charBuffer, sizeof(charBuffer) * sizeof(char), GetImguiFlags(eImguiType::InputText, attributeList));

				if (isTextEdited == true)
				{
					*stdString = charBuffer;
				}

				return isTextEdited;
			}

			/*
			bool imguiWithReflection_std_wstring(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				D_ASSERT(object != nullptr);

				std::wstring* stdString = static_cast<std::wstring*>(object);

				wchar_t wcharBuffer[50];

				if (stdString->empty() == false)
				{
					std::strncpy(wcharBuffer, stdString->c_str(), stdString->size() + 1);
				}
				else
				{
					wcharBuffer[0] = '\0';
				}


				bool isTextEdited = ImGui::inputt(label, wcharBuffer, sizeof(wcharBuffer) * sizeof(wchar_t), GetImguiFlags(eImguiType::InputText, attributeList));

				if (isTextEdited == true)
				{
					*stdString = wcharBuffer;
				}

				return isTextEdited;
			}
			*/

			bool imguiWithReflection_enum(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				const dooms::reflection::DEnum dEnum{ fieldDType->AsDEnum() };
				D_ASSERT(dEnum.IsValid() == true);

				const size_t enumConstantCount = dEnum.GetEnumConstantsCount();

				const size_t originalEnumConstantIndexOfCurrentValue = dEnum.GetEnumConstantIndex(*(INT32*)object);
				D_ASSERT(originalEnumConstantIndexOfCurrentValue >= 0);

				size_t currentEnumConstantIndexOfCurrentValue = originalEnumConstantIndexOfCurrentValue;

				if (ImGui::BeginCombo(label, dEnum.GetNameOfEnumConstantIndex(currentEnumConstantIndexOfCurrentValue), ImGuiComboFlags_NoArrowButton))
				{
					for (size_t enumConstantIndex = 0; enumConstantIndex < enumConstantCount; enumConstantIndex++)
					{
						imguiWithReflection::PushImgui();

						bool is_selected = (currentEnumConstantIndexOfCurrentValue == enumConstantIndex);

						if (ImGui::Selectable(dEnum.GetNameOfEnumConstantIndex(enumConstantIndex), is_selected))
						{
							currentEnumConstantIndexOfCurrentValue = enumConstantIndex;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}

						imguiWithReflection::PopImgui();
					}
					ImGui::EndCombo();
				}

				//ImGui::PopItemWidth();

				bool isValueChanged;
				if(originalEnumConstantIndexOfCurrentValue != currentEnumConstantIndexOfCurrentValue)
				{
					INT32 currentEnumConstantValue;

					const bool isSuccess = dEnum.GetEnumConstantValue(currentEnumConstantIndexOfCurrentValue, currentEnumConstantValue);
					D_ASSERT(isSuccess == true);

					*(INT32*)object = currentEnumConstantValue;

					isValueChanged = true;
				}
				else
				{
					isValueChanged = false;
				}


				return isValueChanged;
			}

			bool imguiWithReflection_std_array(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier,  const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				D_ASSERT(fieldDType != nullptr);

				const dooms::reflection::DTemplateType dTemplateType = fieldDType->AsDTemplateType();

				std::array<char, 1>* fakeArray = reinterpret_cast<std::array<char, 1>*>(object);

				const reflection::DTemplateArgumentType dTemplateTypeArgument = dTemplateType.GetTemplateArgumentType(0);

				D_ASSERT(dTemplateTypeArgument.IsValid() == true);

				const bool isArrayElementTypePointer = dTemplateTypeArgument.GetIsTemplateArgumentPointerType();
				
				const size_t vectorElementTypeSize = isArrayElementTypePointer == true ? sizeof(void*) : dTemplateTypeArgument.GetTypeSize();

				std::string_view typeFullNameStringView = typeFullName;

				const size_t lastTemplateArgumentComma = typeFullNameStringView.find_last_of(',');
				const size_t lastTemplateCloseBlancket = typeFullNameStringView.find_last_of('>');


				bool isVectorElementChanged = false;

				if(lastTemplateArgumentComma != std::string::npos && lastTemplateCloseBlancket != std::string::npos)
				{
					const std::string stdArraySizeTemplateArgumentString
					{
						typeFullNameStringView.begin() + lastTemplateArgumentComma + 1,
						typeFullNameStringView.begin() + lastTemplateCloseBlancket
					};

					char* pos = NULL;
					const size_t arrayElementRealCount = strtoll(stdArraySizeTemplateArgumentString.c_str(), &pos, 10);

					char elementString[] = "Index 10000";
					char elementIndexStringBuffer[5];

					
					if (ImGui::TreeNode(label))
					{
						for (size_t i = 0; i < arrayElementRealCount; i++)
						{
							const INT32 indexStringLength = sprintf_s(elementIndexStringBuffer, "%llu", i);
							D_ASSERT(indexStringLength != -1);
							std::memcpy(elementString + sizeof(elementString) - 6, elementIndexStringBuffer, indexStringLength + 1);

							bool isValueChange = false;

							const std::string arrayElementTypeFullName = dTemplateTypeArgument.GetTypeFullName();
							const reflection::eProperyQualifier array_element_qualifier = (isArrayElementTypePointer == true) ? reflection::eProperyQualifier::POINTER : reflection::eProperyQualifier::VALUE;

							dooms::ui::imguiWithReflectionHelper::DrawImguiFieldFromDField
							(
								fakeArray->data() + i * vectorElementTypeSize,
								elementString,
								arrayElementTypeFullName.c_str(),
								array_element_qualifier,
								&dTemplateTypeArgument,
								attributeList,
								isValueChange,
								nullptr,
								nullptr
							);
						}

						ImGui::TreePop();
					}

				}
				

				return isVectorElementChanged;
			}

			bool imguiWithReflection_std_vector(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier,  const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				D_ASSERT(fieldDType != nullptr);

				const dooms::reflection::DTemplateType dTemplateType = fieldDType->AsDTemplateType();
				
				std::vector<char>* fakeVector = reinterpret_cast<std::vector<char>*>(object);

				const reflection::DTemplateArgumentType dTemplateTypeArgument = dTemplateType.GetTemplateArgumentType(0);

				D_ASSERT(dTemplateTypeArgument.IsValid() == true);
				
				const bool isVectorElementTypePointer = dTemplateTypeArgument.GetIsTemplateArgumentPointerType();
				
				const size_t vectorElementTypeSize = isVectorElementTypePointer == true ? sizeof(void*) : dTemplateTypeArgument.GetTypeSize();
				
				const size_t vectorElementFakeCount = fakeVector->size(); // this size is ( endAddress - startAddress )
				// this can be applied only to msvc std::vector
				const size_t vectorElementRealCount = isVectorElementTypePointer == true ? (vectorElementFakeCount / sizeof(void*)) : (vectorElementFakeCount / vectorElementTypeSize); // this size is ( endAddress - startAddress ) / pointer size

				char elementString[] = "Index 10000";
				char elementIndexStringBuffer[5];

				bool isVectorElementChanged = false;

				if (ImGui::TreeNode(label))
				{
					for(size_t i = 0 ; i < vectorElementRealCount; i++)
					{
						const INT32 indexStringLength = sprintf_s(elementIndexStringBuffer, "%llu", i);
						D_ASSERT(indexStringLength != -1);
						std::memcpy(elementString + sizeof(elementString) - 6, elementIndexStringBuffer, indexStringLength + 1);

						bool isValueChange = false;

						const std::string vectorElementTypeFullName = dTemplateTypeArgument.GetTypeFullName();
						const reflection::eProperyQualifier vector_element_qualifier = (isVectorElementTypePointer == true) ? reflection::eProperyQualifier::POINTER : reflection::eProperyQualifier::VALUE;

						dooms::ui::imguiWithReflectionHelper::DrawImguiFieldFromDField
						(
							fakeVector->data() + i * vectorElementTypeSize,
							elementString,
							vectorElementTypeFullName.c_str(),
							vector_element_qualifier,
							&dTemplateTypeArgument,
							attributeList,
							isValueChange,
							nullptr,
							nullptr
						);

						isVectorElementChanged |= isValueChange;
					}

					if(attributeList.GetIsReadOnly() == false)
					{
						if (ImGui::Button("Add Item"))
						{
							fakeVector->resize(vectorElementFakeCount + vectorElementTypeSize);
							isVectorElementChanged = true;
						}

						if (ImGui::Button("Pop Back"))
						{
							if (vectorElementFakeCount >= vectorElementTypeSize)
							{
								fakeVector->resize(vectorElementFakeCount - vectorElementTypeSize);
								isVectorElementChanged = true;
							}
						}
					}

					
					
					ImGui::TreePop();
				}
				
				
				return isVectorElementChanged;
			}


			// Don't support std::unique_ptr.
			// because it can make a lot of bugs.
			/*bool imguiWithReflection_std_unique_ptr(void* const object, const char* const label, const char* const typeFullName, const reflection::eProperyQualifier dataQualifier, const reflection::DAttributeList& attributeList, const reflection::DType* const fieldDType)
			{
				D_ASSERT(fieldDType != nullptr);

				const dooms::reflection::DTemplateType dTemplateType = fieldDType->AsDTemplateType();

				std::unique_ptr<char>* fakeUniquePtr = reinterpret_cast<std::unique_ptr<char>*>(object);

				void* StoredObjectPointer = fakeUniquePtr->get();

				const std::string label = std::string(label) + " ( std::unique_ptr )";

				dooms::ui::imguiWithReflectionHelper::DrawImguiFieldFromDField
				(
					fakeUniquePtr,
					label,
					vectorElementTypeFullName.c_str(),
					vector_element_qualifier,
					&dTemplateTypeArgument,
					attributeList,
					isValueChange,
					nullptr,
					nullptr
				);
			}
			*/

		}
	}
}
void dooms::ui::imguiFieldFunctionGetter::Initialize()
{
	if (IsInitialized == false)
	{
		ImguiSpecialDrawFuncMap.emplace("math::Vector2", imguiWithReflection_math_Vector2);
		ImguiSpecialDrawFuncMap.emplace("math::Vector3", imguiWithReflection_math_Vector3);
		ImguiSpecialDrawFuncMap.emplace("math::Vector4", imguiWithReflection_math_Vector4);
		ImguiSpecialDrawFuncMap.emplace("math::Matrix3x3", imguiWithReflection_math_Matrix3x3);
		ImguiSpecialDrawFuncMap.emplace("math::Matrix4x4", imguiWithReflection_math_Matrix4x4);
		ImguiSpecialDrawFuncMap.emplace("math::Quaternion", imguiWithReflection_math_Quaternion);
		ImguiSpecialDrawFuncMap.emplace("bool", imguiWithReflection_bool);
		ImguiSpecialDrawFuncMap.emplace("char", imguiWithReflection_char);
		ImguiSpecialDrawFuncMap.emplace("unsigned char", imguiWithReflection_unsigned_char);
		ImguiSpecialDrawFuncMap.emplace("short", imguiWithReflection_short);
		ImguiSpecialDrawFuncMap.emplace("unsigned short", imguiWithReflection_unsigned_short);
		ImguiSpecialDrawFuncMap.emplace("int", imguiWithReflection_int);
		ImguiSpecialDrawFuncMap.emplace("unsigned int", imguiWithReflection_unsigned_int);
		ImguiSpecialDrawFuncMap.emplace("long long", imguiWithReflection_long_long);
		ImguiSpecialDrawFuncMap.emplace("unsigned long long", imguiWithReflection_unsigned_long_long);
		ImguiSpecialDrawFuncMap.emplace("float", imguiWithReflection_float);
		ImguiSpecialDrawFuncMap.emplace("double", imguiWithReflection_double);
		ImguiSpecialDrawFuncMap.emplace("dooms::Entity", imguiWithReflection_dooms_Entity);
		//ImguiSpecialDrawFuncMap.emplace("dooms::TransformCoreData", imguiWithReflection_TransformCoreData);
		ImguiSpecialDrawFuncMap.emplace("std::basic_string<char,std::char_traits<char>,std::allocator<char>>", imguiWithReflection_std_string);
		ImguiSpecialDrawFuncMap.emplace("std::vector", imguiWithReflection_std_vector);
		ImguiSpecialDrawFuncMap.emplace("std::array", imguiWithReflection_std_array);

		IsInitialized = true;
	}
}

dooms::ui::imguiFieldFunctionGetter::IMGUI_WITH_REFLECTION_FUNC dooms::ui::imguiFieldFunctionGetter::GetImguiWithReflectionFunction
(
	const char* const typeFullName,
	const reflection::eProperyQualifier valueQualifier,
	const reflection::DType* const fieldDType // Type full name of fieldDType can be different with typeFullName. because typeFullName can be end with '*' or '&' if it's pointer or reference
)
{
	D_ASSERT(typeFullName != nullptr);

	IMGUI_WITH_REFLECTION_FUNC func = nullptr;

	std::string_view typeFullNameStrView { typeFullName };

	auto iter = ImguiSpecialDrawFuncMap.find(typeFullNameStrView);
	if (iter != ImguiSpecialDrawFuncMap.end())
	{
		func = iter->second;
	}

	if (iter == ImguiSpecialDrawFuncMap.end())
	{
		const reflection::DPrimitive::ePrimitiveType primitiveTypeOfFieldType = fieldDType->GetPrimitiveType();

		if (primitiveTypeOfFieldType == reflection::DPrimitive::ePrimitiveType::TEMPLATE_TYPE)
		{
			// if fail to find and field type is template type, check again with template type name ( not full name )
			std::string templateTypeName;
			const bool isSuccess = dooms::reflection::dTemplateTypeHelper::GetTemplateTypeNameFromTypeFullName(typeFullName, templateTypeName);
			D_ASSERT(isSuccess == true);
			
			iter = ImguiSpecialDrawFuncMap.find(std::string_view{ templateTypeName });
			if (iter != ImguiSpecialDrawFuncMap.end())
			{
				func = iter->second;
			}
		}
		else if (primitiveTypeOfFieldType == reflection::DPrimitive::ePrimitiveType::ENUM)
		{
			func = imguiWithReflection_enum;
		}

	}

	return func;
}
