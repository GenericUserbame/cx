#include <cstdio>
#include "buffer.h"
#include "error.h"
#include "types.h"
#include "symtab.h"

namespace cx{

	cx_type::type_ptr p_boolean_type;
	cx_type::type_ptr p_char_type;
	cx_type::type_ptr p_byte_type;
//	cx_type::type_ptr p_wchar_type;
	cx_type::type_ptr p_integer_type;
	cx_type::type_ptr p_double_type;
	cx_type::type_ptr p_reference_type;
	cx_type::type_ptr p_void_type;
	cx_type::type_ptr p_dummy_type;

	cx_type::cx_type(type_form form, type_code type)
		: typeform(form), typecode(type){}

	/** Constructors    General.
	 *
	 * @param fc  : form code.
	 * @param s   : byte size of type.
	 * @param p_id : ptr to symbol table node of type identifier.
	 */
	cx_type::cx_type(type_form form, type_code type, size_t size, symbol_table_node_ptr &p_id, symbol_table_ptr &p_members)
		: typeform(form), typecode(type), size(size), p_type_id(p_id) {

		switch (typeform) {
		case F_ARRAY:
			this->array.element_count = 0;
			this->array.min_index = 0;
			this->array.p_index_type = p_integer_type;
			break;
		default:
			break;
		}

		this->complex.p_class_scope = p_members;
	}

	/** Destructor      Delete the allocated objects according to
	 *                  the form code.  Note that the objects
	 *                  pointed to by enumeration.p_const_ids and by
	 *                  subrange.p_base_type are deleted along with
	 *                  the symbol tables that contain their
	 *                  identifiers.
	 */
	cx_type::~cx_type() {}

	/** initialize_builtin_types   Initialize the predefined
	 *                              types by entering their
	 *                              identifiers into the symbol
	 *                              table.
	 *
	 * @param p_symtab : ptr to symbol table.
	 */
	void initialize_builtin_types(symbol_table_ptr &p_symtab) {

		symbol_table_node_ptr p_integer_id = p_symtab->enter(L"int", DC_TYPE);
		symbol_table_node_ptr p_byte_id = p_symtab->enter(L"byte", DC_TYPE);
		symbol_table_node_ptr p_double_id = p_symtab->enter(L"real", DC_TYPE);
		symbol_table_node_ptr p_reference_id = p_symtab->enter(L"class", DC_TYPE);
		symbol_table_node_ptr p_boolean_id = p_symtab->enter(L"bool", DC_TYPE);
		symbol_table_node_ptr p_char_id = p_symtab->enter(L"char", DC_TYPE);
		//symbol_table_node_ptr p_wchar_id = p_symtab->enter(L"wchar", DC_TYPE);
		symbol_table_node_ptr p_false_id = p_symtab->enter(L"false", DC_CONSTANT);
		symbol_table_node_ptr p_true_id = p_symtab->enter(L"true", DC_CONSTANT);
		symbol_table_node_ptr p_void_id = p_symtab->enter(L"void", DC_TYPE);

		// only used for functions with no return value
		if (p_void_type == nullptr) {
			p_void_type = std::make_shared<cx_type>(F_SCALAR, T_VOID, 0, p_void_id, p_std_type_members);
		}

		if (p_integer_type == nullptr) {
			p_integer_type = std::make_shared<cx_type>(F_SCALAR, T_INT, sizeof(cx_int), p_integer_id, p_std_type_members);
		}

		if (p_byte_type == nullptr) {
			p_byte_type = std::make_shared<cx_type>(F_SCALAR, T_BYTE, sizeof(uint8_t), p_byte_id, p_std_type_members);
		}

		if (p_double_type == nullptr) {
			p_double_type = std::make_shared<cx_type>(F_SCALAR, T_DOUBLE, sizeof(cx_real), p_double_id, p_std_type_members);
		}

		if (p_boolean_type == nullptr) {
			p_boolean_type = std::make_shared<cx_type>(F_SCALAR, T_BOOLEAN, sizeof(bool), p_boolean_id, p_std_type_members);
		}

		if (p_char_type == nullptr) {
			p_char_type = std::make_shared<cx_type>(F_SCALAR, T_CHAR, sizeof(wchar_t), p_char_id, p_std_type_members);
		}

		/*if (p_wchar_type == nullptr) {
			p_wchar_type = std::make_shared<cx_type>(F_SCALAR, T_WCHAR, sizeof(wchar_t), p_wchar_id, p_std_type_members);
		}*/

		if (p_reference_type == nullptr) {
			p_reference_type = std::make_shared<cx_type>(F_REFERENCE, T_REFERENCE, sizeof(uintptr_t), p_reference_id, p_std_type_members);
		}

		// Link each predefined type id's node to it's type object
		p_integer_id->p_type = p_integer_type;
		p_byte_id->p_type = p_byte_type;
		p_double_id->p_type = p_double_type;
		p_reference_id->p_type = p_reference_type;
		p_boolean_id->p_type = p_boolean_type;
		p_char_id->p_type = p_char_type;
//		p_wchar_id->p_type = p_wchar_type;
		p_false_id->p_type = p_boolean_type;
		p_true_id->p_type = p_boolean_type;
		p_void_id->p_type = p_reference_type;


		p_boolean_type->enumeration.max = 1;
		p_boolean_type->enumeration.p_const_ids = p_false_id;

		p_false_id->defined.constant.value.z_ = false;
		p_true_id->defined.constant.value.z_ = true;
		
		// initialize standard type members
		//init_std_members();

		if (p_dummy_type == nullptr){
			p_dummy_type = std::make_shared<cx_type>(F_NONE, T_DUMMY);
		}
	}


	/************************
	 *                      *
	 *  type Compatibility  *
	 *                      *
	 ************************/

	/** check_relational_op_operands  Check that the types of the two operands
	 *                      of a relational operator are compatible.
	 *                      Flag an incompatible type error if not.
	 *
	 * @param p_type1 : ptr to the first  operand's type object.
	 * @param p_type2 : ptr to the second operand's type object.
	 */
	void check_relational_op_operands(symbol_table_node_ptr &p_function_id, const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		check_assignment_type_compatible(p_function_id, p_type1, p_type2, ERR_INCOMPATIBLE_TYPES);
	}

	/** check_integer_or_real  Check that the type of each operand is
	 *                      either integer or real.  Flag an
	 *                      incompatible type error if not.
	 *
	 * @param p_type1 : ptr to the first  operand's type object.
	 * @param p_type2 : ptr to the second operand's type object or nullptr.
	 */
	void check_integer_or_real(const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		///p_type1 = std::make_shared<cx_type>(p_type1->base_type());

		if ((p_type1 != p_integer_type) && (p_type1 != p_double_type)) {
			cx_error(ERR_INCOMPATIBLE_TYPES);
		}

		if (p_type2 != nullptr) {
			//p_type2 = p_type2->base_type();

			if ((p_type2 != p_integer_type) && (p_type2 != p_double_type)) {
				cx_error(ERR_INCOMPATIBLE_TYPES);
			}
		}
	}

	void check_bitwise_integer(const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		type_code target_type = p_type1->base_type()->typecode;
		type_code value_type = p_type2->base_type()->typecode;

		switch (target_type) {
		case T_INT:
		case T_CHAR:
//		case T_WCHAR:
		case T_BYTE:
			switch (value_type) {
			case T_INT:
			case T_CHAR:
//			case T_WCHAR:
			case T_BYTE:
				return;
				break;
			default:break;
			}
			break;
		default:
			break;
		}

		cx_error(ERR_INCOMPATIBLE_TYPES);
	}

	// TODO add additional type checks
	void check_bitwise_integer(const cx_type::type_ptr p_type1) {
		type_code target_type = p_type1->base_type()->typecode;

		switch (target_type) {
		case T_INT:
		case T_CHAR:
//		case T_WCHAR:
		case T_BYTE:
			return;
			break;
		default:
			break;
		}

		cx_error(ERR_INCOMPATIBLE_TYPES);
	}

	/** check_boolean        Check that the type of each operand is
	 *                      boolean.  Flag an incompatible type
	 *                      error if not.
	 *
	 * @param p_type1 : ptr to the first  operand's type object.
	 * @param p_type2 : ptr to the second operand's type object or nullptr.
	 */
	void check_boolean(const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		/*if ((p_type1->base_type() != p_boolean_type.get())
			|| ((p_type2 != nullptr) && (p_type2->base_type() != p_boolean_type.get()))) {
			cx_error(ERR_INCOMPATIBLE_TYPES);
		}*/
	}

	/** check_assignment_type_compatible   Check that a value's type is
	 *                                  assignment compatible with
	 *                                  the target's type.  Flag an
	 *                                  error if not.
	 *
	 * @param p_target_type : ptr to the target's type object.
	 * @param p_value_type  : ptr to the value's  type object.
	 * @param ec          : error code.
	 */
	void check_assignment_type_compatible(symbol_table_node_ptr &p_function_id, const cx_type::type_ptr p_target_type,
		const cx_type::type_ptr p_value_type, error_code ec) {

		type_code target_type = p_target_type->base_type()->typecode;
		type_code value_type = p_value_type->base_type()->typecode;

		// If same type return
		if (target_type == value_type) return;

		if (p_target_type->is_scalar_type()) {
			switch (target_type) {
			case T_BYTE:
				switch (value_type) {
				case T_INT:					
				case T_BOOLEAN:
					p_function_id->defined.routine.program_code.push_back({ I2B });
					return;
					break;
				case T_DOUBLE:
					p_function_id->defined.routine.program_code.push_back({ D2I });
					p_function_id->defined.routine.program_code.push_back({ I2B });
					return;
					break;
				case T_CHAR:
					p_function_id->defined.routine.program_code.push_back({ C2I });
					p_function_id->defined.routine.program_code.push_back({ I2B });
					return;
					break;
				default:
					break;
				}
				break;
			case T_INT:
				switch (value_type) {
				case T_BOOLEAN:
				case T_BYTE:
					return;
					break;
				case T_DOUBLE:
					p_function_id->defined.routine.program_code.push_back({ D2I });
					return;
					break;
				case T_CHAR:
					p_function_id->defined.routine.program_code.push_back({ C2I });
					return;
				default:
					break;
				}
				break;
			case T_CHAR:
				switch (value_type) {
				case T_INT:
				case T_BYTE:
					p_function_id->defined.routine.program_code.push_back({ I2C });
					return;
					return;
					break;
				case T_DOUBLE:
					p_function_id->defined.routine.program_code.push_back({ D2I });
					p_function_id->defined.routine.program_code.push_back({ I2C });
					return;
					break;
				default:
					break;
				}
				break;
			case T_DOUBLE:
				switch (value_type) {
				case T_INT:
				case T_BYTE:
					p_function_id->defined.routine.program_code.push_back({ I2D });
					return;
					break;
				case T_CHAR:
					p_function_id->defined.routine.program_code.push_back({ C2I });
					p_function_id->defined.routine.program_code.push_back({ I2D });
					return;
					break;
				default:
					break;
				}
				break;
			case T_BOOLEAN:
				switch (value_type) {
				case T_INT:
				case T_BYTE:
					return;
					break;
				case T_DOUBLE:
					p_function_id->defined.routine.program_code.push_back({ D2I });
					return;
					break;
				case T_CHAR:
					p_function_id->defined.routine.program_code.push_back({ C2I });
					return;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		else if (p_target_type->typeform == F_ARRAY) {
			//        if (p_target_type->base_type() ==
			//            p_value_type->array.p_element_type->type_code) return;
			return;
		}

		cx_error(ec);
	}

	/** integer_operands     Check that the types of both operands
	 *                      are integer.
	 *
	 * @param p_type1 : ptr to the first  operand's type object.
	 * @param p_type2 : ptr to the second operand's type object.
	 * @return true if yes, false if no.
	 */
	bool integer_operands(const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		
		type_code target_type = p_type1->base_type()->typecode;
		type_code value_type = p_type2->base_type()->typecode;

		switch (target_type) {
		case T_BYTE:
			switch (value_type) {
			case T_INT:
			case T_CHAR:
//			case T_WCHAR:
			case T_BOOLEAN:
			case T_DOUBLE:
			case T_BYTE:
				return true;
				break;
			default:
				break;
			}
			break;
		case T_INT:
			switch (value_type) {
			case T_INT:
			case T_CHAR:
//			case T_WCHAR:
			case T_BOOLEAN:
			case T_DOUBLE:
			case T_BYTE:
				return true;
				break;
			default:
				break;
			}
			break;
		case T_CHAR:
			switch (value_type) {
			case T_INT:
			case T_CHAR:
//			case T_WCHAR:
			case T_BOOLEAN:
			case T_DOUBLE:
			case T_BYTE:
				return true;
				break;
			default:
				break;
			}
			break;
		case T_BOOLEAN:
			switch (value_type) {
			case T_INT:
			case T_CHAR:
//			case T_WCHAR:
			case T_BOOLEAN:
			case T_DOUBLE:
			case T_BYTE:
				return true;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		return false;
	}

	/** real_operands        Check that the types of both operands
	 *                      are real, or that one is real and the
	 *                      other is integer.
	 *
	 * @param p_type1 : ptr to the first  operand's type object.
	 * @param p_type2 : ptr to the second operand's type object.
	 * @return true if yes, false if no.
	 */
	bool real_operands(const cx_type::type_ptr p_type1, const cx_type::type_ptr p_type2) {
		const cx_type *p_type_a = p_type1.get()->base_type();
		const cx_type *p_type_b = p_type2.get()->base_type();

		return (((p_type_a == p_double_type.get()) && (p_type_b == p_double_type.get()))
			|| ((p_type_a == p_double_type.get()) && (p_type_b == p_integer_type.get()))
			|| ((p_type_b == p_double_type.get()) && (p_type_a == p_integer_type.get())));

	}
}