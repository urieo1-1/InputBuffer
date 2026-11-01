#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

// Input buffer: if you release jump while still in the air less than N physics
// steps after pressing it, the release is delayed until the press is N steps old.
// That means a press made slightly before landing still registers as a held
// input when you touch the ground, so the jump comes out.

class $modify(BufferedGameLayer, GJBaseGameLayer) {
	struct Fields {
		int step = 0;
		int pressStep[2] = {-1000, -1000};
		bool pendingRelease[2] = {false, false};
	};

	void handleButton(bool down, int button, bool isPlayer1) {
		// only touch the jump button
		if (button != 1 || !Mod::get()->getSettingValue<bool>("enabled")) {
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}

		auto f = m_fields.self();
		int idx = isPlayer1 ? 0 : 1;
		int buffer = static_cast<int>(Mod::get()->getSettingValue<int64_t>("buffer-frames"));

		if (down) {
			f->pendingRelease[idx] = false; // new press cancels a delayed release
			f->pressStep[idx] = f->step;
			GJBaseGameLayer::handleButton(true, button, isPlayer1);
			return;
		}

		PlayerObject* player = isPlayer1 ? m_player1 : m_player2;
		int age = f->step - f->pressStep[idx];

		// released early while airborne -> hold it a little longer
		if (buffer > 0 && player && !player->m_isOnGround && age < buffer) {
			f->pendingRelease[idx] = true;
			return;
		}

		f->pendingRelease[idx] = false;
		GJBaseGameLayer::handleButton(false, button, isPlayer1);
	}

	void processCommands(float dt) {
		GJBaseGameLayer::processCommands(dt);

		auto f = m_fields.self();
		f->step++;

		int buffer = static_cast<int>(Mod::get()->getSettingValue<int64_t>("buffer-frames"));
		for (int idx = 0; idx < 2; idx++) {
			if (f->pendingRelease[idx] && f->step - f->pressStep[idx] >= buffer) {
				f->pendingRelease[idx] = false;
				GJBaseGameLayer::handleButton(false, 1, idx == 0);
			}
		}
	}
};
