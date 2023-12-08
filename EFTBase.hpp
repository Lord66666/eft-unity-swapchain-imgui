#include "EFTCore.hpp"

struct Player;

struct Transform
{
	void Position(Vector3* vec);
    void Rotation(Vector3* vec);
};

struct Component
{
	Transform* GetComponentTransform();
};

struct Camera : public Component
{
	Vector3 WorldToScreenPoint(Vector3* WorldPos);
    Vector3 WorldToScreen(Vector3 WorldPos);

    float GetFieldOfView();
};

class UnityString
{
public:
    char pad_0000[0x10];
    int32_t size;
    char pad_0018[0x14];
    wchar_t base[256];
};

struct LocalGameWorld
{
	List<Player>* RegisteredPlayers()
	{
		return MemoryUtils::Read<List<Player>*>((uintptr_t)this + 0x80);	// 0x80 - Registered, 0xA0 - All
	}
};

struct BifacialTransform : public Component
{
	Transform* Original()
	{
		return MemoryUtils::Read<Transform*>((uintptr_t)this + 0x10);
	}
};

enum HumanBoneType : int32_t
{
    ROOT = 0,

    R_PELVIS = 20,
    R_KNEE = 22,
    R_FOOT = 23,

    R_HAND0 = 111,
    R_HAND1 = 112,
    R_HAND2 = 113,
    R_HAND3 = 114,

    L_PELVIS = 15,
    L_KNEE = 17,
    L_FOOT = 18,

    L_HAND0 = 90,
    L_HAND1 = 91,
    L_HAND2 = 92,
    L_HAND3 = 93,

    SPINE = 36,
    PELVIS = 29,

    NECK = 132,
    HEAD = 133,
    NECK_MODEL = 228
};

enum PlayerBoneType : int32_t
{
	Head,
	Neck,
	LeftShoulder,
	RightShoulder,
	Ribcage = 4,
	LeftThigh2,
	RightThigh2,
	WeaponRoot,
	Body,
	Fireport,
	Pelvis,
	LeftThigh1,
	RightThigh1,
	Spine1,
	Spine
};

struct PlayerBones
{
	BifacialTransform* BodyTransform()
	{
		return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x108);
	}

	BifacialTransform* Head()
	{
		return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xE0);
	}

	Transform* Neck()
	{
		return MemoryUtils::Read<Transform*>((uintptr_t)this + 0x30);
	}

    Transform* LootRaycastOrigin()
    {
        return MemoryUtils::Read<Transform*>((uintptr_t)this + 0x70);
    }

    void SetLootRaycast(Transform* lootTransform)
    {
        MemoryUtils::Write<Transform*>((uintptr_t)this + 0x70, lootTransform);
    }

	Transform* Spine1()
	{
		return MemoryUtils::Read<Transform*>((uintptr_t)this + 0x178);
	}

	BifacialTransform* GetBoneByType(PlayerBoneType type)
	{
		switch (type)
		{
		case PlayerBoneType::Head:          return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xE0);
		case PlayerBoneType::LeftShoulder:  return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xE8);
		case PlayerBoneType::RightShoulder: return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xF0);
		case PlayerBoneType::Ribcage:       return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xD8);
		case PlayerBoneType::LeftThigh2:    return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xF8);
		case PlayerBoneType::RightThigh2:   return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x100);
		case PlayerBoneType::WeaponRoot:    return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xD0);
		case PlayerBoneType::Body:          return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x108);
		case PlayerBoneType::Fireport:      return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x140);
		case PlayerBoneType::Pelvis:        return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x118);
		case PlayerBoneType::LeftThigh1:    return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x120);
		case PlayerBoneType::RightThigh1:   return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x128);
		case PlayerBoneType::Spine:         return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0x130);

		default:
			return MemoryUtils::Read<BifacialTransform*>((uintptr_t)this + 0xE0);
		}
	}
};

struct BoneArray
{
    char pad_0000[8];
};

struct BoneListner
{
    BoneArray* GetBoneArray()
    {
        return MemoryUtils::Read<BoneArray*>((uintptr_t)this + 0x10);
    }
};

struct Skeleton
{
	List<Transform>* Values()
	{
		return MemoryUtils::Read<List<Transform>*>((uintptr_t)this + 0x28);	// 0x28
	}

    BoneListner* GetBoneListner()
    {
        return MemoryUtils::Read<BoneListner*>((uintptr_t)this + 0x28);
    }
};

struct PlayerBody
{
	Skeleton* GetSkeletonRootJoint()
	{
		return MemoryUtils::Read<Skeleton*>((uintptr_t)this + 0x28);
	}
};

enum BodyParts: int32_t
{
    BHEAD = 0,
    THORAX,
    STOMACH,
    LEFTARM,
    RIGHTARM,
    LEFTLEG,
    RIGHTLEG,
    NUM_BODY_PARTS
};

class Health
{
public:
    char pad_0000[0x10];
    float Health;
    float HealthMax;
};

class HealthContainer
{
public:
    char pad_0000[0x10];
    Health* m_Health;
};

class BodyPartContainer
{
public:
    HealthContainer* m_BodyPart;
    char pad_0038[16];
};

class BodyController
{
public:
    char pad_0000[0x30];
    BodyPartContainer m_BodyParts[NUM_BODY_PARTS];
};

class HealthBody
{
public:
    char pad_0000[0x18];
    BodyController* m_BodyController;
};

class HealthController
{
public:
    char pad_0000[0x50];
    HealthBody* m_HealthBody;
};

class PlayerInfo
{
public:
    char pad_0000[0x10];
    UnityString* m_pPlayerName;
    char pad_0018[0x58];
    int32_t Side;
    int32_t RegistrationDate;
};

class PlayerProfile
{
public:
    char pad_0000[0x28];
    PlayerInfo* m_PlayerInfo;
};

class PlayerSpring
{
public:
    Transform* Fireport()
    {
        return MemoryUtils::Read<Transform*>((uintptr_t)this + 0x88);
    }
};

class MovementContext
{
public:
    char pad_0000[0x20C];
    Vector2 ViewAngles;
};

class ShotEffector
{
public:
    char pad_0010[0x38];
    Vector2 RecoilStrengthXy;
    char pad_0018[0x40];
    Vector2 RecoilStrengthZ;
    char pad_0020[0x68];
    float Intensity;
};

class BreathEffector
{
public:
    char pad_0000[0xA0];
    bool IsAiming;
    char pad_0091[0xA4];
    float Intensity;
};

class CharacterController
{
public:
    char pad_0020[0xE0];
    Vector3 Velocity;
};

class Physical
{

};

class AmmoTemplate
{
public:
    char pad_0000[0x10];
    UnityString* m_ShortName;
};

class WeaponTemplate
{
public:
    char pad_0000[0x18];
    UnityString* m_ShortName;

    AmmoTemplate* GetAmmoTemplate()
    {
        return MemoryUtils::Read<AmmoTemplate*>((uintptr_t)this + 0x158);
    }
};

class Weapon
{
public:
    WeaponTemplate* GetWeaponTemplate()
    {
        return MemoryUtils::Read<WeaponTemplate*>((uintptr_t)this + 0x40);
    }
};

class HandsController
{
public:
    Weapon* GetWeapon()
    {
        return MemoryUtils::Read<Weapon*>((uintptr_t)this + 0x60);
    }
};

class FirearmController
{
public:
    Weapon* GetWeapon()
    {
        return MemoryUtils::Read<Weapon*>((uintptr_t)this + 0x60);
    }
};

class ProceduralWeaponAnimation
{
public:
    char pad_0000[0x28];
    BreathEffector* m_Breath;
    char pad_0010[0x48];
    ShotEffector* m_Shooting;
    char pad_0018[0x100];
    uint32_t mask;
    char pad_0020[0x104];
    Vector3 Sway;

    FirearmController* GetFirearmController()
    {
        return MemoryUtils::Read<FirearmController*>((uintptr_t)this + 0x80);
    }

    PlayerSpring* GetHandsContainer()
    {
        return MemoryUtils::Read<PlayerSpring*>((uintptr_t)this + 0x18);
    }
};

struct Player
{
	bool IsLocal()
	{
		return MemoryUtils::Read<bool>((uintptr_t)this + 0x7F7) == true;
	}

    Physical* GetPhysical()
    {
        return MemoryUtils::Read<Physical*>((uintptr_t)this + 0x4C8);
    }

	PlayerBody* GetPlayerBody()
	{
		return MemoryUtils::Read<PlayerBody*>((uintptr_t)this + 0xA8);
	}

	PlayerBones* GetPlayerBones()
	{
		return MemoryUtils::Read<PlayerBones*>((uintptr_t)this + 0x550);
	}

    PlayerProfile* GetPlayerProfile()
    {
        return MemoryUtils::Read<PlayerProfile*>((uintptr_t)this + 0x4B8);
    }

    HealthController* GetHealthController()
    {
        return MemoryUtils::Read<HealthController*>((uintptr_t)this + 0x4F0);
    }

    CharacterController* GetCharacterController()
    {
        return MemoryUtils::Read<CharacterController*>((uintptr_t)this + 0x28);
    }

    MovementContext* GetMovementContext()
    {
        return MemoryUtils::Read<MovementContext*>((uintptr_t)this + 0x40);
    }

    ProceduralWeaponAnimation* GetProceduralWeaponAnimation()
    {
        return MemoryUtils::Read<ProceduralWeaponAnimation*>((uintptr_t)this + 0x190);
    }

    HandsController* GetHandsController()
    {
        return MemoryUtils::Read<HandsController*>((uintptr_t)this + 0x508);
    }

	Vector3 GetPosition()
	{
		Vector3 result = Vector3();
		this->GetPlayerBones()->BodyTransform()->Original()->Position(&result);
		return result;
	}

    bool IsScav();
    bool IsBot();

    Vector3 GetHumanBonePosition3D(HumanBoneType type);
    Vector3 GetHumanBonePosition(HumanBoneType type);
	
    Transform* GetBoneTransform(int32_t index);

    Transform* GetHumanBoneTransform(HumanBoneType type);
    Transform* GetFireportTransform(PlayerBoneType type);

    Vector3 GetBonePosition3D(PlayerBoneType type);
	Vector3 GetBonePosition(PlayerBoneType type);

    Vector3 GetCameraPosition();

	Vector3 GetHeadPosition();
	Vector3 GetScreenPosition();

	bool IsOnScreen();
	float GetDistance();
    float GetLocalDistance(Vector3 TargetPos);
};