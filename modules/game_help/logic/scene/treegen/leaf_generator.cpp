#include "leaf_generator.h"

void ProceduralTreeParameter::_bind_methods() {
	ADD_GROUP(L"树干整体参数", "");
	ADD_SIMPLE_ENUM_MEMBER_PROPERTY(int, shape, "圆锥形,球形,半球形,圆柱形,锥型圆柱形,火焰,反向圆锥,趋向火焰,自定义形状");

	// 整棵树的缩放比例。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, scale, 0.01, 150);

	// 缩放比例的最大变化量。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, scale_v, 0, 149.99);

	// 分支的级数，通常为3或4。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(int, levels, 1, 4);

	// 树干长度与半径的比例。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, ratio, 0.0001f, 1);

	// 分支半径在不同级别之间减少的幅度。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, ratio_power, 0.1, 8);

	// 树干基部半径增加的比例，以树干半径的分数表示。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, flare, 0, 10);

	// 树干基部高度处分叉的数量，如果为负数，则分叉数量将随机选择，最大为|base_splits|。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(int, base_splits, -5, 5);

#define ADD_TREE_MEMBER_PROPERTY_ARRAY4(index)                                \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, down_angle, index, 0, 360);      \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, down_angle_v, index, -360, 360); \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, base_size, index, 0.01, 1);      \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, rotation, index, -360, 360);     \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, rotation_v, index, -360, 360);   \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(int, branches, index, -500, 500);       \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, length, index, 0, 1);            \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, length_v, 0, 0, 1);              \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, taper, index, 0, 3);             \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, segment_splits, index, 0, 2);    \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, split_angle, index, 0, 360);     \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, split_angle_v, index, 0, 360);   \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(int, curve_resolution, index, 1, 10);   \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve, index, -360, 360);        \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve_v, index, -360, 360);      \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve_back, index, -360, 360);   \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, bend_v, index, 0, 360);          \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, branch_dist, index, 0, 1);       \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, radius_modify, index, 0, 1);     \
	ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, tropism, index, -10, 10);
	{
		ADD_GROUP(L"树枝层1", "_0_");
		ADD_TREE_MEMBER_PROPERTY_ARRAY4(0);
	}
	{
		ADD_GROUP(L"树枝层2", "_1_");
		ADD_TREE_MEMBER_PROPERTY_ARRAY4(1);
	}
	{
		ADD_GROUP(L"树枝层3", "_2_");
		ADD_TREE_MEMBER_PROPERTY_ARRAY4(2);
	}
	{
		ADD_GROUP(L"树枝层4", "_3_");
		ADD_TREE_MEMBER_PROPERTY_ARRAY4(3);
	}
#undef ADD_TREE_MEMBER_PROPERTY_ARRAY4

	ADD_GROUP(L"树叶", "");
	// 每个最深层分支上的叶子或花的数量。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(int, n_leaves, -1000, 3000);

	// 预定义的叶子形状。
	ADD_SIMPLE_ENUM_MEMBER_PROPERTY(int, leaf_shape, "卵型,茅草叶,红薯叶,枫叶,棕榈,尖刺橡树,圆橡木,椭圆,矩形,三角形,樱花,橘子花,木兰花");

	// 叶子的缩放比例。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, leaf_scale, 0.001, 1000);

	// 叶子在x方向的缩放比例。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, leaf_scale_x, 0.001, 1000);

	// 叶子重新定向以面向光线（向上和向外）的比例。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, leaf_bend, 0, 1);

	ADD_GROUP(L"自定义形状", "");
	// 修剪包络峰值宽度出现的位置，以从修剪底部向上的分数距离表示。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, prune_width_peak, 0, 200);

	// 修剪包络下部的曲率。
	// < 1导致凸形，> 1导致凹形。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, prune_power_low, -200, 200);
	// 修剪包络上部的曲率。
	// < 1导致凸形，> 1导致凹形。
	ADD_SIMPLE_RANGE_MEMBER_PROPERTY(float, prune_power_high, -200, 200);
