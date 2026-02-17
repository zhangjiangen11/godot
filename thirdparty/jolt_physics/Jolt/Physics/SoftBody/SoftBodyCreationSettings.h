// Jolt Physics 物理引擎库 (https://github.com/jrouwe/JoltPhysics)
// 版权声明文本：2023 Jorrit Rouwe
// 开源协议：MIT

#pragma once

#include <Jolt/Core/StreamUtils.h>
#include <Jolt/ObjectStream/SerializableObject.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>

JPH_NAMESPACE_BEGIN

/// 此类包含创建软体对象所需的所有信息
/// 注意：软体功能仍在开发中，存在若干使用限制。请阅读《架构与API文档》了解更多信息！
class JPH_EXPORT SoftBodyCreationSettings {
	JPH_DECLARE_SERIALIZABLE_NON_VIRTUAL(JPH_EXPORT, SoftBodyCreationSettings)

public:
	/// 构造函数
	SoftBodyCreationSettings() = default;
	SoftBodyCreationSettings(const SoftBodySharedSettings *inSettings, RVec3Arg inPosition, QuatArg inRotation, ObjectLayer inObjectLayer) : mSettings(inSettings), mPosition(inPosition), mRotation(inRotation), mObjectLayer(inObjectLayer) {}

	/// 将当前对象的状态以二进制形式保存到inStream。不存储共享配置（SharedSettings）和组过滤器（GroupFilter）。
	void SaveBinaryState(StreamOut &inStream) const;

	/// 从inStream恢复当前对象的状态。不恢复共享配置（SharedSettings）和组过滤器（GroupFilter）。
	void RestoreBinaryState(StreamIn &inStream);

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
	void SaveWithChildren(StreamOut &inStream, SharedSettingsToIDMap *ioSharedSettingsMap, MaterialToIDMap *ioMaterialMap, GroupFilterToIDMap *ioGroupFilterMap) const;

	using SBCSResult = Result<SoftBodyCreationSettings>;

	/// 恢复软体配置、其所有子对象及材质。
	/// 传入空的ioSharedSettingsMap/ioMaterialMap/ioGroupFilterMap，或在从同一流读取多个形状时复用同一映射表，以恢复重复的对象。
	static SBCSResult sRestoreWithChildren(StreamIn &inStream, IDToSharedSettingsMap &ioSharedSettingsMap, IDToMaterialMap &ioMaterialMap, IDToGroupFilterMap &ioGroupFilterMap);

	RefConst<SoftBodySharedSettings> mSettings; ///< 定义当前软体的核心配置（关联SoftBodySharedSettings）

	RVec3 mPosition{ RVec3::sZero() }; ///< 软体的初始位置
	Quat mRotation{ Quat::sIdentity() }; ///< 软体的初始旋转

	/// 用户自定义数据（可由应用层使用）
	uint64 mUserData = 0;

	///@name 碰撞相关配置
	ObjectLayer mObjectLayer = 0; ///< 当前软体所属的碰撞层（决定两个对象是否能发生碰撞）
	CollisionGroup mCollisionGroup; ///< 当前软体所属的碰撞组（决定两个对象是否能发生碰撞）

	uint32 mNumIterations = 5; ///< Number of solver iterations
	float mLinearDamping = 0.1f; ///< Linear damping: dv/dt = -mLinearDamping * v. Value should be zero or positive and is usually close to 0.
	float mMaxLinearVelocity = 500.0f; ///< Maximum linear velocity that a vertex can reach (m/s)
	float mRestitution = 0.0f; ///< Restitution when colliding
	float mFriction = 0.2f; ///< Friction coefficient when colliding
	float mPressure = 0.0f; ///< n * R * T, amount of substance * ideal gas constant * absolute temperature, see https://en.wikipedia.org/wiki/Pressure
	float mGravityFactor = 1.0f; ///< Value to multiply gravity with for this body
	float mVertexRadius = 0.0f; ///< How big the particles are, can be used to push the vertices a little bit away from the surface of other bodies to prevent z-fighting
	bool mUpdatePosition = true; ///< Update the position of the body while simulating (set to false for something that is attached to the static world)
	bool mMakeRotationIdentity = true; ///< Bake specified mRotation in the vertices and set the body rotation to identity (simulation is slightly more accurate if the rotation of a soft body is kept to identity)
	bool mAllowSleeping = true; ///< If this body can go to sleep or not
	bool mFacesDoubleSided = false; ///< If the faces in this soft body should be treated as double sided for the purpose of collision detection (ray cast / collide shape / cast shape)
};

JPH_NAMESPACE_END