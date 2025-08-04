#include "CfwSelectPage.hpp"
#include "../ViewFunctions.hpp"
#include "../UI/UIManagement.hpp"

using namespace std;

CfwSelectPage::CfwSelectPage(const vector<string>& folders) : Folders(folders) {}

CfwSelectPage::~CfwSelectPage()
{

}

static const int BtnWidth = 500;
static const int XCursorBtn = SCR_W / 2 - BtnWidth / 2;

void CfwSelectPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPageFullscreen("CfwSelectPage", 10, 10);
	ImGui::SetWindowFocus();

	if (Folders.size() == 0)
	{
		ImGui::PushFont(font30);
		Utils::ImGuiCenterString("无法找到 Atmosphere 文件夹。");
		ImGui::PopFont();
		ImGui::NewLine();
		ImGui::TextWrapped(
			"请确保您SD卡的根目录中有 \"atmosphere\" 文件夹。\n\n"
			"如果文件夹存在，则可能是您的SD卡出现了问题。\n");
	}	
	else {
		Utils::ImGuiCenterString("检测到多个CFW文件夹，您想使用哪一个？");

		ImGui::PushFont(font30);
		ImGui::SetCursorPos({ (float)XCursorBtn, ImGui::GetCursorPosY() + 30 });

		int count = 0;
		for (const auto& e : Folders)
		{
			ImGui::SetCursorPosX((float)XCursorBtn);
			if (ImGui::Button(e.c_str(), { BtnWidth, 50 }))
			{
				fs::cfw::SetFolder(e);
				PopPage(this);
			}
			count++;
		}

		ImGui::PopFont();
	}

	ImGui::NewLine();
	if (Utils::ImGuiCenterButton("关闭此应用"))
		App::Quit();

	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void CfwSelectPage::Update()
{
	if (gamepad.buttons[GLFW_GAMEPAD_BUTTON_START])
		App::Quit();
}