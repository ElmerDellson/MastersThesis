#include "stdafx.h"
#include "Thesis.h"

//---------------------------------------------------------------------------------------------------------------------
// Handle camera manipulation.
//
void Thesis::OnButtonDown(UINT32 lParam) {
	nv_helpers_dx12::CameraManip.setMousePosition(-GET_X_LPARAM(lParam),
		-GET_Y_LPARAM(lParam));
}

void Thesis::OnMouseMove(UINT8 wParam, UINT32 lParam) {
	using nv_helpers_dx12::Manipulator;
	Manipulator::Inputs inputs;
	inputs.lmb = wParam & MK_LBUTTON;
	inputs.mmb = wParam & MK_MBUTTON;
	inputs.rmb = wParam & MK_RBUTTON;
	if (!inputs.lmb && !inputs.rmb && !inputs.mmb)
		return; //< No mouse button pressed.

	inputs.ctrl = GetAsyncKeyState(VK_CONTROL);
	inputs.shift = GetAsyncKeyState(VK_SHIFT);
	inputs.alt = GetAsyncKeyState(VK_MENU);

	CameraManip.mouseMove(-GET_X_LPARAM(lParam), -GET_Y_LPARAM(lParam), inputs);
}

void Thesis::OnKeyUp(UINT8 key) {
	if (key == VK_SPACE)
		m_animate = !m_animate;
	if (key == 0x41 /*A*/)
		m_MSAAOn = !m_MSAAOn;
	if (key == 0x46 /*F*/)
		m_interpolateProbes = !m_interpolateProbes;
	if (key == 0x44 /*D*/)
		m_useDirectionalLight = !m_useDirectionalLight;
	if (key == 0x4C /*L*/)
		m_useAdvancedLighting = !m_useAdvancedLighting;
	if (key == 0x50 /*P*/)
		m_useProbeLighting = !m_useProbeLighting;
	if (key == 0x53 /*S*/)
		m_useShadows = !m_useShadows;
	if (key == 0x55 /*U*/)
		m_updateProbes = !m_updateProbes;
	if (key == 0x54 /*T*/)
		m_movingLight = !m_movingLight;
	if (key == 0x4F /*O*/)
		m_useOctagonalProbes = !m_useOctagonalProbes;
	if (key == 0x43 /*C*/)
		m_renderProbeMarkers = !m_renderProbeMarkers;
	if (key == 0x4D /*M*/)
		m_renderLightMarkers = !m_renderLightMarkers;
}

void Thesis::OnKeyDown(UINT8 key) {
	if (key == VK_LEFT) {
		if (m_movingLight)
			m_lightPos[0] += m_lightSpeed;
		else
			m_doorPosition[0] += m_lightSpeed;
	}
	if (key == VK_RIGHT) {
		if (m_movingLight)
			m_lightPos[0] -= m_lightSpeed;
		else
			m_doorPosition[0] -= m_lightSpeed;
	}
	if (key == VK_UP) {
		if (m_movingLight)
			m_lightPos[2] += m_lightSpeed;
		else
			m_doorPosition[2] += m_lightSpeed;
	}
	if (key == VK_DOWN) {
		if (m_movingLight)
			m_lightPos[2] -= m_lightSpeed;
		else
			m_doorPosition[2] -= m_lightSpeed;
	}
	if (key == VK_HOME) {
		if (m_movingLight)
			m_lightPos[1] += m_lightSpeed;
		else
			m_doorPosition[1] += m_lightSpeed;
	}
	if (key == VK_END) {
		if (m_movingLight)
			m_lightPos[1] -= m_lightSpeed;
		else
			m_doorPosition[1] -= m_lightSpeed;
	}

	if (key == VK_PRIOR)
		m_probeBrightness += .5f;
	if (key == VK_NEXT)
		m_probeBrightness -= .5f;

	if (key == 0x31 /*1*/)
		m_bounceAttenuation += 0.005f;
	if (key == 0x32 /*2*/)
		m_bounceAttenuation -= 0.005f;

	if (key == 0x51 /*Q*/)
		m_falloffRegulator += 10.f;
	if (key == 0x57 /*W*/)
		m_falloffRegulator -= 10.f;

	if (key == 0xBB /*+*/)
		m_brightness += 0.1f;
	if (key == 0xBD /*-*/)
		m_brightness -= 0.1f;

	if (key == 0x39 /*9*/)
		m_multiBounceIntensity += 0.2f;
	if (key == 0x30 /*0*/)
		m_multiBounceIntensity -= 0.2f;

	if (key == 0x4E /*N*/)
		m_useMultiBounce = !m_useMultiBounce;

	if (key == 0x33 /*3*/)
		m_directionalLightAngle[0] += m_lightSpeed;
	if (key == 0x34 /*4*/)
		m_directionalLightAngle[0] -= m_lightSpeed;
	if (key == 0x35 /*5*/)
		m_directionalLightAngle[1] += m_lightSpeed;
	if (key == 0x36 /*6*/)
		m_directionalLightAngle[1] -= m_lightSpeed;
	if (key == 0x37 /*7*/)
		m_ambientTerm -= 0.01f;
	if (key == 0x38 /*8*/)
		m_ambientTerm += 0.01f;
}