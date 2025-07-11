#pragma once
#include "TreeFunction.hpp"
#include <vector>

namespace Mtree {
class GrowthFunction : public TreeFunction {
	GDCLASS(GrowthFunction, TreeFunction);
	static void _bind_methods();

private:
	float update_vigor_ratio_rec(TreeNode &node);
	void update_vigor_rec(TreeNode &node, float vigor);
	void simulate_growth_rec(TreeNode &node, int id);
	void get_weight_rec(TreeNode &node);
	void apply_gravity_rec(TreeNode &node, Basis curent_rotation);
	void update_absolute_position_rec(TreeNode &node, const Vector3 &node_position);

public:
	DECL_SIMPLE_MEMBER_PROPERTY(int, iterations) = 5;
	DECL_SIMPLE_MEMBER_PROPERTY(float, apical_dominance) = .7f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, grow_threshold) = .5f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, split_angle) = 60;
	DECL_SIMPLE_MEMBER_PROPERTY(float, branch_length) = 1;
	DECL_SIMPLE_MEMBER_PROPERTY(float, gravitropism) = .1f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, randomness) = .1f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, cut_threshold) = .2f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, split_threshold) = .7f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, gravity_strength) = 1;

	DECL_SIMPLE_MEMBER_PROPERTY(float, apical_control) = .7f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, codominant_proba) = .1f;
	DECL_SIMPLE_MEMBER_PROPERTY(int, codominant_count) = 2;
	DECL_SIMPLE_MEMBER_PROPERTY(float, branch_angle) = 60;
	DECL_SIMPLE_MEMBER_PROPERTY(float, philotaxis_angle) = 2.399f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, flower_threshold) = .5f;

	DECL_SIMPLE_MEMBER_PROPERTY(float, growth_delta) = .1f;
	DECL_SIMPLE_MEMBER_PROPERTY(float, flowering_delta) = .1f;

	DECL_SIMPLE_MEMBER_PROPERTY(float, root_flux) = 5;

public:
	void execute(std::vector<Stem> &stems, int id, int parent_id) override;
};

class BioNodeInfo : public GrowthInfo {
public:
	enum class NodeType { Meristem,
		Branch,
		Cut,
		Ignored } type;
	float branch_weight = 0;
	Vector3 center_of_mass;
	Vector3 absolute_position;
	float vigor_ratio = 1;
	float vigor = 0;
	int age = 0;
	float philotaxis_angle = 0;

	BioNodeInfo(NodeType type, int age = 0, float philotaxis_angle = 0) {
		this->type = type;
		this->age = age;
		this->philotaxis_angle = philotaxis_angle;
	}
};

} //namespace Mtree
