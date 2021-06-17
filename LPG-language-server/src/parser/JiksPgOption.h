#pragma once
#include <tuple.h>
#include "../code.h"
#include "../ASTUtils.h"
class JiksPgOption : public Code, public Util
{


    public:
	JiksPgOption();

        enum
        {
            //
            // Possible values for option "names"
            //
            MINIMUM = 1,
            MAXIMUM = 2,
            OPTIMIZED = 3,

            //
            // Possible values for rule_classnames
            //
            SEQUENTIAL = 1,
            STABLE = 2,

            //
            // Possible values for option "programming_language"
            //
            NONE = 0,
            XML = 1,
            C = 2,
            CPP = 3,
            JAVA = 4,
            PLX = 5,
            PLXASM = 6,
            ML = 7,
            CPP2 = 8,
            //
            // Possible values for option "trace"
            //
            // NONE = 0,
            //
            CONFLICTS = 1,
            FULL = 2,

            //
            // Possible values for option "automatic_ast"
            //
            // NONE = 0,
            //
            NESTED = 1,
            TOPLEVEL = 2,

            //
            // Possible values for option "variables"
            //
            // NONE = 0,
            //
            BOTH = 1,
            NON_TERMINALS = 2,
            TERMINALS = 3,

            //
            // Possible values for option "visitor"
            //
            // NONE = 0,
            //
            DEFAULT = 1,
            PREORDER = 2
        };

        int return_code;

        const char* home_directory;
        Tuple<const char*> include_search_directory,
                           template_search_directory,
                           filter_file,
                           import_file;

        const char* template_directory,
            * ast_directory_prefix;

        bool attributes,
            backtrack,
            legacy,
            list,
            glr,
            slr,
            verbose,
            first,
            follow,
            priority,
            edit,
            states,
            xref,
            nt_check,
            conflicts,
            read_reduce,
            remap_terminals,
            goto_default,
            shift_default,
            byte,
            warnings,
            single_productions,
            error_maps,
            debug,
            parent_saved,
            precedence,
            scopes,
            serialize,
            soft_keywords,
            table;

        bool for_parser;
        int lalr_level,
            margin,
            max_cases,
            names,
            rule_classnames,
            trace,
            programming_language,
            automatic_ast,
            variables,
            visitor;

        char escape,
            or_marker;

        const char* factory,
            * file_prefix,

            * grm_file,
            * lis_file,
            * tab_file,

            * dat_directory,
            * dat_file,
            * dcl_file,
            * def_file,
            * directory_prefix,
            * prs_file,
            * sym_file,
            * top_level_ast_file,
            * imp_file,
            * exp_file,
            * exp_prefix,
            * exp_suffix,

            * out_directory,
            * ast_directory,
            * ast_package;

        //private:
        const char* ast_type;
        //public:
        const char* exp_type,
            * prs_type,
            * sym_type,
            * top_level_ast_file_prefix,
            * dcl_type,
            * imp_type,
            * def_type,
            * action_type,
            * visitor_type,

            * filter,
            * import_terminals,
            * include_directory,
            * template_name,
            * extends_parsetable,
            * parsetable_interfaces,
            * package,
            * prefix,
            * suffix;

        bool quiet;

        void EmitHeader(IToken*, const char*);
        void EmitHeader(IToken*, IToken*, const char*);
        void Emit(IToken*, const char*, const char*);
        void Emit(IToken*, const char*, Tuple<const char*>&);
        void Emit(IToken*, IToken*, const char*, const char*);
        void Emit(IToken*, IToken*, const char*, Tuple<const char*>&);
        void EmitError(int, const char*);
        void EmitError(int, Tuple<const char*>&);
        void EmitWarning(int, const char*);
        void EmitWarning(int, Tuple<const char*>&);
        void EmitInformative(int, const char*);
        void EmitInformative(int, Tuple<const char*>&);

        void EmitError(IToken* token, const char* msg) { Emit(token, "Error: ", msg); return_code = 12; }
        void EmitError(IToken* token, Tuple<const char*>& msg) { Emit(token, "Error: ", msg); return_code = 12; }
        void EmitWarning(IToken* token, const char* msg) { Emit(token, "Warning: ", msg); }
        void EmitWarning(IToken* token, Tuple<const char*>& msg) { Emit(token, "Warning: ", msg); }
        void EmitInformative(IToken* token, const char* msg) { Emit(token, "Informative: ", msg); }
        void EmitInformative(IToken* token, Tuple<const char*>& msg) { Emit(token, "Informative: ", msg); }

        void EmitError(IToken* startToken, IToken* endToken, const char* msg) { Emit(startToken, endToken, "Error: ", msg); return_code = 12; }
        void EmitError(IToken* startToken, IToken* endToken, Tuple<const char*>& msg) { Emit(startToken, endToken, "Error: ", msg); return_code = 12; }
        void EmitWarning(IToken* startToken, IToken* endToken, const char* msg) { Emit(startToken, endToken, "Warning: ", msg); }
        void EmitWarning(IToken* startToken, IToken* endToken, Tuple<const char*>& msg) { Emit(startToken, endToken, "Warning: ", msg); }
        void EmitInformative(IToken* startToken, IToken* endToken, const char* msg) { Emit(startToken, endToken, "Informative: ", msg); }
        void EmitInformative(IToken* startToken, IToken* endToken, Tuple<const char*>& msg) { Emit(startToken, endToken, "Informative: ", msg); }
};