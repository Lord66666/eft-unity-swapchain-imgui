#include "EFTBase.hpp"

enum MonoOrStereoscopicEye : int32_t
{
	Left,
	Right,
	Mono
};

typedef PVOID (__fastcall* tGetPositionInjected)(Transform* instance, Vector3* OutPos);
tGetPositionInjected GetPositionInjected = nullptr;

typedef PVOID (__fastcall* tGetRotationInjected)(Transform* instance, Vector3* OutPos);
tGetRotationInjected GetRotationInjected = nullptr;

typedef Transform* (__fastcall* tGetComponentTransformInjected)(Component* instance);
tGetComponentTransformInjected GetComponentTransformInjected = nullptr;

typedef PVOID (__fastcall* tWorldToScreenPointInjected)(Camera* instance, Vector3* WorldPos, MonoOrStereoscopicEye eye, Vector3* ScreenPos);
tWorldToScreenPointInjected WorldToScreenPointInjected = nullptr;

typedef float (__fastcall* tGetFieldOfViewInjected)(Camera* instance);
tGetFieldOfViewInjected GetFieldOfViewInjected = nullptr;

void Transform::Position(Vector3* vec)
{
	uintptr_t procedure = Unity::UnityPlayer + 0x9960A0;

	if (!MemoryUtils::IsUserAddress(procedure))
		return;

	GetPositionInjected = (tGetPositionInjected) procedure;

	GetPositionInjected(this, vec);
}

void Transform::Rotation(Vector3* vec)
{
	uintptr_t procedure = Unity::UnityPlayer + 0x9952D0;

	if (!MemoryUtils::IsUserAddress(procedure))
		return;

	GetRotationInjected = (tGetRotationInjected)procedure;

	GetRotationInjected(this, vec);
}

Transform* Component::GetComponentTransform()
{
	uintptr_t procedure = Unity::UnityPlayer + 0x936D20; // The component is not attached to any game object!

	if (!MemoryUtils::IsUserAddress(procedure))
		return nullptr;

	GetComponentTransformInjected = (tGetComponentTransformInjected) procedure;

	return GetComponentTransformInjected(this);
}

Vector3 Camera::WorldToScreen(Vector3 WorldPos)
{
	if (!EFTCore::m_FPSCamera)
		return Vector3();

	Matrix4x4 temp_matrix;
	MemoryUtils::ReadBuffer(EFTCore::m_FPSCamera + 0xDC, &temp_matrix, sizeof(temp_matrix));

	auto CameraMatrix = temp_matrix;
	auto ViewMatrix = CameraMatrix.Transpose();

	Vector3 translation = { ViewMatrix[3][0], ViewMatrix[3][1], ViewMatrix[3][2] };
	Vector3 up = { ViewMatrix[1][0], ViewMatrix[1][1], ViewMatrix[1][2] };
	Vector3 right = { ViewMatrix[0][0], ViewMatrix[0][1], ViewMatrix[0][2] };

	auto w = translation.Dot(WorldPos) + ViewMatrix[3][3];

	if (w < 0.1f)
		return Vector3();

	auto x = right.Dot(WorldPos) + ViewMatrix[0][3];
	auto y = up.Dot(WorldPos) + ViewMatrix[1][3];

	Vector3 ScreenPos = Vector3();

	ScreenPos.x = (Unity::displayWidth / 2) * (1.f + x / w);
	ScreenPos.y = (Unity::displayHeight / 2) * (1.f - y / w);
	ScreenPos.z = 0.0f;

	return ScreenPos;
}

Vector3 Camera::WorldToScreenPoint(Vector3* WorldPos)
{
	Vector3 ScreenPos = Vector3();

	uintptr_t procedure = Unity::UnityPlayer + 0x928AF0;

	if (!MemoryUtils::IsUserAddress(procedure))
		return Vector3();

	WorldToScreenPointInjected = (tWorldToScreenPointInjected) procedure;

	WorldToScreenPointInjected(this, WorldPos, Mono, &ScreenPos);

	if (ScreenPos.z < 0.098f)
		return Vector3();

	return ScreenPos;
}

float Camera::GetFieldOfView()
{
	float FieldOfView = 0.0f;

	uintptr_t procedure = Unity::UnityPlayer + 0x92BE40; // Camera::get_fieldOfView

	if (!MemoryUtils::IsUserAddress(procedure))
		return FieldOfView;

	GetFieldOfViewInjected = (tGetFieldOfViewInjected)procedure;

	FieldOfView = GetFieldOfViewInjected(this);

	return FieldOfView;
}

Transform* Player::GetBoneTransform(int32_t index)
{
	auto playerBody = this->GetPlayerBody();

	if (!MemoryUtils::IsAddressValid(playerBody))
		return nullptr;

	auto skeletonJoint = playerBody->GetSkeletonRootJoint();

	if (!MemoryUtils::IsAddressValid(skeletonJoint))
		return nullptr;

	auto boneListner = skeletonJoint->GetBoneListner();

	if (!MemoryUtils::IsAddressValid(boneListner))
		return nullptr;

	auto boneArray = boneListner->GetBoneArray();

	if (!MemoryUtils::IsAddressValid(boneArray))
		return nullptr;

	Transform* boneTransform = nullptr;

	if (index == 228)
		boneTransform = *reinterpret_cast<Transform**>(uintptr_t(boneArray) + 0x440);
	else
		boneTransform = *reinterpret_cast<Transform**>(uintptr_t(boneArray) + (0x20 + (index * 8)));

	//const auto boneTransform = *reinterpret_cast<Transform**>(uintptr_t(boneArray) + (0x20 + (index * 8)));

	if (!MemoryUtils::IsAddressValid(boneTransform))
		return nullptr;

	return boneTransform;
}

Transform* Player::GetHumanBoneTransform(HumanBoneType type)
{
	auto playerBody = this->GetPlayerBody();

	if (!MemoryUtils::IsAddressValid(playerBody))
		return nullptr;

	auto skeletonJoint = playerBody->GetSkeletonRootJoint();

	if (!MemoryUtils::IsAddressValid(skeletonJoint))
		return nullptr;

	List<Transform>* humanBones = skeletonJoint->Values();

	if (!MemoryUtils::IsAddressValid(humanBones))
		return nullptr;

	auto boneList = humanBones->listBase;
	int boneCount = humanBones->itemCount;

	if (boneCount <= 0 || !boneList)
		return nullptr;

	Transform* humanBone = boneList->entries[type];

	if (!MemoryUtils::IsAddressValid(humanBone))
		return nullptr;

	return humanBone;
}

bool Player::IsScav()
{
	auto profile = this->GetPlayerProfile();
	auto info = profile->m_PlayerInfo;

	return info->RegistrationDate > 0 && info->Side == 4;
}

bool Player::IsBot()
{
	auto profile = this->GetPlayerProfile();
	auto info = profile->m_PlayerInfo;

	return info->RegistrationDate <= 0;
}

Vector3 Player::GetHumanBonePosition3D(HumanBoneType type)
{
	Vector3 bonePos = Vector3();

	auto boneTransform = this->GetBoneTransform(type);

	if (!MemoryUtils::IsAddressValid(boneTransform) || boneTransform == nullptr)
		return bonePos;

	boneTransform->Position(&bonePos);

	return bonePos;
}

Vector3 Player::GetHumanBonePosition(HumanBoneType type)
{
	Vector3 bonePos = Vector3();

	auto boneTransform = this->GetBoneTransform(type);

	if (!MemoryUtils::IsAddressValid(boneTransform))
		return bonePos;

	boneTransform->Position(&bonePos);

	Vector3 bonePos2d = EFTCore::m_Camera->WorldToScreen(bonePos);

	if (bonePos2d.Zero())
		return Vector3();

	return bonePos2d;
}

Transform* Player::GetFireportTransform(PlayerBoneType type)
{
	return this->GetPlayerBones()->GetBoneByType(type)->Original();
}

Vector3 Player::GetBonePosition3D(PlayerBoneType type)
{
	Vector3 bonePos = Vector3();

	if (type == PlayerBoneType::Neck)
		this->GetPlayerBones()->Neck()->Position(&bonePos);
	else if (type == PlayerBoneType::Spine1)
		this->GetPlayerBones()->Spine1()->Position(&bonePos);
	else
		this->GetPlayerBones()->GetBoneByType(type)->Original()->Position(&bonePos);

	if (bonePos.Zero())
		return Vector3();

	return bonePos;
}

Vector3 Player::GetBonePosition(PlayerBoneType type)
{
	Vector3 bonePos = Vector3();

	if (type == PlayerBoneType::Neck)
		this->GetPlayerBones()->Neck()->Position(&bonePos);
	else if (type == PlayerBoneType::Spine1)
		this->GetPlayerBones()->Spine1()->Position(&bonePos);
	else
		this->GetPlayerBones()->GetBoneByType(type)->Original()->Position(&bonePos);

	Vector3 bonePos2d = EFTCore::m_Camera->WorldToScreen(bonePos);

	if (bonePos2d.Zero())
		return Vector3();

	return bonePos2d;

	/*bonePos = EFTCore::m_Camera->WorldToScreenPoint(&bonePos);
	
	if (bonePos.Zero())
		return Vector3();

	bonePos.y = Unity::displayHeight - bonePos.y;

	return bonePos;*/
}

Vector3 Player::GetCameraPosition()
{
	Vector3 result = Vector3();

	if (EFTCore::m_Camera == nullptr)
		return result;

	EFTCore::m_Camera->GetComponentTransform()->Position(&result);

	return result;
}

Vector3 Player::GetHeadPosition()
{
	Vector3 result = Vector3();
	
	this->GetPlayerBones()->Head()->Original()->Position(&result);
	
	result = EFTCore::m_Camera->WorldToScreenPoint(&result);
	
	if (result.Zero())
		return Vector3();

	result.y = Unity::displayHeight - result.y;
	
	return result;
}

Vector3 Player::GetScreenPosition()
{
	Vector3 result = Vector3();

	this->GetPlayerBones()->BodyTransform()->Original()->Position(&result);
	
	result = EFTCore::m_Camera->WorldToScreenPoint(&result);
	
	if (result.Zero())
		return Vector3();

	result.y = Unity::displayHeight - result.y;
	
	return result;
}

bool Player::IsOnScreen()
{
	if (!EFTCore::m_Camera)
		return false;

	auto screenPoint = this->GetScreenPosition();
	
	return screenPoint.z > 0.01f && screenPoint.x > -5.f && screenPoint.y > -5.f && screenPoint.x < Unity::displayWidth && screenPoint.y < Unity::displayHeight;
}

float Player::GetDistance()
{
	if (EFTCore::m_Camera == nullptr)
		return 0;

	Vector3 pos = this->GetPosition();
	Vector3 camera_pos = this->GetCameraPosition();
	
	return pos.Distance(camera_pos);
}

float Player::GetLocalDistance(Vector3 TargetPos)
{
	if (EFTCore::m_Camera == nullptr)
		return 0;

	if (TargetPos.Zero())
		return 0;

	Vector3 pos = this->GetPosition();

	return TargetPos.Distance(pos);
}