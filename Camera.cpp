#include "Camera.h"


Camera::Camera()
{
}

Camera::~Camera()
{
}


void Camera::Initialize() {

	scale_ = { 1, 1, 1 };
	translate_ = { 0.0f, 0.0f, -10.0f };
	rotate_ = { 0.0f, 0.0f, 0.0f };
	worldMatrix_ = MakeAffineMatrix(scale_, rotate_, translate_);

	//マウスの位置
	mousePos_ = {0, 0};
	preMousePos_ = {0, 0};
}

void Camera::Update() {

	ImGui::Begin("Camera");
	ImGui::SliderFloat3("rotate", &rotate_.x, -2 * M_PI, 2 * M_PI);
	ImGui::SliderFloat3("translate", &translate_.x, -100, 100);
	ImGui::End();

	Vector3 prePos = translate_;
	preMousePos_ = mousePos_;
	ImVec2 mousePos = ImGui::GetMousePos();
	mousePos_.x = int(mousePos.x);
	mousePos_.y = int(mousePos.y);

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		Vector2Int mouseAmount = mousePos_ - preMousePos_;

		rotate_.x += mouseAmount.y * 0.01f;
		rotate_.y += mouseAmount.x * 0.01f;
	}

	Vector3 cameraVelocity = { 0.0f, 0.0f, 0.0f };
	if (ImGui::IsKeyDown(ImGuiKey_A)) {
		cameraVelocity.x += -0.2f;
	}
	if (ImGui::IsKeyDown(ImGuiKey_D)) {
		cameraVelocity.x += 0.2f;
	}
	if (ImGui::IsKeyDown(ImGuiKey_W)) {
		cameraVelocity.z += 0.2f;
	}
	if (ImGui::IsKeyDown(ImGuiKey_S)) {
		cameraVelocity.z += -0.2f;
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		Vector2Int mouseAmount = mousePos_ - preMousePos_;

		cameraVelocity.x += -mouseAmount.x * 0.1f;
		cameraVelocity.y += mouseAmount.y * 0.1f;
	}

	//マウスホイールの処理方法がわかったらやる
	/*float wheel = ;
	if (wheel != 0) {
		cameraVelocity.z += wheel * 0.01f;
	}*/

	translate_ += TransformNormal(cameraVelocity, worldMatrix_);

	if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
		translate_.y = prePos.y;
	}

	worldMatrix_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);
}