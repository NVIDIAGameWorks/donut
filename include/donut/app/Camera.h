/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <unordered_map>
#include <array>
#include <optional>
#include <memory>

#include <donut/core/math/math.h>

#define GLFW_INCLUDE_NONE // Do not include any OpenGL headers
#include <GLFW/glfw3.h>

namespace donut::engine
{
    class PlanarView;
    class SceneCamera;
}

namespace donut::app
{

    // A camera with position and orientation. Methods for moving it come from derived classes.
    class BaseCamera
    {
    public:
        virtual void KeyboardUpdate(int key, int scancode, int action, int mods) { }
        virtual void MousePosUpdate(double xpos, double ypos) { }
        virtual void MouseButtonUpdate(int button, int action, int mods) { }
        virtual void MouseScrollUpdate(double xoffset, double yoffset) { }
        virtual void JoystickButtonUpdate(int button, bool pressed) { }
        virtual void JoystickUpdate(int axis, float value) { }
        virtual void Animate(float deltaT) { }
        virtual ~BaseCamera() = default;

        void SetMoveSpeed(float value) { m_MoveSpeed = value; }
        void SetRotateSpeed(float value) { m_RotateSpeed = value; }

        [[nodiscard]] const dm::affine3& GetWorldToViewMatrix() const { return m_MatWorldToView; }
        [[nodiscard]] const dm::affine3& GetTranslatedWorldToViewMatrix() const { return m_MatTranslatedWorldToView; }
        [[nodiscard]] const dm::float3& GetPosition() const { return m_CameraPos; }
        [[nodiscard]] const dm::float3& GetDir() const { return m_CameraDir; }
        [[nodiscard]] const dm::float3& GetUp() const { return m_CameraUp; }

    protected:
        // This can be useful for derived classes while not necessarily public, i.e., in a third person
        // camera class, public clients cannot direct the gaze point.
        void BaseLookAt(dm::float3 cameraPos, dm::float3 cameraTarget, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });
        void UpdateWorldToView();

        dm::affine3 m_MatWorldToView = dm::affine3::identity();
        dm::affine3 m_MatTranslatedWorldToView = dm::affine3::identity();

        dm::float3 m_CameraPos   = 0.f;   // in worldspace
        dm::float3 m_CameraDir   = dm::float3(1.f, 0.f, 0.f); // normalized
        dm::float3 m_CameraUp    = dm::float3(0.f, 1.f, 0.f); // normalized
        dm::float3 m_CameraRight = dm::float3(0.f, 0.f, 1.f); // normalized

        float m_MoveSpeed = 1.f;      // movement speed in units/second
        float m_RotateSpeed = .005f;  // mouse sensitivity in radians/pixel
    };

    class FirstPersonCamera : public BaseCamera
    {
    public:
        void KeyboardUpdate(int key, int scancode, int action, int mods) override;
        void MousePosUpdate(double xpos, double ypos) override;
        void MouseButtonUpdate(int button, int action, int mods) override;
        void Animate(float deltaT) override;
        void AnimateSmooth(float deltaT);

        void LookAt(dm::float3 cameraPos, dm::float3 cameraTarget, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });
        void LookTo(dm::float3 cameraPos, dm::float3 cameraDir, dm::float3 cameraUp = dm::float3{ 0.f, 1.f, 0.f });

    private:
        std::pair<bool, dm::affine3> AnimateRoll(dm::affine3 initialRotation);
        std::pair<bool, dm::float3> AnimateTranslation(float deltaT);
        void UpdateCamera(dm::float3 cameraMoveVec, dm::affine3 cameraRotation);

        dm::float2 mousePos;
        dm::float2 mousePosPrev;
        // fields used only for AnimateSmooth()
        dm::float2 mousePosDamp;
        bool isMoving = false;

        typedef enum
        {
            MoveUp,
            MoveDown,
            MoveLeft,
            MoveRight,
            MoveForward,
            MoveBackward,

            YawRight,
            YawLeft,
            PitchUp,
            PitchDown,
            RollLeft,
            RollRight,

            SpeedUp,
            SlowDown,

            KeyboardControlCount,
        } KeyboardControls;

        typedef enum
        {
            Left,
            Middle,
            Right,

            MouseButtonCount,
            MouseButtonFirst = Left,
        } MouseButtons;

        const std::unordered_map<int, int> keyboardMap = {
            { GLFW_KEY_Q, KeyboardControls::MoveDown },
            { GLFW_KEY_E, KeyboardControls::MoveUp },
            { GLFW_KEY_A, KeyboardControls::MoveLeft },
            { GLFW_KEY_D, KeyboardControls::MoveRight },
            { GLFW_KEY_W, KeyboardControls::MoveForward },
            { GLFW_KEY_S, KeyboardControls::MoveBackward },
            { GLFW_KEY_LEFT, KeyboardControls::YawLeft },
            { GLFW_KEY_RIGHT, KeyboardControls::YawRight },
            { GLFW_KEY_UP, KeyboardControls::PitchUp },
            { GLFW_KEY_DOWN, KeyboardControls::PitchDown },
            { GLFW_KEY_Z, KeyboardControls::RollLeft },
            { GLFW_KEY_C, KeyboardControls::RollRight },
            { GLFW_KEY_LEFT_SHIFT, KeyboardControls::SpeedUp },
            { GLFW_KEY_RIGHT_SHIFT, KeyboardControls::SpeedUp },
            { GLFW_KEY_LEFT_CONTROL, KeyboardControls::SlowDown },
            { GLFW_KEY_RIGHT_CONTROL, KeyboardControls::SlowDown },
        };

        const std::unordered_map<int, int> mouseButtonMap = {
            { GLFW_MOUSE_BUTTON_LEFT, MouseButtons::Left },
            { GLFW_MOUSE_BUTTON_MIDDLE, MouseButtons::Middle },
            { GLFW_MOUSE_BUTTON_RIGHT, MouseButtons::Right },
        };

        std::array<bool, KeyboardControls::KeyboardControlCount> keyboardState = { false };
        std::array<bool, MouseButtons::MouseButtonCount> mouseButtonState = { false };
    };

    class ThirdPersonCamera : public BaseCamera
    {
    public:
        void KeyboardUpdate(int key, int scancode, int action, int mods) override;
        void MousePosUpdate(double xpos, double ypos) override;
        void MouseButtonUpdate(int button, int action, int mods) override;
        void MouseScrollUpdate(double xoffset, double yoffset) override;
        void JoystickButtonUpdate(int button, bool pressed) override;
        void JoystickUpdate(int axis, float value) override;
        void Animate(float deltaT) override;

        dm::float3 GetTargetPosition() const { return m_TargetPos; }
        void SetTargetPosition(dm::float3 position) { m_TargetPos = position; }

        float GetDistance() const { return m_Distance; }
        void SetDistance(float distance) { m_Distance = distance; }
        
        float GetRotationYaw() const { return m_Yaw; }
        float GetRotationPitch() const { return m_Pitch; }
        void SetRotation(float yaw, float pitch);

        float GetMaxDistance() const { return m_MaxDistance; }
        void SetMaxDistance(float value) { m_MaxDistance = value; }

        void SetView(const engine::PlanarView& view);

        void LookAt(dm::float3 cameraPos, dm::float3 cameraTarget);
        void LookTo(dm::float3 cameraPos, dm::float3 cameraDir,
            std::optional<float> targetDistance = std::optional<float>());
        
    private:
        void AnimateOrbit(float deltaT);
        void AnimateTranslation(const dm::float3x3& viewMatrix);

        // View parameters to derive translation amounts
        dm::float4x4 m_ProjectionMatrix = dm::float4x4::identity();
        dm::float4x4 m_InverseProjectionMatrix = dm::float4x4::identity();
        dm::float2 m_ViewportSize = dm::float2::zero();

        dm::float2 m_MousePos = 0.f;
        dm::float2 m_MousePosPrev = 0.f;

        dm::float3 m_TargetPos = 0.f;
        float m_Distance = 30.f;
        
        float m_MinDistance = 0.f;
        float m_MaxDistance = std::numeric_limits<float>::max();
        
        float m_Yaw = 0.f;
        float m_Pitch = 0.f;
        
        float m_DeltaYaw = 0.f;
        float m_DeltaPitch = 0.f;
        float m_DeltaDistance = 0.f;

        typedef enum
        {
            HorizontalPan,

            KeyboardControlCount,
        } KeyboardControls;

        const std::unordered_map<int, int> keyboardMap = {
            { GLFW_KEY_LEFT_ALT, KeyboardControls::HorizontalPan },
        };

        typedef enum
        {
            Left,
            Middle,
            Right,

            MouseButtonCount
        } MouseButtons;

        std::array<bool, KeyboardControls::KeyboardControlCount> keyboardState = { false };
        std::array<bool, MouseButtons::MouseButtonCount> mouseButtonState = { false };
    };

    // The SwitchableCamera class provides a combination of first-person, third-person, and scene graph cameras.
    // The active camera can be chosen from those options, and switches between the camera types
    // can preserve the current camera position and orientation when switching to user-controllable types.
    class SwitchableCamera
    {
    public:
        // Returns the active user-controllable camera (first-person or third-person),
        // or nullptr if a scene camera is active.
        BaseCamera* GetActiveUserCamera();

        // A constant version of GetActiveUserCamera.
        BaseCamera const* GetActiveUserCamera() const;

        bool IsFirstPersonActive() const { return !m_SceneCamera && m_UseFirstPerson; }
        bool IsThirdPersonActive() const { return !m_SceneCamera && !m_UseFirstPerson; }
        bool IsSceneCameraActive() const { return !!m_SceneCamera; }

        // Always returns the first-person camera object.
        FirstPersonCamera& GetFirstPersonCamera() { return m_FirstPerson; }

        // Always returns the third-person camera object.
        ThirdPersonCamera& GetThirdPersonCamera() { return m_ThirdPerson; }

        // Returns the active scene camera object, or nullptr if a user camera is active.
        std::shared_ptr<engine::SceneCamera>& GetSceneCamera() { return m_SceneCamera; }

        // Returns the view matrix for the currently active camera.
        dm::affine3 GetWorldToViewMatrix() const;

        // Fills out the projection parameters from a scene camera, if there is a perspective camera active.
        // Returns true when the parameters were filled, false if no such camera available.
        // In the latter case, the input values for the parameters are left unmodified.
        bool GetSceneCameraProjectionParams(float& verticalFov, float& zNear) const;

        // Switches to the first-person camera, optionally copying the position and direction
        // from another active camera type.
        void SwitchToFirstPerson(bool copyView = true);

        // Switches to the third-person camera, optionally copying the position and direction
        // from another active camera type. When 'targetDistance' is specified, it overrides the current
        // distance stored in the third-person camera. Suggested use is to determine the distance to the
        // object in the center of the view at the time of the camera switch and use that distance.
        void SwitchToThirdPerson(bool copyView = true, std::optional<float> targetDistance = std::optional<float>());

        // Switches to the provided scene graph camera that must not be a nullptr.
        // The user-controllable cameras are not affected by this call.
        void SwitchToSceneCamera(std::shared_ptr<engine::SceneCamera> const& sceneCamera);

        // The following methods direct user input events to the active user camera
        // and return 'true' if such camera is active.

        bool KeyboardUpdate(int key, int scancode, int action, int mods);
        bool MousePosUpdate(double xpos, double ypos);
        bool MouseButtonUpdate(int button, int action, int mods);
        bool MouseScrollUpdate(double xoffset, double yoffset);
        bool JoystickButtonUpdate(int button, bool pressed);
        bool JoystickUpdate(int axis, float value);

        // Calls 'Animate' on the active user camera.
        // It is necessary to call Animate on the camera once per frame to correctly update its state.
        void Animate(float deltaT);

    private:
        FirstPersonCamera m_FirstPerson;
        ThirdPersonCamera m_ThirdPerson;
        std::shared_ptr<engine::SceneCamera> m_SceneCamera;
        bool m_UseFirstPerson = false;
    };
}
