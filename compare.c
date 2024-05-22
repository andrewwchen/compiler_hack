#include"ast.h"
#include<stdio.h>
#include<cstddef>

#include <unordered_map> 
#include <cstring>
#include <string>
#include<vector>
#include <iostream>
#include <cstdarg>
#include <fstream>
#include <memory>
#include <cstdio>
#include <regex>
#include<algorithm>


using namespace std;

string get_tree_string(astNode *node, unordered_map<string, int> *nodeFrequencies) {
	switch(node->type){
		case ast_func: {
    		astFunc func = node->func;
            string func_tree = "{func";
            ++(*nodeFrequencies)[func_tree];
            func_tree.append("{");
            func_tree.append(get_tree_string(func.body, nodeFrequencies));
            func_tree.append("}");
			if (func.param != NULL) {
                func_tree.append("{");
                func_tree.append(get_tree_string(func.param, nodeFrequencies));
                func_tree.append("}");
			}
            func_tree.append("}");
			return func_tree;
		}
		case ast_stmt: {
            astStmt stmt = node->stmt;
            switch(stmt.type){
                case ast_call: {
                    astCall call = stmt.call;
                    string call_tree = "call_";
                    call_tree.append(call.name);
                    ++(*nodeFrequencies)[call_tree];
                    if (call.param != NULL) {
                        call_tree.append("{");
                        call_tree.append(get_tree_string(call.param, nodeFrequencies));
                        call_tree.append("}");
                    }
                    return call_tree;
                }
                case ast_ret: {
                    string ret_tree = "ret";
                    ++(*nodeFrequencies)[ret_tree];
                    ret_tree.append("{");
                    ret_tree.append(get_tree_string(stmt.ret.expr, nodeFrequencies));
                    ret_tree.append("}");
                    return ret_tree;
                }
                case ast_block: {
                    string block_tree = "block";
                    ++(*nodeFrequencies)[block_tree];
                    vector<astNode*> slist = *(stmt.block.stmt_list);
                    vector<astNode*>::iterator it = slist.begin();
                    while (it != slist.end()){
                        block_tree.append("{");
                        block_tree.append(get_tree_string(*it, nodeFrequencies));
                        block_tree.append("}");
                        it++;
                    }
                    return block_tree;
                }
                case ast_while: {
                    string while_tree = "while";
                    ++(*nodeFrequencies)[while_tree];
                    while_tree.append("{");
                    while_tree.append(get_tree_string(stmt.whilen.cond, nodeFrequencies));
                    while_tree.append("}");
                    while_tree.append("{");
                    while_tree.append(get_tree_string(stmt.whilen.body, nodeFrequencies));
                    while_tree.append("}");
                    return while_tree;
                }
                case ast_if: {
                    astIf ifn = stmt.ifn;
                    string if_tree = "if";
                    ++(*nodeFrequencies)[if_tree];
                    if_tree.append("{");
                    if_tree.append(get_tree_string(ifn.cond, nodeFrequencies));
                    if_tree.append("}");
                    if_tree.append("{");
                    if_tree.append(get_tree_string(ifn.if_body, nodeFrequencies));
                    if_tree.append("}");
                    if (ifn.else_body != NULL) {
                        if_tree.append("{");
                        if_tree.append(get_tree_string(ifn.else_body, nodeFrequencies));
                        if_tree.append("}");
                    }
                    return if_tree;
                }
                case ast_decl: {
                    string decl_tree = "decl_";
                    ++(*nodeFrequencies)[decl_tree];
                    decl_tree.append(stmt.decl.name);
                    return decl_tree;
                }
                case ast_asgn:	{
                    string asgn_tree = "asgn";
                    ++(*nodeFrequencies)[asgn_tree];
                    asgn_tree.append("{");
                    asgn_tree.append(get_tree_string(stmt.asgn.lhs, nodeFrequencies));
                    asgn_tree.append("}");
                    asgn_tree.append("{");
                    asgn_tree.append(get_tree_string(stmt.asgn.rhs, nodeFrequencies));
                    asgn_tree.append("}");
                    return asgn_tree;
                }
            }
		}
		case ast_var: {
            string var_tree = "var_";
            ++(*nodeFrequencies)[var_tree];
            var_tree.append(node->var.name);
            return var_tree;
		}
		case ast_cnst: {
            string cnst_tree = "cnst_";
            ++(*nodeFrequencies)[cnst_tree];
            cnst_tree.append(std::to_string(node->cnst.value));
            return cnst_tree;
        }
		case ast_rexpr: {
            string rexpr_tree = "rexpr";
            ++(*nodeFrequencies)[rexpr_tree];
            rexpr_tree.append("{");
            rexpr_tree.append(get_tree_string(node->rexpr.lhs, nodeFrequencies));
            rexpr_tree.append("}");
            rexpr_tree.append("{");
            switch(node->rexpr.op) {
                case lt: {
                    rexpr_tree.append("lt");
                    break;
                }
                case gt: {
                    rexpr_tree.append("gt");
                    break;
                }
                case le: {
                    rexpr_tree.append("le");
                    break;
                }
                case ge: {
                    rexpr_tree.append("ge");
                    break;
                }
                case eq: {
                    rexpr_tree.append("eq");
                    break;
                }
                case neq: {
                    rexpr_tree.append("ne");
                    break;
                }
            }
            rexpr_tree.append("}");
            rexpr_tree.append("{");
            rexpr_tree.append(get_tree_string(node->rexpr.rhs, nodeFrequencies));
            rexpr_tree.append("}");
            return rexpr_tree;
		}
		case ast_bexpr: {
            string bexpr_tree = "bexpr";
            ++(*nodeFrequencies)[bexpr_tree];
            bexpr_tree.append("{");
            bexpr_tree.append(get_tree_string(node->bexpr.lhs, nodeFrequencies));
            bexpr_tree.append("}");
            bexpr_tree.append("{");
            switch(node->bexpr.op) {
                case add: {
                    bexpr_tree.append("add");
                    break;
                }
                case sub: {
                    bexpr_tree.append("sub");
                    break;
                }
                case mul: {
                    bexpr_tree.append("mul");
                    break;
                }
                case divide: {
                    bexpr_tree.append("div");
                    break;
                }
            }
            bexpr_tree.append("}");
            bexpr_tree.append("{");
            bexpr_tree.append(get_tree_string(node->bexpr.rhs, nodeFrequencies));
            bexpr_tree.append("}");
            return bexpr_tree;
		}
		case ast_uexpr: {
            string uexpr_tree = "uexpr";
            ++(*nodeFrequencies)[uexpr_tree];
            uexpr_tree.append("{");
            uexpr_tree.append(get_tree_string(node->uexpr.expr, nodeFrequencies));
            uexpr_tree.append("}");
            return uexpr_tree;
        }
		default: {
            string invalid_tree = "";
            printf("node has incorrect type\n");
			return invalid_tree;
		}
	}
}

std::string exec(const char* cmd) {
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

int calculate_similarity(astNode *rootAst1, astNode *rootAst2) {
    unordered_map<string, int> nodeFrequencies1;
    unordered_map<string, int> nodeFrequencies2;
	string tree1 = get_tree_string(rootAst1->prog.func, &nodeFrequencies1);
	string tree2 = get_tree_string(rootAst2->prog.func, &nodeFrequencies2);
    //printf("%s\n", tree1.c_str());
    //printf("%s\n", tree2.c_str());
    int total1 = 0;
    int total2 = 0;
    int frequencyDifference = 0;

	for (unordered_map<string,int>::iterator iter = nodeFrequencies1.begin(); iter != nodeFrequencies1.end(); ++iter) {
        total1 += iter->second;
	}

    for (unordered_map<string,int>::iterator iter = nodeFrequencies2.begin(); iter != nodeFrequencies2.end(); ++iter) {
        total2 += iter->second;
	}

    for (unordered_map<string,int>::iterator iter = nodeFrequencies1.begin(); iter != nodeFrequencies1.end(); ++iter) {
        frequencyDifference += abs(iter->second - nodeFrequencies2[iter->first]);
        nodeFrequencies2.erase(iter->first);
	}

    for (unordered_map<string,int>::iterator iter = nodeFrequencies2.begin(); iter != nodeFrequencies2.end(); ++iter) {
        frequencyDifference += abs(iter->second - nodeFrequencies1[iter->first]);
	}

    int total = max(total1, total2);
    float logicScore = (float)(total - frequencyDifference) / (float) total;

    string cmd = "./ted string ";
    cmd.append(tree1);
    cmd.append(" ");
    cmd.append(tree2);
    std::string str = exec(cmd.c_str());

    regex rgx("Size of source tree:([0-9]+)\nSize of destination tree:([0-9]+)\nDistance TED:([0-9]+)");
    smatch matches;
    
    float structureScore = 0;
    if (regex_search(str, matches, rgx)) {
        int tree1Size = stoi(matches[1].str());
        int tree2Size = stoi(matches[2].str());
        int editDistance = stoi(matches[3].str());
        int totalSize = tree1Size + tree2Size;
        structureScore = (float)(totalSize-editDistance) / (float)totalSize;
    }

    float score = 80.0 * structureScore + 20.0 * logicScore;

    return score;
}

