#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "CheatManager.hpp"
#include "GUI.hpp"

#include <ranges>

#include "Memory.hpp"
#include "SkinDatabase.hpp"
#include "Utils.hpp"
#include "fnv_hash.hpp"
#include "imgui/imgui.h"
//#define GLOBALSKINS
//#define LOGGER

static float v_time = 0.0f;

// 美化 ImGui 的样式
void applyImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 设置全局样式
    style.WindowRounding = 7.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // 设置颜色
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);   // 窗口背景色
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);   // 头部背景色
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // 头部悬停色
    colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // 头部激活色
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.60f, 0.90f, 1.00f);   // 按钮背景色
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.70f, 1.00f, 1.00f); // 按钮悬停色
    colors[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.50f, 0.80f, 1.00f); // 按钮激活色
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);   // 控件背景色
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.90f); // 控件悬停色
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // 控件激活色
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);     // 标签背景色
    colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // 标签悬停色
    colors[ImGuiCol_TabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f); // 标签激活色
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);  // 标题背景色
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // 标题激活色
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // 标题折叠色

    // 设置边框颜色
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);   // 边框颜色
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f); // 黑色阴影，透明度为 50%

    style.WindowBorderSize = 1.0f;  // 窗口边框大小
    style.FrameBorderSize = 1.0f;   // 控件边框大小
    style.ItemSpacing = ImVec2(8, 6); // 控件间距
    style.ScrollbarSize = 12.0f;    // 滚动条宽度
    style.GrabMinSize = 8.0f;       // 滑块最小尺寸
    style.WindowPadding = ImVec2(8, 8); // 窗口内边距
}

inline static void footer() noexcept
{
    using namespace std::string_literals;
    ImGui::Separator();
    ImGui::textUnformattedCentered("Copyright (C) 2021-2024 R3nzTheCodeGOD");
}

static void changeTurretSkin(const std::int32_t skinId, const std::int32_t team) noexcept
{
    if (skinId == -1)
        return;

    const auto turrets{ cheatManager.memory->turretList };
    const auto playerTeam{ cheatManager.memory->localPlayer->get_team() };

    for (auto i{ 0u }; i < turrets->length; ++i) {
        if (const auto turret{ turrets->list[i] }; turret->get_team() == team) {
            if (playerTeam == team) {
                turret->get_character_data_stack()->base_skin.skin = skinId * 2;
                turret->get_character_data_stack()->update(true);
            } else {
                turret->get_character_data_stack()->base_skin.skin = skinId * 2 + 1;
                turret->get_character_data_stack()->update(true);
            }
        }
    }
}

void GUI::render() noexcept
{
    applyImGuiStyle(); // 应用自定义样式

    std::call_once(set_font_scale, [&]
        {
            ImGui::GetIO().FontGlobalScale = cheatManager.config->fontScale;
        });

    v_time += static_cast<float>(ImGui::GetIO().DeltaTime);

    const auto player{ cheatManager.memory->localPlayer };
    const auto heroes{ cheatManager.memory->heroList };
    static const auto my_team{ player ? player->get_team() : 100 };
    static int gear{ player ? player->get_character_data_stack()->base_skin.gear : 0 };

    static const auto vector_getter_skin = [](void* vec, const std::int32_t idx, const char** out_text) noexcept {
        const auto& vector{ *static_cast<std::vector<SkinDatabase::skin_info>*>(vec) };
        if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
        *out_text = idx == 0 ? "Default" : vector.at(idx - 1).skin_name.c_str();
        return true;
        };

    static const auto vector_getter_ward_skin = [](void* vec, const std::int32_t idx, const char** out_text) noexcept {
        const auto& vector{ *static_cast<std::vector<std::pair<std::int32_t, const char*>>*>(vec) };
        if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
        *out_text = idx == 0 ? "Default" : vector.at(idx - 1).second;
        return true;
        };

    static auto vector_getter_gear = [](void* vec, const std::int32_t idx, const char** out_text) noexcept {
        const auto& vector{ *static_cast<std::vector<const char*>*>(vec) };
        if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
        *out_text = vector[idx];
        return true;
        };

    static auto vector_getter_default = [](void* vec, const std::int32_t idx, const char** out_text) noexcept {
        const auto& vector{ *static_cast<std::vector<const char*>*>(vec) };
        if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
        *out_text = idx == 0 ? "Default" : vector.at(idx - 1);
        return true;
        };

    ImGui::SetNextWindowBgAlpha(0.85f); // 设置窗口背景透明度

    ImGui::Begin("Kuka", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
            if (player) {
                if (ImGui::BeginTabItem(reinterpret_cast<const char*>(std::u8string(u8"玩家").c_str()))) {
                    auto& values{ cheatManager.database->champions_skins[fnv::hash_runtime(player->get_character_data_stack()->base_skin.model.str)] };
                    ImGui::Text(reinterpret_cast<const char*>(std::u8string(u8"玩家设置:").c_str()));

                    if (ImGui::Combo(reinterpret_cast<const char*>(std::u8string(u8"当前皮肤").c_str()), &cheatManager.config->current_combo_skin_index, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
                        if (cheatManager.config->current_combo_skin_index > 0) {
                            player->change_skin(values[cheatManager.config->current_combo_skin_index - 1].model_name, values[cheatManager.config->current_combo_skin_index - 1].skin_id);
                            cheatManager.config->save();
                        }

                    const auto playerHash{ fnv::hash_runtime(player->get_character_data_stack()->base_skin.model.str) };
                    if (const auto it{ std::ranges::find_if(cheatManager.database->specialSkins,
                        [&skin = player->get_character_data_stack()->base_skin.skin, &ph = playerHash](const SkinDatabase::specialSkin& x) noexcept -> bool
                        {
                            return x.champHash == ph && (x.skinIdStart <= skin && x.skinIdEnd >= skin);
                        })
                        }; it != cheatManager.database->specialSkins.end())
                    {
                        const auto stack{ player->get_character_data_stack() };
                        gear = stack->base_skin.gear;

                        if (ImGui::Combo("Current Gear", &gear, vector_getter_gear, static_cast<void*>(&it->gears), it->gears.size())) {
                            player->get_character_data_stack()->base_skin.gear = static_cast<std::int8_t>(gear);
                            player->get_character_data_stack()->update(true);
                        }
                        ImGui::Separator();
                    }

                    if (ImGui::Combo(reinterpret_cast<const char*>(std::u8string(u8"当前守卫皮肤").c_str()), &cheatManager.config->current_combo_ward_index, vector_getter_ward_skin, static_cast<void*>(&cheatManager.database->wards_skins), cheatManager.database->wards_skins.size() + 1))
                        cheatManager.config->current_ward_skin_index = cheatManager.config->current_combo_ward_index == 0 ? -1 : cheatManager.database->wards_skins.at(cheatManager.config->current_combo_ward_index - 1).first;
                    footer();
                    ImGui::EndTabItem();
                }
            }

            static std::int32_t temp_heroes_length = heroes->length;
            if (temp_heroes_length > 1)
            {
                if (ImGui::BeginTabItem(reinterpret_cast<const char*>(std::u8string(u8"其他英雄").c_str()))) {
                    ImGui::Text(reinterpret_cast<const char*>(std::u8string(u8"其他英雄设置").c_str()));
                    std::int32_t last_team{ 0 };
                    for (auto i{ 0u }; i < heroes->length; ++i) {
                        const auto hero{ heroes->list[i] };

                        if (hero == player)
                        {
                            continue;
                        }

                        const auto champion_name_hash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };
                        if (champion_name_hash == FNV("PracticeTool_TargetDummy"))
                        {
                            temp_heroes_length = heroes->length - 1;
                            continue;
                        }

                        const auto hero_team{ hero->get_team() };
                        const auto is_enemy{ hero_team != my_team };

                        if (last_team == 0 || hero_team != last_team) {
                            if (last_team != 0)
                                ImGui::Separator();
                            if (is_enemy)
                                ImGui::Text(reinterpret_cast<const char*>(std::u8string(u8"敌方英雄").c_str()));
                            else
                                ImGui::Text(reinterpret_cast<const char*>(std::u8string(u8"我方英雄").c_str()));
                            last_team = hero_team;
                        }

                        auto& config_array{ is_enemy ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };
                        const auto [fst, snd] { config_array.insert({ champion_name_hash, 0 }) };

                        std::snprintf(this->str_buffer, sizeof(this->str_buffer), cheatManager.config->heroName ? "HeroName: [ %s ]##%X" : "PlayerName: [ %s ]##%X", cheatManager.config->heroName ? hero->get_character_data_stack()->base_skin.model.str : hero->get_name()->c_str(), reinterpret_cast<std::uintptr_t>(hero));

                        auto& values{ cheatManager.database->champions_skins[champion_name_hash] };
                        if (ImGui::Combo(str_buffer, &fst->second, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
                            if (fst->second > 0) {
                                hero->change_skin(values[fst->second - 1].model_name, values[fst->second - 1].skin_id);
                                cheatManager.config->save();
                            }
                    }
                    footer();
                    ImGui::EndTabItem();
                }
            }

#ifdef GLOBALSKINS
            if (ImGui::BeginTabItem("Global Skins")) {
                ImGui::Text("Global Skins Settings:");
                if (ImGui::Combo("Minion Skins:", &cheatManager.config->current_combo_minion_index, vector_getter_default, static_cast<void*>(&cheatManager.database->minions_skins), cheatManager.database->minions_skins.size() + 1))
                    cheatManager.config->current_minion_skin_index = cheatManager.config->current_combo_minion_index - 1;
                ImGui::Separator();
                if (ImGui::Combo("Order Turret Skins:", &cheatManager.config->current_combo_order_turret_index, vector_getter_default, static_cast<void*>(&cheatManager.database->turret_skins), cheatManager.database->turret_skins.size() + 1))
                    changeTurretSkin(cheatManager.config->current_combo_order_turret_index - 1, 100);
                if (ImGui::Combo("Chaos Turret Skins:", &cheatManager.config->current_combo_chaos_turret_index, vector_getter_default, static_cast<void*>(&cheatManager.database->turret_skins), cheatManager.database->turret_skins.size() + 1))
                    changeTurretSkin(cheatManager.config->current_combo_chaos_turret_index - 1, 200);
                ImGui::Separator();
                ImGui::Text("Jungle Mobs Skins Settings:");
                for (auto& [name, name_hashes, skins] : cheatManager.database->jungle_mobs_skins) {
                    std::snprintf(str_buffer, 256, "Current %s skin", name);
                    const auto [fst, snd] { cheatManager.config->current_combo_jungle_mob_skin_index.insert({ name_hashes.front(), 0 }) };
                    if (ImGui::Combo(str_buffer, &fst->second, vector_getter_default, &skins, skins.size() + 1))
                        for (const auto& hash : name_hashes)
                            cheatManager.config->current_combo_jungle_mob_skin_index[hash] = fst->second;
                }
                footer();
                ImGui::EndTabItem();
            }
#endif // GLOBALSKIN

            
#ifdef LOGGER
            if (ImGui::BeginTabItem("Logger")) {
                cheatManager.logger->draw();
                ImGui::EndTabItem();
            }
#endif // LOGGER

            if (ImGui::BeginTabItem(reinterpret_cast<const char*>(std::u8string(u8"设置").c_str()))) {
                ImGui::hotkey(reinterpret_cast<const char*>(std::u8string(u8"菜单栏").c_str()), cheatManager.config->menuKey);
             /*   ImGui::Checkbox(cheatManager.config->heroName ? "HeroName based" : "PlayerName based", &cheatManager.config->heroName);*/
                ImGui::Checkbox(reinterpret_cast<const char*>(std::u8string(u8"切换皮肤快捷键").c_str()), &cheatManager.config->quickSkinChange);
                ImGui::hoverInfo("It allows you to change skin without opening the menu with the key you assign from the keyboard.");

                if (cheatManager.config->quickSkinChange) {
                    ImGui::Separator();
                    ImGui::hotkey(reinterpret_cast<const char*>(std::u8string(u8"上一个皮肤快捷键").c_str()), cheatManager.config->previousSkinKey);
                    ImGui::hotkey(reinterpret_cast<const char*>(std::u8string(u8"下一个皮肤快捷键").c_str()), cheatManager.config->nextSkinKey);
                    ImGui::Separator();
                }

                if (player)
                    ImGui::InputText(reinterpret_cast<const char*>(std::u8string(u8"修改名字").c_str()), player->get_name());

                if (ImGui::Button(reinterpret_cast<const char*>(std::u8string(u8"随机皮肤").c_str()))) {
                    for (auto i{ 0u }; i < heroes->length; ++i) {
                        const auto hero{ heroes->list[i] };
                        const auto championHash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };

                        if (championHash == FNV("PracticeTool_TargetDummy"))
                            continue;

                        const auto skinCount{ cheatManager.database->champions_skins[championHash].size() };
                        auto& skinDatabase{ cheatManager.database->champions_skins[championHash] };
                        auto& config{ (hero->get_team() != my_team) ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };

                        if (hero == player) {
                            cheatManager.config->current_combo_skin_index = random(1ull, skinCount);
                            hero->change_skin(skinDatabase[cheatManager.config->current_combo_skin_index - 1].model_name, skinDatabase[cheatManager.config->current_combo_skin_index - 1].skin_id);
                        } else {
                            auto& data{ config[championHash] };
                            data = random(1ull, skinCount);
                            hero->change_skin(skinDatabase[data - 1].model_name, skinDatabase[data - 1].skin_id);
                        }
                        cheatManager.config->save();
                    }
                } ImGui::hoverInfo("Randomly changes the skin of all champions.");

                ImGui::SliderFloat("Font Scale", &cheatManager.config->fontScale, 1.0f, 2.0f, "%.3f");
                if (ImGui::GetIO().FontGlobalScale != cheatManager.config->fontScale) {
                    ImGui::GetIO().FontGlobalScale = cheatManager.config->fontScale;
                } ImGui::hoverInfo("Changes the menu font scale.");

                if (ImGui::Button(reinterpret_cast<const char*>(std::u8string(u8"").c_str())))
                    cheatManager.hooks->uninstall();
                ImGui::hoverInfo("You will be returned to the reconnect screen.");
                ImGui::Text("FPS: %.0f FPS", ImGui::GetIO().Framerate);
                footer();
                ImGui::EndTabItem();
            }
        }
    }

    ImGui::End();
}
