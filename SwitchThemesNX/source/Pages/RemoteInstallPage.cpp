#include "RemoteInstallPage.hpp"
#include "../ViewFunctions.hpp"
#include <numeric>
#include "RemoteInstall/API.hpp"
#include "../SwitchThemesCommon/Layouts/json.hpp"

using namespace std;

static bool ComboBoxApiProviderGetter(void*, int index, const char** str)
{
	if (index < 0 || (size_t)index >= RemoteInstall::API::ProviderCount())
		return false;
	
	*str = RemoteInstall::API::GetProvider(index).Name.c_str();
	return true;
}

RemoteInstallPage::~RemoteInstallPage()
{
	if (!UseLowMemory)
		RemoteInstall::Finalize();
}

RemoteInstallPage::RemoteInstallPage() : 
BtnStart("开始远程安装###InstallBtn")
{
	Name = "下载主题";
	if (!UseLowMemory)
	{
		SetRemoteInstallCode("");
		RemoteInstall::Initialize();
	}
}

void RemoteInstallPage::Render(int X, int Y)
{
	AllowLeft = true;

	Utils::ImGuiSetupPage(this, X, Y, DefaultWinFlags & ~ImGuiWindowFlags_NoScrollbar);
	
	if (!RemoteInstallFile)
	{
		ImGui::PushFont(font40);
		ImGui::Text("从互联网下载");
		ImGui::PopFont();

		if (UseLowMemory)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::TextWrapped("此功能在小程序模式下不可用，请使用前端模式启动。");
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::TextWrapped("从列表中选择一个主题源来下载主题。\n您可以按照Github上的wiki说明添加自定义源。");
			ImGui::PushItemWidth(500);
			if (ImGui::Combo("###ProviderSelection", &ProviderIndex, ComboBoxApiProviderGetter, nullptr, RemoteInstall::API::ProviderCount()))
				SelectedProviderStatic = RemoteInstall::API::GetProvider(ProviderIndex).Static;
			
			PAGE_RESET_FOCUS;
			if (ImGui::IsItemFocused())
				ImGui::SetScrollY(0);

			if (!SelectedProviderStatic) {
				ImGui::SameLine();
				if (ImGui::Button("随机主题"))
					StartRemoteInstallFixed(RemoteInstall::FixedTypes::Random);
				CurItemBlockLeft();
				ImGui::SameLine();
				if (ImGui::Button("最新主题"))
					StartRemoteInstallFixed(RemoteInstall::FixedTypes::Recent);
				CurItemBlockLeft();
			}

			ImGui::TextWrapped("按ID搜索主题");
			ImGui::PushStyleColor(ImGuiCol_Button, 0xDFDFDFDF);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xEFEFEFEF);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFFFFFFFF);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
			if (ImGui::Button(RemoteInstallBtnText.c_str(), { 300, 0 }))
				SetRemoteInstallCode(PlatformTextInput(RemoteInstallCode.c_str()));
			ImGui::PopStyleColor(4);
			ImGui::SameLine(0, 20);
			if (ImGui::Button("搜索###SearchBtn", { 150, 0 }) && RemoteInstallCode != "")
				StartRemoteInstallByCode();
			CurItemBlockLeft();
			ImGui::TextWrapped("ID不是名称，按名称搜索主题将不起作用，请在浏览器中打开您的源，选择一个主题，它应该会显示其唯一ID。");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::PushFont(font40);
		ImGui::Text("从主题注入器远程安装");
		ImGui::PopFont();
	}

	if (RemoteInstallFile)
	{
		if (RemoteInstallFile->Render() == ThemeEntry::UserAction::Enter || AutoInstall)
			PushFunction([this]() {
				RemoteInstallFile->Install(!AutoInstall);
				RemoteInstallFile = nullptr;

				if (AutoInstall)
					PlatformReboot();
			});
		if (ImGui::IsWindowFocused())
			Utils::ImGuiSelectItem();

		ImGui::TextWrapped("按A键安装，按B键取消");
	}
	else 
	{
		ImGui::TextWrapped("您可以直接从电脑上的主题注入器安装主题，转到'NXTheme builder'选项卡并单击'远程安装...'");
		if (ImGui::Button(BtnStart.c_str()))
		{
			if (!server.IsHosting())
				StartServer();
			else
				StopServer();
		}
		if (UseLowMemory) PAGE_RESET_FOCUS;
		ImGui::TextWrapped("保持在此页面，否则请求将无法执行");
		ImGui::Checkbox("自动安装并重启", &AutoInstall);
	}
	Utils::ImGuiSetWindowScrollable();

	Utils::ImGuiCloseWin();
}

void RemoteInstallPage::StartRemoteInstallByCode()
{
	PushFunction([this]() {
		try {
			DisplayLoading("正在加载...");
			RemoteInstall::Begin(SelectedProvider(), RemoteInstallCode);
		}
		catch (nlohmann::json::type_error& ex)	{
			DialogBlocking("解析服务器响应时出错，这通常意味着您请求的代码找不到，请确保代码有效。\n\n错误信息:\n"s + ex.what());
		}
		catch (std::exception& ex) {
			DialogBlocking("处理请求时出错，请确保代码有效且您已连接到互联网。\n\n错误信息:\n"s + ex.what());
		}
	});
}

void RemoteInstallPage::StartRemoteInstallFixed(RemoteInstall::FixedTypes type)
{
	PushFunction([this, type]() {
		try {
			DisplayLoading("正在加载...");
			RemoteInstall::BeginType(SelectedProvider(), type);
		}
		catch (std::exception& ex) {
			DialogBlocking("处理请求时出错，请确保您已连接到互联网，稍后再试，如果仍然不起作用，可能是所选源不支持此功能。\n\n错误信息:\n"s + ex.what());
		}
	});
}

void RemoteInstallPage::CurItemBlockLeft() 
{
	AllowLeft &= !ImGui::IsItemFocused();
}

void RemoteInstallPage::Update()
{
	if (RemoteInstallFile && KeyPressed(GLFW_GAMEPAD_BUTTON_B))
	{
		RemoteInstallFile = nullptr;
		return;
	}

	if (Utils::PageLeaveFocusInput(AllowLeft)) {
		Parent->PageLeaveFocus(this);
		return;
	}

	if (RemoteInstallFile) return;

	UpdateServer();
}

void RemoteInstallPage::SetRemoteInstallCode(const char* input)
{
	RemoteInstallCode = std::string(input);
	if (RemoteInstallCode == "")
		RemoteInstallBtnText = "输入文本###themeIDinput";
	else
		RemoteInstallBtnText = RemoteInstallCode + "###themeIDinput";
}

void RemoteInstallPage::StartServer()
{
	try 
	{
		server.StartHosting();
		BtnStart = ("IP: " + server.GetHostname() + " - 按此停止###InstallBtn");
	}
	catch (std::exception& ex)
	{
		Dialog(ex.what());
	}
}

void RemoteInstallPage::StopServer()
{
	server.StopHosting();
	BtnStart = "开始远程安装###InstallBtn";
}

void RemoteInstallPage::DialogError(const std::string &msg)
{
	Dialog("出现错误，请重试。\n" + msg);
}

const RemoteInstall::Provider& RemoteInstallPage::SelectedProvider()
{
	return RemoteInstall::API::GetProvider(ProviderIndex);
}

void RemoteInstallPage::UpdateServer()
{	
	try 
	{
		server.HostUpdate();
		if (server.HasFinished())
		{
			RemoteInstallFile = ThemeEntry::FromSZS(server.Buffer());
			server.Clear();
			StopServer();
		}
	}
	catch (std::exception& ex)
	{
		Dialog(ex.what());
		StopServer();
	}
}