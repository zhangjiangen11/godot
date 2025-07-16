#pragma once
#include "../base_types/TreeMesher.hpp"
#include <tuple>

namespace Mtree {

class ManifoldMesher : public TreeMesher {
public:
	struct AttributeNames {
		inline static std::string smooth_amount = "smooth_amount";
		inline static std::string radius = "radius";
		inline static std::string direction = "direction";
	};

	int radial_resolution = 8;
	int smooth_iterations = 4;
	TreeMesh mesh_tree(Tree &tree) override;
};

} //namespace Mtree
