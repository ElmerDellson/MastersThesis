#include "stdafx.h"
#include "Thesis.h"

void Thesis::SetSettings() {
	if (m_GIPreset == Off)
		m_useProbeLighting = false;

	std::vector<float> cameraPos{};
	std::vector<float> cameraLookAt{};
	if (m_scene == Scene0) {
		// Cubes
		m_cubeScale = 110.f;

		// Probes.
		m_nbProbeLayers = 21;

		// Shadows.
		m_useShadows = true;

		// Ambient term.
		m_ambientTerm = 0.07f;

		// Camera.
		cameraPos = { 0.f, 200.f, -700.f };
		cameraLookAt = { 0, 200.f, 0.f };

		// Light.
		float lightPos[3]{ 0.0f, 313.f, 0.f };
		std::copy(lightPos, lightPos + 3, m_lightPos);

		// Falloff.
		m_falloffRegulator = 680.f;

		// Brightness of whole image.
		m_brightness = 1.f;

		// Bounce attenuation.
		m_bounceAttenuation = 0.105f;

		// Light spread.
		m_lightSpread = 157000;

		// Multibounce intensity.
		m_multiBounceIntensity = 5.4f;

		// Brightness of probe lighting.
		m_probeBrightness = 26.2f; 
		//m_probeBrightness = 101.f; // For GI only images.
	}
	else if (m_scene == Scene1) {
		// Cubes
		m_cubeScale = 25.f;

		// Probes.
		m_nbProbeLayers = 12;

		// Shadows.
		m_useShadows = true;

		// Ambient term.
		m_ambientTerm = 0.01f;

		// Camera.
		cameraPos = { -100.f, 100.f, -300.f };
		cameraLookAt = { 0, 100.f, 0.f };

		// Light.
		float lightPos[3]{ 170.0f, 167.f, 15.f };
		std::copy(lightPos, lightPos + 3, m_lightPos);

		// Falloff.
		m_falloffRegulator = 690.f;

		// Brightness
		m_brightness = 1.8f;

		// Bounce attenuation.
		m_bounceAttenuation = 0.05f;

		// Light intensity.
		m_lightSpread = 100000;

		// Multibounce intensity.
		m_multiBounceIntensity = 8.4f;

		m_probeBrightness = 50.f;
	}

	// Initialize camera.
	nv_helpers_dx12::CameraManip.setWindowSize(GetWidth(), GetHeight());
	nv_helpers_dx12::CameraManip.setLookat(glm::vec3(cameraPos[0], cameraPos[1], cameraPos[2]),
		glm::vec3(cameraLookAt[0], cameraLookAt[1], cameraLookAt[2]), glm::vec3(0, 1, 0));
}