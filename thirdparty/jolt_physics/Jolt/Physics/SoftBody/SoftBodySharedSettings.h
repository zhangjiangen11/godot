// Jolt Physics 物理引擎库 (https://github.com/jrouwe/JoltPhysics)
// 版权声明文本：2023 Jorrit Rouwe
// 开源协议：MIT

#pragma once

#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Core/StreamUtils.h>

JPH_NAMESPACE_BEGIN

/// 此类定义了所有粒子及其约束的配置。
/// 该类在模拟过程中使用，可在多个软体之间共享。
class JPH_EXPORT SoftBodySharedSettings : public RefTarget<SoftBodySharedSettings>
{
	JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, SoftBodySharedSettings)

public:
	/// 要创建的弯曲约束类型
	enum class EBendType
	{
		None,														///< 不创建任何弯曲约束
		Distance,													///< 简单的距离约束（用于弯曲限制）
		Dihedral,													///< 二面角弯曲约束（计算开销最大，但支持初始不在同一平面的三角形）
	};

	/// 要创建的长距离附着约束（LRA）类型
	enum class ELRAType
	{
		None,														///< 不创建LRA约束
		EuclideanDistance,											///< 基于「最近运动学顶点与当前顶点的欧几里得距离」创建LRA约束
		GeodesicDistance,											///< 基于「最近运动学顶点与当前顶点的测地距离」创建LRA约束（沿边约束路径计算）
	};

	/// CreateConstraints函数执行时使用的每个顶点的属性。
	/// 对于边约束或剪切约束，柔度（Compliance）会取两个关联顶点的平均值。
	/// 对于弯曲约束，柔度会取共享边上两个顶点的平均值。
	struct JPH_EXPORT VertexAttributes
	{
		/// 构造函数
						VertexAttributes() = default;
						VertexAttributes(float inCompliance, float inShearCompliance, float inBendCompliance, ELRAType inLRAType = ELRAType::None, float inLRAMaxDistanceMultiplier = 1.0f) 
							: mCompliance(inCompliance), mShearCompliance(inShearCompliance), mBendCompliance(inBendCompliance), mLRAType(inLRAType), mLRAMaxDistanceMultiplier(inLRAMaxDistanceMultiplier) { }

		float			mCompliance = 0.0f;							///< 普通边约束的柔度（柔度=刚度的倒数）。设为FLT_MAX可禁用所有涉及该顶点的普通边约束。
		float			mShearCompliance = 0.0f;					///< 剪切边约束的柔度。设为FLT_MAX可禁用所有涉及该顶点的剪切边约束。
		float			mBendCompliance = FLT_MAX;					///< 弯曲边约束的柔度。设为FLT_MAX可禁用所有涉及该顶点的弯曲约束。
		ELRAType		mLRAType = ELRAType::None;					///< 要创建的长距离附着约束（LRA）类型。
		float			mLRAMaxDistanceMultiplier = 1.0f;			///< LRA约束最大距离的乘数，例如1.01表示最大距离比静息姿态下的计算距离长1%。
	};

	/// 基于软体的面自动创建约束
	/// @param inVertexAttributes 每个顶点的属性列表（与mVertices一一对应，若列表长度小于mVertices则重复最后一个元素）。该参数定义了要创建的约束的属性。
	/// @param inVertexAttributesLength inVertexAttributes的长度
	/// @param inBendType 要创建的弯曲约束类型
	/// @param inAngleTolerance 当两个相连的三角形构成四边形（大致在同一平面且形成近似90度角的正方形）时，会创建剪切边。该参数定义角度容差（单位：弧度）。
	void				CreateConstraints(const VertexAttributes *inVertexAttributes, uint inVertexAttributesLength, EBendType inBendType = EBendType::Distance, float inAngleTolerance = DegreesToRadians(8.0f));

	/// 计算该软体所有边弹簧的初始长度（若调用CreateConstraint，此步骤已自动完成）
	void				CalculateEdgeLengths();

	/// 计算杆（Rod）的属性
	/// 注意：若两个杆通过RodBendTwist约束连接但指向相反方向，此函数可能会交换RodStretchShear约束的mVertex顺序。
	void				CalculateRodProperties();

	/// 基于欧几里得距离计算长距离附着约束（LRA）的最大长度（若调用CreateConstraints，此步骤已自动完成）
	/// @param inMaxDistanceMultiplier LRA约束最大距离的乘数，例如1.01表示最大距离比静息姿态下的计算距离长1%。
	void				CalculateLRALengths(float inMaxDistanceMultiplier = 1.0f);

	/// 计算弯曲约束的常量（若调用CreateConstraints，此步骤已自动完成）
	void				CalculateBendConstraintConstants();

	/// 计算该软体所有四面体的初始体积
	void				CalculateVolumeConstraintVolumes();

	/// 计算运行时计算蒙皮约束法线所需的信息
	void				CalculateSkinnedConstraintNormals();

	/// 软体优化相关信息（部分元素的索引可能已变更）
	class OptimizationResults
	{
	public:
		Array<uint>		mEdgeRemap;									///< 旧边索引到新边索引的映射表
		Array<uint>		mLRARemap;									///< 旧LRA约束索引到新LRA约束索引的映射表
		Array<uint>		mRodStretchShearConstraintRemap;			///< 旧杆拉伸-剪切约束索引到新杆拉伸-剪切约束索引的映射表
		Array<uint>		mRodBendTwistConstraintRemap;				///< 旧杆弯曲-扭转约束索引到新杆弯曲-扭转约束索引的映射表
		Array<uint>		mDihedralBendRemap;							///< 旧二面角弯曲约束索引到新二面角弯曲约束索引的映射表
		Array<uint>		mVolumeRemap;								///< 旧体积约束索引到新体积约束索引的映射表
		Array<uint>		mSkinnedRemap;								///< 旧蒙皮约束索引到新蒙皮约束索引的映射表
	};

	/// 优化软体配置以适配模拟。该函数会重新排序约束，使其可并行执行。
	void				Optimize(OptimizationResults &outResults);

	/// 无返回结果的软体配置优化
	void				Optimize()									{ OptimizationResults results; Optimize(results); }

	/// 克隆当前对象
	Ref<SoftBodySharedSettings> Clone() const;

	/// 将当前对象的状态以二进制形式保存到inStream。不存储材质列表。
	void				SaveBinaryState(StreamOut &inStream) const;

	/// 从inStream恢复当前对象的状态。不恢复材质列表。
	void				RestoreBinaryState(StreamIn &inStream);

	using SharedSettingsToIDMap = StreamUtils::ObjectToIDMap<SoftBodySharedSettings>;
	using IDToSharedSettingsMap = StreamUtils::IDToObjectMap<SoftBodySharedSettings>;
	using MaterialToIDMap = StreamUtils::ObjectToIDMap<PhysicsMaterial>;
	using IDToMaterialMap = StreamUtils::IDToObjectMap<PhysicsMaterial>;

	/// 保存该共享配置及其材质。传入空的ioSettingsMap/ioMaterialMap，或在向同一流保存多个配置对象时复用同一映射表，以避免重复写入。
	void				SaveWithMaterials(StreamOut &inStream, SharedSettingsToIDMap &ioSettingsMap, MaterialToIDMap &ioMaterialMap) const;

	using SettingsResult = Result<Ref<SoftBodySharedSettings>>;

	/// 恢复软体配置及材质。传入空的ioSettingsMap/ioMaterialMap，或在从同一流读取多个配置对象时复用同一映射表，以恢复重复的对象。
	static SettingsResult sRestoreWithMaterials(StreamIn &inStream, IDToSharedSettingsMap &ioSettingsMap, IDToMaterialMap &ioMaterialMap);

	/// 创建立方体软体。可用于创建简单的测试用软体。
	/// 该立方体包含边约束、体积约束和面。
	/// @param inGridSize 每个轴向上的点数
	/// @param inGridSpacing 点之间的间距
	static Ref<SoftBodySharedSettings> sCreateCube(uint inGridSize, float inGridSpacing);

	/// 顶点代表一个粒子，该结构体中的数据仅在软体创建阶段使用，模拟阶段不使用
	struct JPH_EXPORT Vertex
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, Vertex)

		/// 构造函数
						Vertex() = default;
						Vertex(const Float3 &inPosition, const Float3 &inVelocity = Float3(0, 0, 0), float inInvMass = 1.0f) : mPosition(inPosition), mVelocity(inVelocity), mInvMass(inInvMass) { }

		Float3			mPosition { 0, 0, 0 };						///< 顶点的初始位置
		Float3			mVelocity { 0, 0, 0 };						///< 顶点的初始速度
		float			mInvMass = 1.0f;							///< 顶点的初始质量的倒数
	};

	/// 面定义了软体的表面
	struct JPH_EXPORT Face
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, Face)

		/// 构造函数
						Face() = default;
						Face(uint32 inVertex1, uint32 inVertex2, uint32 inVertex3, uint32 inMaterialIndex = 0) : mVertex { inVertex1, inVertex2, inVertex3 }, mMaterialIndex(inMaterialIndex) { }

		/// 检查该面是否为退化面（指向同一顶点两次的面）
		bool			IsDegenerate() const						{ return mVertex[0] == mVertex[1] || mVertex[0] == mVertex[2] || mVertex[1] == mVertex[2]; }

		uint32			mVertex[3];									///< 构成该面的顶点索引
		uint32			mMaterialIndex = 0;							///< 该面的材质在SoftBodySharedSettings::mMaterials中的索引
	};

	/// 边约束通过弹簧保持两个顶点的距离恒定：|x1 - x2| = 静息长度
	struct JPH_EXPORT Edge
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, Edge)

		/// 构造函数
						Edge() = default;
						Edge(uint32 inVertex1, uint32 inVertex2, float inCompliance = 0.0f) : mVertex { inVertex1, inVertex2 }, mCompliance(inCompliance) { }

		/// 返回该约束的最小顶点索引
		uint32			GetMinVertexIndex() const					{ return min(mVertex[0], mVertex[1]); }

		uint32			mVertex[2];									///< 构成该边的顶点索引
		float			mRestLength = 1.0f;							///< 弹簧的静息长度，由CalculateEdgeLengths计算
		float			mCompliance = 0.0f;							///< 弹簧刚度的倒数（柔度）
	};

	/**
	 * 二面角弯曲约束用于保持两个三角形沿其共享边的夹角恒定。
	 *
	 *        x2
	 *       /  \
	 *      / t0 \
	 *     x0----x1  （共享边：x0-x1）
	 *      \ t1 /
	 *       \  /
	 *        x3
	 *
	 * x0..x3为顶点，t0和t1为共享边x0..x1的两个三角形
	 *
	 * 实现基于：
	 * - 《Position Based Dynamics》 - Matthias Muller 等人
	 * - 《Strain Based Dynamics》 - Matthias Muller 等人
	 * - 《Simulation of Clothing with Folds and Wrinkles》 - R. Bridson 等人
	 */
	struct JPH_EXPORT DihedralBend
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, DihedralBend)

		/// 构造函数
						DihedralBend() = default;
						DihedralBend(uint32 inVertex1, uint32 inVertex2, uint32 inVertex3, uint32 inVertex4, float inCompliance = 0.0f) : mVertex { inVertex1, inVertex2, inVertex3, inVertex4 }, mCompliance(inCompliance) { }

		/// 返回该约束的最小顶点索引
		uint32			GetMinVertexIndex() const					{ return min(min(mVertex[0], mVertex[1]), min(mVertex[2], mVertex[3])); }

		uint32			mVertex[4];									///< 共享一条边的两个三角形的顶点索引（前两个顶点为共享边）
		float			mCompliance = 0.0f;							///< 约束刚度的倒数（柔度）
		float			mInitialAngle = 0.0f;						///< 三角形法向量之间的初始夹角（π - 二面角），由CalculateBendConstraintConstants计算
	};

	/// 体积约束，保持四面体的体积恒定
	struct JPH_EXPORT Volume
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, Volume)

		/// 构造函数
						Volume() = default;
						Volume(uint32 inVertex1, uint32 inVertex2, uint32 inVertex3, uint32 inVertex4, float inCompliance = 0.0f) : mVertex { inVertex1, inVertex2, inVertex3, inVertex4 }, mCompliance(inCompliance) { }

		/// 返回该约束的最小顶点索引
		uint32			GetMinVertexIndex() const					{ return min(min(mVertex[0], mVertex[1]), min(mVertex[2], mVertex[3])); }

		uint32			mVertex[4];									///< 构成四面体的顶点索引
		float			mSixRestVolume = 1.0f;						///< 四面体静息体积的6倍，由CalculateVolumeConstraintVolumes计算
		float			mCompliance = 0.0f;							///< 约束刚度的倒数（柔度）
	};

	/// 逆绑定矩阵：将蒙皮顶点从绑定姿态转换到关节局部空间
	class JPH_EXPORT InvBind
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, InvBind)

	public:
		/// 构造函数
						InvBind() = default;
						InvBind(uint32 inJointIndex, Mat44Arg inInvBind) : mJointIndex(inJointIndex), mInvBind(inInvBind) { }

		uint32			mJointIndex = 0;							///< 该逆绑定矩阵关联的关节索引
		Mat44			mInvBind = Mat44::sIdentity();				///< 逆绑定矩阵，将绑定姿态下的顶点（Vertex::mPosition）转换到关节局部空间
	};

	/// 关节及其蒙皮权重
	class JPH_EXPORT SkinWeight
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, SkinWeight)

	public:
		/// 构造函数
						SkinWeight() = default;
						SkinWeight(uint32 inInvBindIndex, float inWeight) : mInvBindIndex(inInvBindIndex), mWeight(inWeight) { }

		uint32			mInvBindIndex = 0;							///< mInvBindMatrices中的索引
		float			mWeight = 0.0f;								///< 蒙皮权重值
	};

	/// 蒙皮约束：将顶点绑定到多个关节，并限制模拟顶点与蒙皮顶点的最大距离
	class JPH_EXPORT Skinned
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, Skinned)

	public:
		/// 构造函数
						Skinned() = default;
						Skinned(uint32 inVertex, float inMaxDistance, float inBackStopDistance, float inBackStopRadius) : mVertex(inVertex), mMaxDistance(inMaxDistance), mBackStopDistance(inBackStopDistance), mBackStopRadius(inBackStopRadius) { }

		/// 归一化权重，使权重总和为1
		void			NormalizeWeights()
		{
			// 计算总权重
			float total = 0.0f;
			for (const SkinWeight &w : mWeights)
				total += w.mWeight;

			// 归一化
			if (total > 0.0f)
				for (SkinWeight &w : mWeights)
					w.mWeight /= total;
		}

		/// 最大蒙皮权重数量
		static constexpr uint cMaxSkinWeights = 4;

		uint32			mVertex = 0;								///< mVertices中被蒙皮的顶点索引
		SkinWeight		mWeights[cMaxSkinWeights];					///< 蒙皮权重列表，顶点的绑定姿态默认存储在Vertex::mPosition中。第一个权重为0的元素表示列表结束。权重总和应等于1。
		float			mMaxDistance = FLT_MAX;						///< 该顶点与蒙皮顶点的最大距离限制，设为FLT_MAX时禁用。设为0时表示将顶点硬绑定到蒙皮顶点。
		float			mBackStopDistance = FLT_MAX;				///< 若mBackStopDistance >= mMaxDistance则禁用。mVertex周围的面会计算平均法线，在该法线反方向、顶点后方mBackStopDistance处，会生成一个后挡球（BackStop Sphere）。模拟顶点会被推出该球体，可用于近似蒙皮顶点后方的蒙皮网格体积。
		float			mBackStopRadius = 40.0f;					///< 后挡球的半径。默认值较大，使球体近似为平面。
		uint32			mNormalInfo = 0;							///< 计算该顶点法线所需的信息：低24位为mSkinnedConstraintNormals中的起始索引，高8位为面的数量（由CalculateSkinnedConstraintNormals生成）
	};

	/// 长距离附着约束（LRA）：限制运动学顶点与动态顶点之间的最大距离
	/// 参考论文：《Long Range Attachments - A Method to Simulate Inextensible Clothing in Computer Games》, Tae-Yong Kim, Nuttapong Chentanez and Matthias Mueller-Fischer
	class JPH_EXPORT LRA
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, LRA)

	public:
		/// 构造函数
						LRA() = default;
						LRA(uint32 inVertex1, uint32 inVertex2, float inMaxDistance) : mVertex { inVertex1, inVertex2 }, mMaxDistance(inMaxDistance) { }

		/// 返回该约束的最小顶点索引
		uint32			GetMinVertexIndex() const					{ return min(mVertex[0], mVertex[1]); }

		uint32			mVertex[2];									///< 相连的两个顶点。第一个顶点应为运动学顶点，第二个为动态顶点。
		float			mMaxDistance = 0.0f;						///< 顶点间的最大距离，由CalculateLRALengths计算
	};

	/// 离散科塞拉特杆（Cosserat Rod）：用刚性杆连接两个粒子，杆具有固定长度和转动惯量。
	/// 杆可替代Edge约束来限制两个顶点的位置。杆的朝向可用于确定附着在杆上的几何体（如植物叶片）的朝向。
	/// 注意：每个杆至少需要一个RodBendTwist约束来限制杆的旋转。若不添加该约束，杆的朝向可能会绕杆轴以恒定速度旋转。
	/// 实现基于：《Position and Orientation Based Cosserat Rods》 - Kugelstadt and Schoemer - SIGGRAPH 2016
	/// 参考链接：https://www.researchgate.net/publication/325597548_Position_and_Orientation_Based_Cosserat_Rods
	struct JPH_EXPORT RodStretchShear
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, RodStretchShear)

		/// 构造函数
						RodStretchShear() = default;
						RodStretchShear(uint32 inVertex1, uint32 inVertex2, float inCompliance = 0.0f) : mVertex { inVertex1, inVertex2 }, mCompliance(inCompliance) { }

		/// 返回该约束的最小顶点索引
		uint32			GetMinVertexIndex() const					{ return min(mVertex[0], mVertex[1]); }

		uint32			mVertex[2];									///< 构成杆的顶点索引
		float			mLength = 1.0f;								///< 杆的固定长度，由CalculateRodProperties计算
		float			mInvMass = 1.0f;							///< 杆的质量的倒数（静态杆设为0），由CalculateRodProperties计算，后续可手动覆盖
		float			mCompliance = 0.0f;							///< 杆刚度的倒数（柔度）
		Quat			mBishop	= Quat::sZero();					///< 杆的毕晓普坐标系（静息姿态下杆的旋转，使其相对于相邻杆无扭转），由CalculateRodProperties计算
	};

	/// 杆弯曲-扭转约束：连接两个科塞拉特杆，限制杆之间的弯曲和扭转
	struct JPH_EXPORT RodBendTwist
	{
		JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, RodBendTwist)

		/// 构造函数
						RodBendTwist() = default;
						RodBendTwist(uint32 inRod1, uint32 inRod2, float inCompliance = 0.0f) : mRod { inRod1, inRod2 }, mCompliance(inCompliance) { }

		uint32			mRod[2];									///< 被约束的杆的索引（mRodStretchShearConstraints中的索引）
		float			mCompliance = 0.0f;							///< 杆约束刚度的倒数（柔度）
		Quat			mOmega0 = Quat::sZero();					///< 杆之间的初始旋转：rod1.mBishop.Conjugated() * rod2.mBishop，由CalculateRodProperties计算
	};

	/// 向软体添加一个面
	void				AddFace(const Face &inFace)					{ JPH_ASSERT(!inFace.IsDegenerate()); mFaces.push_back(inFace); }

	Array<Vertex>		mVertices;									///< 软体的顶点/粒子列表
	Array<Face>			mFaces;										///< 软体的面列表
	Array<Edge>			mEdgeConstraints;							///< 软体的边/弹簧约束列表
	Array<DihedralBend>	mDihedralBendConstraints;					///< 软体的二面角弯曲约束列表
	Array<Volume>		mVolumeConstraints;							///< 软体的体积约束列表（保持软体中四面体的体积恒定）
	Array<Skinned>		mSkinnedConstraints;						///< 绑定到蒙皮顶点的约束列表
	Array<InvBind>		mInvBindMatrices;							///< 顶点蒙皮用的逆绑定矩阵列表
	Array<LRA>			mLRAConstraints;							///< 长距离附着约束（LRA）列表
	Array<RodStretchShear>	mRodStretchShearConstraints;			///< 科塞拉特杆拉伸-剪切约束列表（连接两个顶点，限制拉伸和剪切）
	Array<RodBendTwist>	mRodBendTwistConstraints;					///< 科塞拉特杆弯曲-扭转约束列表（连接两个杆，限制弯曲和扭转）
	PhysicsMaterialList mMaterials { PhysicsMaterial::sDefault };	///< 软体面的材质列表，由Face::mMaterialIndex引用

private:
	friend class SoftBodyMotionProperties;

	/// 计算最近的运动学顶点数组
	void				CalculateClosestKinematic();

	/// 跟踪最近的运动学顶点
	struct ClosestKinematic
	{
		uint32			mVertex = 0xffffffff;						///< 最近运动学顶点的索引
		uint32			mHops = 0xffffffff;							///< 到最近运动学顶点的跳数
		float			mDistance = FLT_MAX;						///< 到最近运动学顶点的距离
	};

	/// 跟踪各约束组的结束索引
	struct UpdateGroup
	{
		uint			mEdgeEndIndex;								///< 本组中边约束的结束索引
		uint			mLRAEndIndex;								///< 本组中LRA约束的结束索引
		uint			mRodStretchShearEndIndex;					///< 本组中杆拉伸-剪切约束的结束索引
		uint			mRodBendTwistEndIndex;						///< 本组中杆弯曲-扭转约束的结束索引
		uint			mDihedralBendEndIndex;						///< 本组中二面角弯曲约束的结束索引
		uint			mVolumeEndIndex;							///< 本组中体积约束的结束索引
		uint			mSkinnedEndIndex;							///< 本组中蒙皮约束的结束索引
	};

	Array<ClosestKinematic> mClosestKinematic;						///< mVertices中每个顶点对应的最近运动学顶点
	Array<UpdateGroup>	mUpdateGroups;								///< 可并行更新的各约束组的结束索引
	Array<uint32>		mSkinnedConstraintNormals;					///< mSkinnedConstraints使用的mFaces数组索引列表，由CalculateSkinnedConstraintNormals计算
};

JPH_NAMESPACE_END