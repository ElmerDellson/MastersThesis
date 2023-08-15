#include "stdafx.h"
#include "Thesis.h"

void Thesis::HandleAnimation(bool runBenchmark)
{
	if (runBenchmark) {
		if (m_scene == Scene0) {
			if (m_frameCount > 200 && m_frameCount < 350)
				m_time++;

			m_instances[0].second = XMMatrixScaling(.5f, 1.f, .5f) * XMMatrixRotationY(.586f + (457 + m_time) / 93.f)
				* XMMatrixTranslation(60.f, m_distBetweenProbes - 10.f, 70.f);

			m_instances[1].second = XMMatrixScaling(.5f, .5f, .5f) * XMMatrixRotationY(-.586f + (463 + m_time) / 93.f)
				* XMMatrixTranslation(-67.5f, m_distBetweenProbes - 10.f, -70.f);

			if (m_frameCount > 600)
				PostQuitMessage(0);
		}
		else if (m_scene == Scene1) {
			if (m_animate)
				m_time++;

			if (m_time > 50 && m_time < 200)
				m_doorPosition[1] += 1.f;

			m_instances[0].second = XMMatrixScaling(.6f, 3.f, 2.0f) * XMMatrixTranslation(m_doorPosition[0], m_doorPosition[1], m_doorPosition[2]);

			if (m_frameCount > 600)
				PostQuitMessage(0);
		}
	}
	else {
		if (m_scene == Scene0) {
			if (m_animate)
				m_time++;

			m_instances[0].second = XMMatrixScaling(.5f, 1.f, .5f) * XMMatrixRotationY(.586f + (457 + m_time) / 93.f)
				* XMMatrixTranslation(60.f, m_distBetweenProbes - 10.f, 70.f);

			m_instances[1].second = XMMatrixScaling(.5f, .5f, .5f) * XMMatrixRotationY(-.586f + (463 + m_time) / 93.f)
				* XMMatrixTranslation(-67.5f, m_distBetweenProbes - 10.f, -70.f);
		}
		else if (m_scene == Scene1) {
			if (m_animate) {
				m_doorPosition[1] += 1.f;

				if (m_doorPosition[1] > 165)
					m_doorPosition[1] = 165;
			}

			m_instances[0].second = XMMatrixScaling(.6f, 3.f, 2.0f) * XMMatrixTranslation(m_doorPosition[0], m_doorPosition[1], m_doorPosition[2]);
		}
			
	}

	m_instances[m_instances.size() - 1].second = XMMatrixScaling(10.f, 10.f, 10.f)
		* XMMatrixTranslation(m_lightPos[0], m_lightPos[1], m_lightPos[2]);


}