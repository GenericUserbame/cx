/** parse Directives
 * parse_directive.cpp
 *
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include "common.h"
#include "buffer.h"
#include "error.h"
#include "parser.h"

#include "cx_api.h"

#ifdef _WIN32
#include <windows.h>

void load_windows_lib(const char *lib, cx_symtab_node *p_function_id){
	typedef cx_symbols (*lib_init)(void);

	cx_symbols *loaded_symbols = nullptr;
	extern cx_datatype cx_dt;
	extern cx_symtab cx_global_symtab;

	HINSTANCE hinst_lib;
	lib_init init_call;
	BOOL f_free_result, f_runtime_linksuccess = FALSE;

	std::string lib_path;
	char *env_path = getenv(__CX_STDLIB__);

	if (env_path != nullptr) {
		lib_path = env_path;
		lib_path += "\\";
	}

	std::string dll = std::string(lib) + ".dll";

	lib_path += dll;

	// Get a handle to the DLL module.
	hinst_lib = LoadLibrary(dll.c_str());

	// If the handle is valid, try to get the function address.
	if (hinst_lib != NULL)
	{
		init_call = (lib_init)GetProcAddress(hinst_lib, "cx_lib_init");

		// If the function address is valid, call the function.
		if (NULL != init_call)
		{
			f_runtime_linksuccess = TRUE;
			loaded_symbols = new cx_symbols((init_call)());
		}
		// Free the DLL module.

		f_free_result = FreeLibrary(hinst_lib);
	}

	if (loaded_symbols != nullptr){
		//for (cx_lib &lib : loaded_symbols){
		std::cout << loaded_symbols->at(0).name << std::endl;
		//}
	}
}

#endif

/** parse_execute_directive      Opens an external script module
 *                      for parsing.
 *
 *      #include <identifier>
 *
 * @param p_function_id : ptr to the routine owning this directive call.
 */
void cx_parser::parse_execute_directive (cx_symtab_node *p_function_id) {

    extern cx_symtab_node *p_program_ptr_id;

    switch (token) {
        case tc_INCLUDE:
        {
            std::string lib_path;
            char *env_path = getenv(__CX_STDLIB__);

            if (env_path != nullptr) {
                lib_path = env_path;
                lib_path += "/";
            }

            get_token();

            lib_path += p_token->string__();
            p_program_ptr_id->found_global_end = true;

            // save current line number
            int old_line_num = current_line_number;
            current_line_number = 0;

            /* true : stdlib module
             * returns nullptr */
            cx_parser *parser = new cx_parser
                    (new cx_source_buffer(lib_path.c_str()), true);

            parser->parse();

            current_line_number = old_line_num;

            delete parser;

            icode.reset();
            icode.put(tc_left_bracket);
            p_program_ptr_id->found_global_end = false;
            get_token_append();
        }
            break;
		case tc_WARN:
			get_token();
			
			if (token != tc_string){
				cx_error(err_unexpected_token);
			}

			{
				std::string msg = p_token->string__();
				msg[0] = ' '; msg[msg.length() - 1] = '\0';
				std::cerr << "warning:" << msg << std::endl;
			}

			get_token_append();
			break;
		case tc_IMPORT:
			get_token();

#ifdef _WIN32
			load_windows_lib(p_token->string__(), p_function_id);
#else

#endif
			break;
        default:
            break;
    }
}
