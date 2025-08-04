#include "QlaunchPatchPage.hpp"
#include "../ViewFunctions.hpp"
#include "RemoteInstall/Worker.hpp"

class ThemeUpdateDownloader : public RemoteInstall::Worker::BaseWorker {
public:
	struct Result {
		std::string error;
		long httpCode;
		std::vector<u8> data;
	};

	ThemeUpdateDownloader(const std::string& url, Result& r) : BaseWorker({url}, true), OutResult(r) {
		appendUrlToError = false;
		SetLoadingLine("正在检查补丁更新...");
	}

protected:
	void OnComplete() {
		const auto& str = Errors.str();
		if (str.length())
			OutResult.error = str;
		else
			OutResult.data = Results.at(0);
	}

	bool OnFinished(uintptr_t index, long httpCode) override {
		OutResult.httpCode = httpCode;
		return true;
	}

	Result& OutResult;
};

QlaunchPatchPage::QlaunchPatchPage() : IPage("主题补丁") { }

void QlaunchPatchPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);

	ImGui::TextWrapped(
		"从固件9.0开始，主菜单的某些部分需要补丁才能安装主题。\n"
		"如果您看到此页面，表示您尚未安装适用于您固件的补丁。"
	);	

	if (PatchMng::QlaunchBuildId() != "")
	{
		ImGui::Text("您的主菜单版本如下 (BuildID) :");
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		Utils::ImGuiCenterString(PatchMng::QlaunchBuildId());
		ImGui::PopStyleColor();
	}
	else 
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
		ImGui::Text("错误：无法检测到您的主菜单版本");
		ImGui::PopStyleColor();
	}

	if (patchStatus == PatchMng::InstallResult::MissingIps) 
	{		
		ImGui::TextWrapped("当前不支持此版本，新固件发布后通常需要几天时间来更新补丁");
		ImGui::TextWrapped(
			"启动此应用程序时会自动从互联网下载新补丁。"
			"如果您愿意，也可以立即检查更新。"
		);
		
		if (ImGui::Button("检查更新"))
			PushFunction([this]() { CheckForUpdates(); });

		if (updateMessageString != "")
		{
			ImGui::SameLine();

			if (updateMessageIsError)
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
			else ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
			
			ImGui::TextWrapped(updateMessageString.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::TextWrapped(
			"如果您不想将主机连接到互联网，可以按照以下说明手动下载补丁:"
		);
		
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		ImGui::Text("https://github.com/exelix11/theme-patches");
		ImGui::PopStyleColor();
	}
	else if (patchStatus == PatchMng::InstallResult::SDError)
	{
		ImGui::TextWrapped(
			"读取或写入SD卡文件时出错，这通常意味着您的SD卡已损坏。\n"
			"请运行归档位修复，如果仍然不起作用，请格式化SD卡并重新设置。"
		);
	}
	else if (patchStatus == PatchMng::InstallResult::UnsupportedCFW)
	{
		ImGui::TextWrapped(
			"您的CFW似乎不受支持。\n"
			"如果您的CFW受支持但看到此消息，则可能是SD卡出现问题，请重新安装CFW。"
		);
	}
	else if (patchStatus == PatchMng::InstallResult::Ok)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		ImGui::Text("更新成功，请重启主机！");
		ImGui::PopStyleColor();
	}

	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void QlaunchPatchPage::Update()
{
	if (Utils::PageLeaveFocusInput())
		Parent->PageLeaveFocus(this);
}

void QlaunchPatchPage::CheckForUpdates() {
	ThemeUpdateDownloader::Result res;
	PushPageBlocking(new ThemeUpdateDownloader("https://exelix11.github.io/theme-patches/ips/" + PatchMng::QlaunchBuildId(), res));

	if (res.error != "")
	{
		updateMessageIsError = true;
		updateMessageString = res.error;
	}
	else if (res.httpCode == 404)
	{
		updateMessageIsError = false;
		updateMessageString = "未找到更新";
	}
	else if (res.httpCode != 200)
	{
		updateMessageIsError = true;
		updateMessageString = "HTTP 错误: code " + res.httpCode;
	}
	else
	{
		updateMessageIsError = false;
		fs::patches::WritePatchForBuild(PatchMng::QlaunchBuildId(), res.data);
		patchStatus = PatchMng::EnsureInstalled();
		updateMessageString = "更新成功，请重启主机！";
	}
}

bool QlaunchPatchPage::ShouldShow()
{
	patchStatus = PatchMng::EnsureInstalled();

	if (patchStatus == PatchMng::InstallResult::Ok)
		return false;

	if (patchStatus == PatchMng::InstallResult::MissingIps)
	{
		CheckForUpdates();
		// Has anything changed ? 
		if (patchStatus == PatchMng::InstallResult::Ok)
			return false;
	}
	
	return true;
}
