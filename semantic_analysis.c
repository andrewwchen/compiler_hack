#include"ast.h"
#include<stdio.h>
#include<cstddef>
#include <string.h>
#include<vector>
#include <algorithm>
using namespace std;

int vector_contains(vector<char*> *v, char *s, int start_from = 0) {
    for (char* s2: *v) {
        if (start_from <= 0) {
            if (strcmp(s, s2) == 0) {
                return 1;
            }
        } else {
            start_from -= 1;
        }
    }
    return 0;
}

int perform_analysis_on_val(astNode *node, vector<char*> *decl_list) {
	switch(node->type){
		case ast_stmt: {
            if (node->stmt.type != ast_call) {
                printf("val is a stmt but not a call\n");
                return -1;
            }
            if (strcmp(node->stmt.call.name, "read") != 0) {
                printf("val is a stmt call but not a read call\n");
                return -1;
            }
            return 0;
		}
		case ast_var: {       
            if (vector_contains(decl_list, node->var.name) == 0) {
                printf("%s referenced before declaration\n", node->var.name);
                return -1;
            }
            return 0;
					  }
		case ast_cnst: {
            return 0;
		}
		case ast_uexpr: {
            if (node->uexpr.expr->type == ast_var) {
                if (vector_contains(decl_list, node->uexpr.expr->var.name) == 0) {
                    printf("%s referenced before declaration\n", node->uexpr.expr->var.name);
                    return -1;
                }
            }
            return 0;
		}
		case ast_bexpr: {
            if (node->bexpr.lhs->type == ast_var) {
                if (vector_contains(decl_list, node->bexpr.lhs->var.name) == 0) {
                    printf("%s referenced before declaration\n", node->bexpr.lhs->var.name);
                    return -1;
                }
            }
            if (node->bexpr.rhs->type == ast_var) {
                if (vector_contains(decl_list, node->bexpr.rhs->var.name) == 0) {
                    printf("%s referenced before declaration\n", node->bexpr.rhs->var.name);
                    return -1;
                }
            }
            return 0;
		}
		default: {
            printf("val has incorrect type (not read, var, cnst, or bexpr)\n");
			return -1;
		}
	}
}

int perform_analysis_on_stmt(astNode *node, vector<char*> *decl_list, vector<int> *decl_list_scope_start_sizes) {
	switch(node->stmt.type){
		case ast_call: {
            if (node->stmt.call.param != NULL) {
                return perform_analysis_on_val(node->stmt.call.param, decl_list);
            }
            return 0;
        }
		case ast_ret: {
            return perform_analysis_on_val(node->stmt.ret.expr, decl_list);
        }
		case ast_asgn:	{
            if (perform_analysis_on_val(node->stmt.asgn.lhs, decl_list) == -1) return -1;
            if (perform_analysis_on_val(node->stmt.asgn.rhs, decl_list) == -1) return -1;
            return 0;
        }
		case ast_decl:	{
            if (vector_contains(decl_list, node->stmt.decl.name, decl_list_scope_start_sizes->back()) == 1) {
                printf("multiple declaration of %s\n", node->stmt.decl.name);
                return -1;
            }
            decl_list->push_back(node->stmt.decl.name);
            return 0;
        }
		case ast_block: {
            decl_list_scope_start_sizes->push_back(decl_list->size());

            vector<astNode*> slist = *(node->stmt.block.stmt_list);
			vector<astNode*>::iterator it = slist.begin();
            while (it != slist.end()){
                if (perform_analysis_on_stmt(*it, decl_list, decl_list_scope_start_sizes) == -1) return -1;
                it++;
            }

            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            
            return 0;
        }
		case ast_while: {
            if (perform_analysis_on_val(node->stmt.whilen.cond->rexpr.lhs, decl_list) == -1) return -1;
            if (perform_analysis_on_val(node->stmt.whilen.cond->rexpr.rhs, decl_list) == -1) return -1;

            decl_list_scope_start_sizes->push_back(decl_list->size());

            if (perform_analysis_on_stmt(node->stmt.whilen.body, decl_list, decl_list_scope_start_sizes) == -1) return -1;

            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());
            
            return 0;
		}
		case ast_if: {
            if (perform_analysis_on_val(node->stmt.ifn.cond->rexpr.lhs, decl_list) == -1) return -1;
            if (perform_analysis_on_val(node->stmt.ifn.cond->rexpr.rhs, decl_list) == -1) return -1;
            
            decl_list_scope_start_sizes->push_back(decl_list->size());

            if (perform_analysis_on_stmt(node->stmt.ifn.if_body, decl_list, decl_list_scope_start_sizes) == -1) return -1;

            int scope_start_size = decl_list_scope_start_sizes->back();
            decl_list_scope_start_sizes->pop_back();
            decl_list->erase(decl_list->begin()+scope_start_size, decl_list->end());

            if (node->stmt.ifn.else_body != NULL) {

                decl_list_scope_start_sizes->push_back(decl_list->size());

                if (perform_analysis_on_stmt(node->stmt.ifn.else_body, decl_list, decl_list_scope_start_sizes) == -1) return -1;

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

    if (rootAst->prog.func->func.param != NULL) {
        decl_list->push_back(rootAst->prog.func->func.param->var.name);
    }; 
    int result = perform_analysis_on_stmt(rootAst->prog.func->func.body, decl_list, decl_list_scope_start_sizes);

	delete(decl_list);
	delete(decl_list_scope_start_sizes);
    return result;
}

