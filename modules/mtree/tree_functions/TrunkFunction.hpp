#pragma once
#include "TreeFunction.hpp"
#include <vector>

namespace Mtree {
class TrunkFunction : public TreeFunction {
	GDCLASS(TrunkFunction, TreeFunction);
	static void _bind_methods();

public:
	DECL_SIMPLE_MEMBER_PROPERTY(float, length) = 10;
	DECL_SIMPLE_MEMBER_PROPERTY(float, start_radius) = .3f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, end_radius) = .05;
	DECL_SIMPLE_MEMBER_PROPERTY(float, shape) = .5;
	DECL_SIMPLE_MEMBER_PROPERTY(float, resolution) = 3.f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, randomness) = .1f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, up_attraction) = .6f;

	void execute(std::vector<Stem> &stems, int id, int parent_id) override;
};

} //namespace Mtree
