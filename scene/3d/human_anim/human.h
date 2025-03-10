#pragma once

#include "./defs.h"
#include "./memory.h"
#include "./types.h"
#include "./bitset.h"
#include "./Simd/vec-trs.h"
#include "./human_skeleton.h"
#include "./human_hand.h"
#include "scene/resources/animation.h"
#include "core/string/ustring.h"

class Skeleton3D;
namespace math
{
    struct SetupAxesInfo;
}

namespace human_anim
{
namespace skeleton
{
    struct HumanSkeleton;

    template<typename transformType> struct SkeletonPoseT;
    typedef SkeletonPoseT<math::trsX> SkeletonPose;
}

namespace human
{
	// 這裏是身體部分的主要骨骼，包羅了avatar 裏面的body和head，不包含手指部分
    enum Bones
    {
        kHips = 0,     //臀部
        kLeftUpperLeg, //1   // 左邊大腿根部骨骼
        kRightUpperLeg, // 右邊邊大腿根部骨骼
        kLeftLowerLeg, //3     // 左邊膝蓋
        kRightLowerLeg, // 右邊膝蓋
        kLeftFoot, //5    // 左邊膝蓋
        kRightFoot, // 右邊膝蓋
        kSpine, //7    // 腹部
        kChest, // 中胸部
        kUpperChest, //9  // 上胸部
        kNeck,  // 脖子
        kHead, //11     // 頭部
        kLeftShoulder, // 左肩（靠近脖子）
        kRightShoulder, //13     // 右肩（靠近脖子）
        kLeftUpperArm, // 左邊上臂根部
        kRightUpperArm, //15     // 右邊上臂根部
        kLeftLowerArm, // 左邊肘部
        kRightLowerArm, //17     // 右邊肘部
        kLeftHand, //  左手腕
        kRightHand, //19     //  右手腕
        kLeftToes, // 左邊脚趾尖
        kRightToes, //21     //  右邊脚趾尖

        kLeftEye, //  左眼
        kRightEye, //23    //  右眼
        kJaw,	  //  下顎
        kLastBone //25    
    };
	// 手部骨骼，枚舉定義值得時候是接到身體後面的
    enum HumanBones
    {
        kBodyBoneStart = 0,
        kBodyBoneStop = kBodyBoneStart + kLastBone,					//25 // 身體骨骼結束
        kLeftHandBoneStart = kBodyBoneStop,							//25 // 左手骨骼開始
        kLeftThumbStart = kLeftHandBoneStart,						//25  //左手拇指開始
        kLeftThumbStop = kLeftThumbStart + hand::kLastPhalange,		//25+3=28 // 左手拇指結束
        kLeftIndexStart = kLeftThumbStop,							//28 // 左手食指開始
        kLeftIndexStop = kLeftIndexStart + hand::kLastPhalange,		//28+3=31 // 左手食指結束
        kLeftMiddleStart = kLeftIndexStop,							//31 // 左手中指開始
        kLeftMiddleStop = kLeftMiddleStart + hand::kLastPhalange,	//31+3=34 // 左手中指結束
        kLeftRingStart = kLeftMiddleStop,							//34 // 左手無名指開始
        kLeftRingStop = kLeftRingStart + hand::kLastPhalange,		//34+3 = 37 // 左手無名指結束
        kLeftLittleStart = kLeftRingStop,							//37 // 左手小指開始
        kLeftLittleStop = kLeftLittleStart + hand::kLastPhalange,	//37+3 = 40 // 左手小指結束
        kLeftHandBoneStop = kLeftLittleStop,						//40 // 左手骨頭結束
        kRightHandBoneStart = kLeftHandBoneStop,					//40 // 右手骨頭開始
        kRightThumbStart = kRightHandBoneStart,						//40 // 右手拇指開始
        kRightThumbStop = kRightThumbStart + hand::kLastPhalange,	//40+3 =43 // 右手拇指結束
        kRightIndexStart = kRightThumbStop,							//43 // 右手食指開始
        kRightIndexStop = kRightIndexStart + hand::kLastPhalange,	//43+3 = 46 // 右手食指結束
        kRightMiddleStart = kRightIndexStop,						//46 // 右手中指開始
        kRightMiddleStop = kRightMiddleStart + hand::kLastPhalange,	//46+3=49 // 右手中指結束
        kRightRingStart = kRightMiddleStop,							//49 // 右手無名指開始
        kRightRingStop = kRightRingStart + hand::kLastPhalange,		//49+3 = 52 // 右手無名指結束
        kRightLittleStart = kRightRingStop,							//52 // 右手小指開始
        kRightLittleStop = kRightLittleStart + hand::kLastPhalange,	//52+3=55 // 右手小指結束
        kRightHandBoneStop = kRightLittleStop,						//55 // 右手手骨結束
        kLastHumanBone = kRightHandBoneStop							//55 // 人體骨骼結束
    };
	// 身體主幹部位的dof
    enum BodyDoF
    {
        kSpineFrontBack = 0,
        kSpineLeftRight,		// 1
        kSpineRollLeftRight,
        kChestFrontBack,		// 3
        kChestLeftRight,
        kChestRollLeftRight,	// 5
        kUpperChestFrontBack,
        kUpperChestLeftRight,	// 7
        kUpperChestRollLeftRight,
        kLastBodyDoF			// 9 身體一共九塊DOF
    };
	// 頭部谷歌的dof
    enum HeadDoF
    {
        kNeckFrontBack = 0,
        kNeckLeftRight,			// 1
        kNeckRollLeftRight,
        kHeadFrontBack,			// 3
        kHeadLeftRight,
        kHeadRollLeftRight,		// 5
        kLeftEyeDownUp,
        kLeftEyeLeftRight,		// 7
        kRightEyeDownUp,
        kRightEyeLeftRight,		// 9
        kJawDownUp,
        kJawLeftRight,			// 11
        kLastHeadDoF
    };
	// 大腿的dof
    enum LegDoF
    {
        kUpperLegFrontBack = 0,
        kUpperLegInOut,			// 1
        kUpperLegRollInOut,
        kLegCloseOpen,			// 3
        kLegRollInOut,
        kFootCloseOpen,			// 5
        kFootInOut,
        kToesUpDown,			// 7
        kLastLegDoF
    };
	// 上肢的dof
    enum ArmDoF
    {
        kShoulderDownUp = 0,    // 0  肩部上下肌肉
        kShoulderFrontBack,		// 1  肩部前后肌肉
        kArmDownUp,				//    手臂上下肌肉
        kArmFrontBack,			// 3  手臂前后肌肉
        kArmRollInOut,			//    手臂滚动内外肌肉
        kForeArmCloseOpen,		// 5  前臂开合肌肉
        kForeArmRollInOut,		//    前臂滚动内外肌肉
        kHandDownUp,			// 7  手部上下肌肉
        kHandInOut,				//	  手部内外肌肉
        kLastArmDoF				// 9
    };

	// 身體重點部分的dof
    enum HumanPartDoF
    {
        kBodyPart = 0,
        kHeadPart,				// 1
        kLeftLegPart,
        kRightLegPart,			// 3
        kLeftArmPart,
        kRightArmPart,			// 5
        kLeftThumbPart,
        kLeftIndexPart,			// 7
        kLeftMiddlePart,
        kLeftRingPart,			// 9
        kLeftLittlePart,
        kRightThumbPart,		// 11
        kRightIndexPart,
        kRightMiddlePart,		// 13
        kRightRingPart,
        kRightLittlePart,		// 15
        kLastHumanPartDoF
    };

	// 身體除了手指部位的全部dof dof的意思是關節的旋轉夾角，
    enum DoF
    {
        kBodyDoFStart = 0,
        kHeadDoFStart = kBodyDoFStart + kLastBodyDoF,			// 0+9 = 9
        kLeftLegDoFStart = kHeadDoFStart + kLastHeadDoF,		// 9+12=21
        kRightLegDoFStart = kLeftLegDoFStart + kLastLegDoF,		// 21+8= 29
        kLeftArmDoFStart = kRightLegDoFStart + kLastLegDoF,		// 29+8 = 37
        kRightArmDoFStart = kLeftArmDoFStart + kLastArmDoF,		// 37 + 9 = 46
        kLastDoF = kRightArmDoFStart + kLastArmDoF				// 46 + 9 = 55 身體除了手指部位的全部dof
    };
	// 身體全部部位的全部dof
    enum HumanDoF
    {
        kHumanDoFStart = 0,
        kHumanDoFStop = kHumanDoFStart + human_anim::human::kLastDoF,							// 55
        kHumanLeftHandDoFStart = kHumanDoFStop,												// 55 // 左手DOF開始
        kHumanLeftHandDoFStop = kHumanLeftHandDoFStart + human_anim::hand::s_DoFCount,			// 55+20 = 75 // 左手DOF結束
        kHumanRightHandDoFStart = kHumanLeftHandDoFStop,									// 75 // 右手DOF開始
        kHumanRightHandDoFStop = kHumanRightHandDoFStart + human_anim::hand::s_DoFCount,		// 75+20 =95 // 右手DOF結束
        kHumanLastDoF = kHumanRightHandDoFStop												// 95 // 身體所有DOF的結束
    };

    enum BodyTDoF
    {
        kSpineTDoF = 0,
        kChestTDoF,        // 1
        kUpperChestTDoF,   // 
        kLastBodyTDoF      // 3
    };

    enum HeadTDoF
    {
        kNeckTDoF = 0,
        kHeadTDoF,
        kLastHeadTDoF
    };

    enum LegTDoF
    {
        kUpperLegTDoF = 0,
        kLowerLegTDoF,
        kFootTDoF,
        kToesTDoF,
        kLastLegTDoF
    };

    enum ArmTDoF
    {
        kShoulderTDoF = 0,
        kUpperArmTDoF,
        kLowerArmTDoF,
        kHandTDoF,
        kLastArmTDoF
    };

    enum TDoF
    {
        kBodyTDoFStart = 0,
        kHeadTDoFStart = kBodyTDoFStart + kLastBodyTDoF,			// 0+3 = 3
        kLeftLegTDoFStart = kHeadTDoFStart + kLastHeadTDoF,			// 3+2 = 5
        kRightLegTDoFStart = kLeftLegTDoFStart + kLastLegTDoF,		// 5+4 = 9
        kLeftArmTDoFStart = kRightLegTDoFStart + kLastLegTDoF,		// 9+4 = 13
        kRightArmTDoFStart = kLeftArmTDoFStart + kLastArmTDoF,		// 13+4 = 17
        kLastTDoF = kRightArmTDoFStart + kLastArmTDoF				// 17+4 = 21
    };

    enum Goal
    {
        kLeftFootGoal,		//  左脚IK
        kRightFootGoal,		//  右脚IK 1
        kLeftHandGoal,		//  左手IK
        kRightHandGoal,		//  右手IK 3
        kLastGoal			//  IK量 4
    };

    struct GoalInfo
    {
        int32_t m_Index;
        int32_t m_TopIndex;
        int32_t m_MidIndex;
        int32_t m_EndIndex;
    };

    const static GoalInfo s_HumanGoalInfo[kLastGoal] =
    {
        { kLeftFoot, kLeftUpperLeg, kLeftLowerLeg, kLeftFoot },
        { kRightFoot, kRightUpperLeg, kRightLowerLeg, kRightFoot },
        { kLeftHand, kLeftUpperArm, kLeftLowerArm, kLeftHand },
        { kRightHand, kRightUpperArm, kRightLowerArm, kRightHand }
    };

    enum HumanPoseMaskInfo
    {
        kMaskRootIndex = 0,
        kMaskDoFStartIndex = kMaskRootIndex + 1,
        kMaskGoalStartIndex = kMaskDoFStartIndex + kLastDoF,
        kMaskLeftHand = kMaskGoalStartIndex + kLastGoal,
        kMaskRightHand = kMaskLeftHand + 1,
        kMaskTDoFStartIndex = kMaskRightHand + 1,
        kLastMaskIndex = kMaskTDoFStartIndex + kLastTDoF
    };

    typedef human_anim::bitset<kLastMaskIndex> HumanPoseMask;



    
	const LocalVector<Pair<String, String>>& get_bone_label();


    
	const HashMap<String, int>& get_bone_to_human_map();
	const HashMap<int, String>& get_human_to_bone_map();
    // 骨骼索引转dof索引
    int32_t MuscleFromBone(int32_t boneIndex, int32_t doFIndex);
    // dof 索引转骨骼索引
    int32_t BoneFromMuscle(int32_t doFIndex);

    int32_t BoneFromTDoF(int32_t doFIndex);
    int32_t BoneParentIndex(int32_t humanIndex);

    struct HumanGoal
    {

        HumanGoal() : m_WeightT(0.0f), m_WeightR(0.0f), m_HintT(0), m_HintWeightT(0), m_X(math::trsIdentity()) {}

        math::trsX m_X;
        float m_WeightT;
        float m_WeightR;

        math::float3 m_HintT;
        float m_HintWeightT;

    };

    struct HumanPose
    {

        HumanPose();

        math::trsX      m_RootX;
        math::float3    m_LookAtPosition;
        math::float4    m_LookAtWeight;

        hand::HandPose  m_LeftHandPose;
        hand::HandPose  m_RightHandPose;

        float           m_DoFArray[kLastDoF];
        math::float3    m_TDoFArray[kLastTDoF];
    };
    struct HumanAnimationKeyFrame {
        HumanAnimationKeyFrame() : time(0.0f) {
            reset();
        }
        double time;
        float dot_array[kLastDoF + hand::s_DoFCount + hand::s_DoFCount ];      

        void reset() {            
            memset(dot_array, 0, sizeof(float) * (kLastDoF + hand::s_DoFCount + hand::s_DoFCount ));
        }
        // 
        bool set_dof(const StringName& p_name, float p_value) {
            String str = p_name.str();

            int index = str.substr(3).to_int();
            if(index < 0 || index >= 95) {
                return false;
            }
            dot_array[ index] = p_value;
            return true;
        }
        
        static bool has_dof(const StringName& p_name) {
            String str = p_name.str();
            if(str.size() != 4) {
                return false;
            }

            int index = str[3] - 33;
            index *= 3;
            if(index < 0 || index >= kLastDoF - 2) {
                return false;
            }
            return true;
        }
        // 
        void blend(HumanAnimationKeyFrame & p_keyframe, Vector<uint8_t>& bone_mask,float p_weight) {

            for(int i = 0;i < kLastDoF ; i++) {
                int bone_index = BoneFromMuscle(i);
                if(bone_mask[bone_index]) {
                    dot_array[i] = math::lerp(dot_array[i], p_keyframe.dot_array[i], p_weight);
                }
            }

            for(int i = 0;i < hand::s_DoFCount; i++) {
                int bone_index = hand::s_HandBoneIndex[i];

                // 左手
                if(bone_mask[kLastBone + bone_index]) {
                    dot_array[kLastDoF + i] = math::lerp(dot_array[kLastDoF + i], p_keyframe.dot_array[kLastDoF + i], p_weight);
                }
                // 右手
                if(bone_mask[kLastBone + hand::s_BoneCount + bone_index]) {
                    dot_array[kLastDoF + hand::s_DoFCount + i] = math::lerp(dot_array[kLastDoF + hand::s_DoFCount + i], p_keyframe.dot_array[kLastDoF + hand::s_DoFCount + i], p_weight);
                }
            }

            
        }

    };

    
    
    struct Human
    {

        Human();

        void init(Skeleton3D* apSkeleton);

        void load(const Dictionary& p_dict);        
        void save(Dictionary& p_dict) ;


        void build_form_skeleton(Skeleton3D* apSkeleton);
        void setup_axes(Skeleton3D* apSkeleton) ;

        Animation* animation_to_dof(const HashMap<String, int> &human_bone_name_index, Animation* p_anim, const Dictionary& p_bone_map);

        void app_dof_to_skeleton(Skeleton3D* apSkeleton, HumanAnimationKeyFrame& p_keyframes);

        

        math::trsX              m_RootX;

        skeleton::HumanSkeleton   m_Skeleton;
        // 所有骨骼的局部rest 姿势变换
        LocalVector<math::trsX>   m_SkeletonLocalPose;
		
        hand::Hand               m_LeftHand;
        hand::Hand               m_RightHand;

        int32_t                 m_HumanBoneIndex[kLastBone];
        float                   m_HumanBoneMass[kLastBone];
        float                   m_RestDof[kLastDoF + hand::s_DoFCount + hand::s_DoFCount];
        float                   m_Scale;
        int32_t                 m_RootBonendex = -1;

        float                   m_ArmTwist;
        float                   m_ForeArmTwist;
        float                   m_UpperLegTwist;
        float                   m_LegTwist;

        float                   m_ArmStretch;
        float                   m_LegStretch;

        float                   m_FeetSpacing;

        bool                    m_HasLeftHand;
        bool                    m_HasRightHand;

        bool                    m_HasTDoF;
    };


	

    bool RequiredBone(uint32_t boneIndex);
    const char* BoneName(uint32_t boneIndex);
    String MuscleName(uint32_t boneIndex);


    void HumanAdjustMass(Human *human);
    void HumanSetupAxes(Human *human, skeleton::SkeletonPose const *skeletonPoseGlobal);
    void HumanCopyAxes(Human const *srcHuman, Human *human);
    void GetMuscleRange(Human const *apHuman, int32_t aDoFIndex, float &aMin, float &aMax);
    math::float4 AddAxis(Human const *human, int32_t index, math::float4 const &q);
    math::float4 RemoveAxis(Human const *human, int32_t index, const math::float4 &q);

    math::float3    HumanComputeBoneMassCenter(Human const *human, skeleton::SkeletonPose const *skeletonPoseGlobal, int32_t boneIndex);
    math::float3    HumanComputeMassCenter(Human const *human, skeleton::SkeletonPose const *skeletonPoseGlobal);
    float           HumanComputeMomentumOfInertia(Human const *human, skeleton::SkeletonPose const *skeletonPoseGlobal);
    math::float4    HumanComputeOrientation(Human const* human, skeleton::SkeletonPose const* apPoseGlobal);
    math::trsX      HumanComputeRootXform(Human const* human, skeleton::SkeletonPose const* apPoseGlobal);
    float           HumanGetFootHeight(Human const* human, bool left);
    math::float3    HumanGetFootBottom(Human const* human, bool left);
    math::float4    HumanGetGoalOrientationOffset(Goal goalIndex);
    math::float3    HumanGetHintPosition(Human const *apHuman, skeleton::SkeletonPose const *apSkeletonPoseGlobal, Goal goalIndex);
    math::trsX      HumanGetGoalXform(Human const *apHuman, skeleton::SkeletonPose const *apSkeletonPoseGlobal, Goal goalIndex);
    math::float3    HumanGetGoalPosition(Human const *apHuman, skeleton::SkeletonPose const *apSkeletonPose, Goal goalIndex);
    math::float4    HumanGetGoalRotation(Human const *apHuman, skeleton::SkeletonPose const *apSkeletonPose, Goal goalIndex);

    void HumanPoseMirror(HumanPose &pose, HumanPose const &poseA);

    void HumanPoseAdjustForMissingBones(Human const *apHuman, HumanPose *apHumanPose);

    void    RetargetFromTDoFBase(Human const *apHuman,
        skeleton::SkeletonPose *apSkeletonPoseGbl,
        math::float3 *apTDoFBase);

    void            RetargetFrom(Human *human,
        skeleton::SkeletonPose const *skeletonPose,
        HumanPose *humanPose,
        skeleton::SkeletonPose *skeletonPoseWsRef,
        skeleton::SkeletonPose *skeletonPoseWsGbl,
        skeleton::SkeletonPose *skeletonPoseWsLcl,
        skeleton::SkeletonPose *skeletonPoseWsWs,
        math::float3 const *tDoFBase);

    void            RetargetTo(Human *human,
        HumanPose const *humanPoseBase,
        HumanPose const *humanPose,
        const math::trsX &x,
        HumanPose *humanPoseOut,
        skeleton::SkeletonPose *skeletonPose,
        skeleton::SkeletonPose *skeletonPoseWs,
        bool adjustMissingBones = true);


    math::SetupAxesInfo const& GetAxeInfo(uint32_t index);
}// namespace human
}

