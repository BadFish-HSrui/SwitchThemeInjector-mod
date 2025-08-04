#include "TextPage.hpp"
#include "../Version.hpp"
#include "../ViewFunctions.hpp"
#include "../UI/imgui/imgui_internal.h"

using namespace std;

TextPage::TextPage(const std::string& title, const std::string& text) :
	Text(text)
{
	Name = title;
	c_str = Text.c_str();
}

TextPage::TextPage(const char* title, const char* text) 
{
	Name = title;
	c_str = text;
}

void TextPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font25);
	ImGui::TextWrapped(c_str);
	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void TextPage::Update()
{
	Parent->PageLeaveFocus(this);
}

CreditsPage::CreditsPage() :
	creditsText("NXThemes安装器由exelix开发 - " + Version::Name + " - 核心版本" + SwitchThemesCommon::CoreVer +
		"\n源代码: github.com/exelix11/SwitchThemeInjector"+
		"\n捐赠: ko-fi.com/exelix11"+
		"\n葡萄糖酸菜鱼 汉化\n\n")
{
	Name = "关于";
}

extern void ShowFirstTimeHelp(bool WelcomeScr); //from main.cpp
void CreditsPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::SetCursorPosY(20);
	ImGui::PushFont(font30);
	ImGui::TextWrapped(creditsText.c_str());
	ImGui::PopFont();

	ImGui::PushFont(font25);
	ImGui::TextWrapped(
		"感谢:\n"
		"Syroot 提供 BinaryData 库\n"
		"AboodXD 提供 Bntx 编辑器和 sarc 库\n"
		"shchmue 提供 Lockpick\n"
		"SciresM 提供 hactool\n"
		"Atmosphere 和 libnx 的所有贡献者\n"
		"github 上的 switch-stuff 提供字体转换器\n"
		"Fincs 提供 hybrid_app 模板\n"
		"DearImgui github 仓库的所有贡献者"
	);

	if (ImGui::Button("显示首次启动信息"))
		PushFunction([]() {ShowFirstTimeHelp(false); });
	PAGE_RESET_FOCUS;

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void CreditsPage::Update()
{
	if (Utils::PageLeaveFocusInput())
		Parent->PageLeaveFocus(this);
}