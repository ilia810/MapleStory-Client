//////////////////////////////////////////////////////////////////////////////////
//	This file is part of the continued Journey MMORPG client					//
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton						//
//																				//
//	This program is free software: you can redistribute it and/or modify		//
//	it under the terms of the GNU Affero General Public License as published by	//
//	the Free Software Foundation, either version 3 of the License, or			//
//	(at your option) any later version.											//
//																				//
//	This program is distributed in the hope that it will be useful,				//
//	but WITHOUT ANY WARRANTY; without even the implied warranty of				//
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the				//
//	GNU Affero General Public License for more details.						//
//																				//
//	You should have received a copy of the GNU Affero General Public License	//
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.		//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace ms
{
    // Logical asset identifiers to replace hardcoded string paths
    // Each enum value maps to one or more physical asset paths with fallback support
    enum class AssetID
    {
        // === UI Assets ===
        
        // Login Screen
        UI_Login_Background,                // Login screen main background
        UI_Login_Title_Background,          // Login title area background
        UI_Login_Version_Position,          // Version text position
        UI_Login_Capslock_Warning,          // Caps lock indicator
        UI_Login_Checkbox_Unchecked,        // Save login unchecked state
        UI_Login_Checkbox_Checked,          // Save login checked state
        UI_Login_Button_Login,              // Login button
        UI_Login_Button_New,                // New account button
        UI_Login_Button_Quit,               // Quit button
        UI_Login_Button_Homepage,           // Homepage button
        UI_Login_Button_PasswdLost,         // Lost password button
        UI_Login_Button_EmailLost,          // Lost email button
        UI_Login_Button_EmailSave,          // Save email button
        UI_Login_Tab_Disabled,              // Disabled tab states
        UI_Login_Tab_Enabled,               // Enabled tab states
        UI_Login_Field_MapleID,             // MapleID input field background
        UI_Login_Field_NexonID,             // NexonID input field background
        UI_Login_Field_Password,            // Password input field background
        
        // World Selection
        UI_WorldSelect_Background,          // World selection background
        UI_WorldSelect_Button_Exit,         // Exit button
        UI_WorldSelect_World_Button,        // World selection buttons
        UI_WorldSelect_Channel_Button,      // Channel selection buttons
        
        // Character Selection
        UI_CharSelect_Background,           // Character selection background
        
        // Shop Interface
        UI_Shop_Background,                 // Shop main background
        UI_Shop_Button_Buy,                 // Buy button
        UI_Shop_Button_Sell,                // Sell button
        
        // Chat Interface
        UI_Chat_Background,                 // Chat window background
        UI_Chat_Input_Background,           // Chat input field
        UI_Chat_Button_ItemLink,            // Item link button (post-v87)
        UI_Chat_Button_Emoticon,            // Emoticon button (post-v87)
        
        // Minimap
        UI_MiniMap_Background,              // Minimap background
        UI_MiniMap_Icons,                   // Minimap icons
        
        // Inventory
        UI_Inventory_Background,            // Inventory window background
        UI_Inventory_Tabs,                  // Inventory tab buttons
        
        // Status Bar
        UI_StatusBar_Background,            // Status bar background
        UI_StatusBar_HP_Bar,                // HP bar graphics
        UI_StatusBar_MP_Bar,                // MP bar graphics
        
        // World Map
        UI_WorldMap_Background,             // World map background
        UI_WorldMap_Icons,                  // World map location icons
        
        // Notice/Dialog
        UI_Notice_Background,               // Notice dialog background
        UI_Notice_Button_OK,                // OK button
        UI_Notice_Button_Cancel,            // Cancel button
        
        // Logo
        UI_Logo_Wizet,                      // Wizet startup logo
        UI_Logo_Nexon,                      // Nexon logo
        
        // === Map Assets ===
        
        // Backgrounds
        Map_Login_Background,               // Login screen map background
        Map_CharSelect_Background,          // Character select background
        
        // Portals
        Map_Portal_Default_Animation,       // Default portal animation
        Map_Portal_Game_Animation,          // In-game portal animation
        
        // Map Helper
        Map_Helper_WorldMap_Image,          // World map images
        Map_Helper_Portal_Graphics,         // Portal graphics
        
        // Effects
        Map_Effect_Animation,               // Map-based effects
        
        // Objects
        Map_Object_CharCreation,            // Character creation objects
        
        // === Character Assets ===
        
        // Body
        Character_Body_Default,             // Default character body
        Character_Body_Skin_Names,          // Skin tone names
        
        // Equipment
        Character_Weapon_Graphics,          // Weapon graphics
        Character_Equipment_Graphics,       // Equipment graphics
        
        // Effects
        Character_Afterimage_Sword,         // Sword afterimage effects
        Character_Skill_Effects,            // Skill visual effects
        
        // === Sound Assets ===
        
        // UI Sounds
        Sound_UI_Button_Click,              // Button click sound
        Sound_UI_Button_Hover,              // Button hover sound
        Sound_UI_Window_Open,               // Window open sound
        Sound_UI_Window_Close,              // Window close sound
        
        // Game Sounds
        Sound_Game_Portal,                  // Portal usage sound
        Sound_Game_LevelUp,                 // Level up sound
        Sound_Game_ItemDrop,                // Item drop sound
        
        // Weapon Sounds
        Sound_Weapon_Sword_Attack,          // Sword attack sounds
        Sound_Weapon_Bow_Attack,            // Bow attack sounds
        Sound_Weapon_Magic_Cast,            // Magic casting sounds
        
        // === String Assets ===
        
        // Equipment Names
        String_Equipment_Weapon_Names,      // Weapon names
        String_Equipment_Armor_Names,       // Armor names
        String_Equipment_Accessory_Names,   // Accessory names
        
        // Monster Names
        String_Monster_Names,               // Monster display names
        String_Monster_Descriptions,        // Monster descriptions
        
        // Skill Names
        String_Skill_Names,                 // Skill names
        String_Skill_Descriptions,          // Skill descriptions
        
        // UI Text
        String_UI_ButtonText,               // UI button text
        String_UI_MenuText,                 // Menu text
        
        // === Item Assets ===
        
        // Item Icons
        Item_Icons_Weapon,                  // Weapon icons
        Item_Icons_Armor,                   // Armor icons
        Item_Icons_Accessory,               // Accessory icons
        Item_Icons_Consumable,              // Consumable item icons
        Item_Icons_Quest,                   // Quest item icons
        
        // === Effect Assets ===
        
        // Visual Effects
        Effect_Skill_Animations,            // Skill effect animations
        Effect_Damage_Numbers,              // Damage number graphics
        Effect_Status_Icons,                // Status effect icons
        
        // === Misc Assets ===
        
        // Character Creation
        Etc_CharCreation_Hair_Options,      // Available hair styles
        Etc_CharCreation_Face_Options,      // Available face options
        Etc_CharCreation_Class_Info,        // Character class information
        
        // Job Data
        Etc_Job_Icons,                      // Job class icons
        Etc_Job_Descriptions,               // Job descriptions
        
        // Count for iteration purposes
        ASSET_COUNT
    };
}