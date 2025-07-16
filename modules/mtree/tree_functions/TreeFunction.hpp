#pragma once
#include "../tree/TreeNode.hpp"
#include "../utilities/RandomGenerator.hpp"
#include "core/io/resource.h"
#include "core/object/ref_counted.h"
#include <vector>

namespace Mtree {
class TreeFunction : public RefCounted {
	GDCLASS(TreeFunction, RefCounted);
	static void _bind_methods() {
	}

protected:
	RandomGenerator rand_gen;
	LocalVector<Ref<TreeFunction>> children;
	void execute_children(std::vector<Stem> &stems, int id);

public:
	DECL_SIMPLE_MEMBER_PROPERTY(int, seed) = 42;

	virtual void execute(std::vector<Stem> &stems, int id = 0, int parent_id = 0) {
	}
	void add_child(Ref<TreeFunction> child);
};
} //namespace Mtree
