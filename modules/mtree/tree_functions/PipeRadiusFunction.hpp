#pragma once
#include <vector>
// #include <queue>
#include "TreeFunction.hpp"
// #include "modules/game_help/mtree/utilities/NodeUtilities.hpp"
// #include "modules/game_help/mtree/utilities/GeometryUtilities.hpp"
#include "Property.hpp"

namespace Mtree {
class PipeRadiusFunction : public TreeFunction {
	GDCLASS(PipeRadiusFunction, TreeFunction);
	static void _bind_methods();

private:
	void update_radius_rec(TreeNode &node);

public:
	DECL_SIMPLE_MEMBER_PROPERTY(float, power) = 2.5f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, end_radius) = .005f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, constant_growth) = .01f;

public:
	void execute(std::vector<Stem> &stems, int id, int parent_id) override;
};

} //namespace Mtree
