#pragma once
#include "../tree_functions/TreeFunction.hpp"
#include "TreeNode.hpp"
#include <vector>

namespace Mtree {
class Tree {
private:
	std::vector<Stem> stems;
	Ref<TreeFunction> firstFunction;

public:
	Tree(Ref<TreeFunction> trunkFunction);
	Tree() { firstFunction = nullptr; }
	void set_first_function(Ref<TreeFunction> function);
	void execute_functions();
	void print_tree();
	std::vector<Stem> &get_stems();
};
} //namespace Mtree
