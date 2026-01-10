// Jolt Physics 物理引擎库 (https://github.com/jrouwe/JoltPhysics)
// 版权声明文本：2023 Jorrit Rouwe
// 开源协议：MIT

#pragma once

#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/ObjectStream/SerializableObject.h>
#include <Jolt/Core/StreamUtils.h>

JPH_NAMESPACE_BEGIN

/// 此类包含创建软体对象所需的所有信息
/// 注意：软体功能仍在开发中，存在若干使用限制。请阅读《架构与API文档》了解更多信息！
class JPH_EXPORT SoftBodyCreationSettings
{
	JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, SoftBodyCreationSettings)

public:
	/// 构造函数
						SoftBodyCreationSettings() = default;
						SoftBodyCreationSettings(const SoftBodySharedSettings *inSettings, RVec3Arg inPosition, QuatArg inRotation, ObjectLayer inObjectLayer) 
							: mSettings(inSettings), mPosition(inPosition), mRotation(inRotation), mObjectLayer(inObjectLayer) { }

	/// 将当前对象的状态以二进制形式保存到inStream。不存储共享配置（SharedSettings）和组过滤器（GroupFilter）。
	void				SaveBinaryState(StreamOut &inStream) const;

	/// 从inStream恢复当前对象的状态。不恢复共享配置（SharedSettings）和组过滤器（GroupFilter）。
	void				RestoreBinaryState(StreamIn &inStream);

	using GroupFilterToIDMap = StreamUtils::ObjectToIDMap<GroupFilter>;
	using IDToGroupFilterMap = StreamUtils::IDToObjectMap<GroupFilter>;
	using SharedSettingsToIDMap = SoftBodySharedSettings::SharedSettingsToIDMap;
	using IDToSharedSettingsMap = SoftBodySharedSettings::IDToSharedSettingsMap;
	using MaterialToIDMap = StreamUtils::ObjectToIDMap<PhysicsMaterial>;
	using IDToMaterialMap = StreamUtils::IDToObjectMap<PhysicsMaterial>;

	/// 保存当前软体创建配置、其关联的共享配置及组过滤器。
	/// 传入空的ioSharedSettingsMap/ioMaterialMap/ioGroupFilterMap，或在向同一流保存多个形状时复用同一映射表，以避免重复写入。
	/// 向ioSharedSettingsMap和ioMaterialMap传入nullptr可跳过共享配置和材质的保存；
	/// 向ioGroupFilterMap传入nullptr可跳过组过滤器的保存。
	void				SaveWithChildren(StreamOut &inStream, SharedSettingsToIDMap *ioSharedSettingsMap, MaterialToIDMap *ioMaterialMap, GroupFilterToIDMap *ioGroupFilterMap) const;

	using SBCSResult = Result<SoftBodyCreationSettings>;

	/// 恢复软体配置、其所有子对象及材质。
	/// 传入空的ioSharedSettingsMap/ioMaterialMap/ioGroupFilterMap，或在从同一流读取多个形状时复用同一映射表，以恢复重复的对象。
	static SBCSResult	sRestoreWithChildren(StreamIn &inStream, IDToSharedSettingsMap &ioSharedSettingsMap, IDToMaterialMap &ioMaterialMap, IDToGroupFilterMap &ioGroupFilterMap);

	RefConst<SoftBodySharedSettings> mSettings;				///< 定义当前软体的核心配置（关联SoftBodySharedSettings）

	RVec3				mPosition { RVec3::sZero() };		///< 软体的初始位置
	Quat				mRotation { Quat::sIdentity() };	///< 软体的初始旋转

	/// 用户自定义数据（可由应用层使用）
	uint64				mUserData = 0;

	///@name 碰撞相关配置
	ObjectLayer			mObjectLayer = 0;					///< 当前软体所属的碰撞层（决定两个对象是否能发生碰撞）
	CollisionGroup		mCollisionGroup;					///< 当前软体所属的碰撞组（决定两个对象是否能发生碰撞）

	uint32				mNumIterations = 5;					///< 求解器迭代次数（值越高模拟精度越高，性能开销越大）
	float				mLinearDamping = 0.1f;				///< 线性阻尼：速度衰减公式为 dv/dt = -mLinearDamping * v
	float				mMaxLinearVelocity = 500.0f;		///< 顶点允许的最大线速度（单位：米/秒）
	float				mRestitution = 0.0f;				///< 碰撞恢复系数（弹性，0=完全非弹性碰撞，1=完全弹性碰撞）
	float				mFriction = 0.2f;					///< 碰撞摩擦系数
	float				mPressure = 0.0f;					///< 内部压力（公式：n * R * T，即物质的量×理想气体常数×绝对温度），参考：https://en.wikipedia.org/wiki/Pressure
	float				mGravityFactor = 1.0f;				///< 当前软体的重力缩放系数（乘以全局重力值）
	float				mVertexRadius = 0.0f;				///< 粒子（顶点）的半径，可用于将顶点略微推离其他物体表面，避免Z-fighting（深度冲突）
	bool				mUpdatePosition = true;				///< 模拟过程中是否更新软体的位置（若软体绑定到静态世界，需设为false）
	bool				mMakeRotationIdentity = true;		///< 将指定的mRotation烘焙到顶点数据中，并将软体旋转设为单位矩阵（软体旋转保持单位矩阵时，模拟精度略高）
	bool				mAllowSleeping = true;				///< 当前软体是否允许进入休眠状态（休眠后停止模拟，节省性能）
	bool				mFacesDoubleSided = false;			///< 碰撞检测（射线检测/形状碰撞/形状投射）时，是否将软体的面视为双面
};

JPH_NAMESPACE_END