//////////////////////////////////////////////////////////////////////////////////
//	UIExplorerCreation_Legacy - v83/v87 Explorer character creation
//	Simplified implementation that works with available v83/v87 assets
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "../UIElement.h"
#include "../../Util/LegacyUI.h"
#include "../../Character/Look/CharLook.h"
#include "../../Character/MapleStat.h"
#include "../../Template/Rectangle.h"
#include "../KeyAction.h"

namespace ms
{
	class UIExplorerCreation_Legacy : public UIElement
	{
	public:
		static constexpr Type TYPE = UIElement::Type::CLASSCREATION;
		static constexpr bool FOCUSED = false;
		static constexpr bool TOGGLED = false;

		UIExplorerCreation_Legacy();

		void draw(float inter) const override;
		void update() override;

		Cursor::State send_cursor(bool clicked, Point<int16_t> cursorpos) override;
		void send_key(int32_t keycode, bool pressed, bool escape) override;

		UIElement::Type get_type() const override;

		void send_naming_result(bool nameused);

	protected:
		Button::State button_pressed(uint16_t buttonid) override;

	private:
		enum Buttons : uint16_t
		{
			BtOK,
			BtCancel,
			BtStatDice,
			BtHairL,
			BtHairR,
			BtFaceL,
			BtFaceR,
			BtSkinL,
			BtSkinR,
			BtGenderM,
			BtGenderF
		};

		// Character creation logic
		void randomize_stats();
		void randomize_look();
		void update_character_display();
		void create_character();
		void return_to_raceselect();
		
		// UI state
		Text version;
		Point<int16_t> version_pos;
		
		// Character name input
		std::string character_name;
		Text name_display;
		bool name_input_active;
		
		// Character stats (for Explorer class)
		std::map<MapleStat::Id, uint16_t> base_stats;
		Text str_display;
		Text dex_display;
		Text int_display;
		Text luk_display;
		
		// Character appearance
		bool female;
		int32_t face;
		int32_t hair;
		int32_t skin;
		
		// UI textures (loaded from v83/v87 assets)
		Texture name_input_bg;
		Texture stat_dice_button;
		Texture ok_button;
		Texture cancel_button;
		Texture job_banner;
		
		// Character preview (simplified)
		CharLook char_look;
		bool char_look_valid;
		
		// Available options for randomization
		std::vector<int32_t> available_faces_m;
		std::vector<int32_t> available_faces_f;
		std::vector<int32_t> available_hairs_m;
		std::vector<int32_t> available_hairs_f;
		std::vector<int32_t> available_skins;
	};
}