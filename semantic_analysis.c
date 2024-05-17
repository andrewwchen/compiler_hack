#include"ast.h"
#include<stdio.h>
#include<cstddef>
#include <string.h>
#include<vector>
#include <algorithm>
using namespace std;

int vector_contains(vector<char*> *v, char *s, int start_from = 0) {
    for (char* s2: *v) {
        if (start_from <= 0 and strcmp(s, s2) == 0) return 1;
        else start_from -= 1;
    }
    return 0;
}

int perform_analysis_on_val(astNode *node, char *func_param, vector<char*> *decl_list) {
	switch(node->type){
		case ast_stmt: {
            astStmt stmt = node->stmt;
            if (stmt.type != ast_call) {
                printf("val is a stmt but not a call\n");
                return -1;
            }
            if (strcmp(stmt.call.name, "read") != 0) {
                printf("val is a stmt call but not a read call\n");
                return -1;
            }
            return 0;
		}
		case ast_var: {
            char *var_name = node->var.name;
            if (vector_contains(decl_list, var_name) == 0 and strcmp(func_param, var_name) != 0) {
                printf("%s referenced before declaration\n", var_name);
                return -1;
            }
            return 0;
		}
		case ast_cnst:
            return 0;
		case ast_uexpr:
            return perform_analysis_on_val(node->uexpr.expr, func_param, decl_list);
		case ast_bexpr: {
            if (perform_analysis_on_val(node->bexpr.lhs, func_param, decl_list) == -1) return -1;
            if (perform_analysis_on_val(node->bexpr.rhs, func_param, decl_list) == -1) return -1;
            return 0;
		}
		default: {
            printf("val has incorrect type (not read, var, cnst, uexpr, or bexpr)\n");
			return -1;
		}
	}
}

int perform_analysis_on_stmt(astNode *node, char *func_param, vector<char*> *decl_list, vector<int> *decl_list_scope_start_sizes) {
    astStmt stmt = node->stmt; 
	switch(stmt.type){
		case ast_call: {
            if (stmt.call.param != NULL) return perform_analysis_on_val(stmt.call.param, func_param, decl_list);
            return 0;
        }
		case ast_ret:
            return perform_analysis_on_val(stmt.ret.expr, func_param, decl_list);
		case ast_asgn:	{
            if (perform_analysis_on_val(stmt.asgn.lhs, func_param, decl_list) == -1) return -1;
            if (perform_analysis_on_val(stmt.asgn.rhs, func_param, decl_list) == -1) return -1;
            return 0;
        }
		case ast_decl:	{
            if (vector_contains(decl_list, stmt.decl.name, decl_list_scope_start_sizes->back()) == 1) {
                printf("multiple declaration of %s\n", stmt.decl.name);
                return -1;
            }
            if (strcmp(func_param, stmt.decl.name) == 0) {
                printf("%s is already a param\n", stmt.decl.name);
                return -1;
            }
            decl_list->push_back(stmt.decl.name);
            return 0;
        }
		case ast_block: {
            decl_list_scope_start_sizes->push_back(decl_list->size());
            vector<astNode*> slist = *(stmt.block.stmt_list);
			vector<astNode*>::iterator it = slist.begin();
            while (it != slist.end()){
                if (perform_analysis_on_stmt(*it, func_param, decl_list, decl_list_scope_start_sizes) == -1) return -1;
                it++;
            }
            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            return 0;
        }
		case ast_while: {
            if (perform_analysis_on_val(stmt.whilen.cond->rexpr.lhs, func_param, decl_list) == -1) return -1;
            if (perform_analysis_on_val(stmt.whilen.cond->rexpr.rhs, func_param, decl_list) == -1) return -1;
            decl_list_scope_start_sizes->push_back(decl_list->size());
            if (perform_analysis_on_stmt(stmt.whilen.body, func_param, decl_list, decl_list_scope_start_sizes) == -1) return -1;
            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            return 0;
		}
		case ast_if: {
            if (perform_analysis_on_val(stmt.ifn.cond->rexpr.lhs, func_param, decl_list) == -1) return -1;
            if (perform_analysis_on_val(stmt.ifn.cond->rexpr.rhs, func_param, decl_list) == -1) return -1;
            decl_list_scope_start_sizes->push_back(decl_list->size());
            if (perform_analysis_on_stmt(stmt.ifn.if_body, func_param, decl_list, decl_list_scope_start_sizes) == -1) return -1;
            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            if (stmt.ifn.else_body != NULL) {
                decl_list_scope_start_sizes->push_back(decl_list->size());
                if (perform_analysis_on_stmt(stmt.ifn.else_body, func_param, decl_list, decl_list_scope_start_sizes) == -1) return -1;
                int scope_start_size = decl_list_scope_start_sizes->back();
                decl_list_scope_start_sizes->pop_back();
                decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            }
            return 0;
		}
        default: {
            printf("stmt has incorrect type\n");
            return -1;
        }
    }
}

// return 0 if success, 1 if failure
// prints error details
int perform_analysis(astNode *rootAst) {
	vector<char*> *decl_list;
	vector<int> *decl_list_scope_start_sizes;
	decl_list = new vector<char*> ();
	decl_list_scope_start_sizes = new vector<int> ();
    int result;
    astFunc func = rootAst->prog.func->func;
    if (func.param == NULL) {
        result = perform_analysis_on_stmt(func.body, NULL, decl_list, decl_list_scope_start_sizes);
    } else {
        result = perform_analysis_on_stmt(func.body, func.param->var.name, decl_list, decl_list_scope_start_sizes);
    }
	delete(decl_list);
	delete(decl_list_scope_start_sizes);
    return result;
}

