#include "ThemeEntry.hpp"
#include "../../fs.hpp"
#include "../../SwitchTools/PatchMng.hpp"
#include "../../SwitchThemesCommon/SarcLib/Yaz0.hpp"
#include "../../ViewFunctions.hpp"
#include "../../SwitchThemesCommon/SwitchThemesCommon.hpp"
#include "../ThemePage.hpp"

class LegacyEntry : public ThemeEntry
{
public:
	LegacyEntry(const std::string& fileName, std::vector<u8>&& RawData)
	{
		FileName = fileName;
		file = RawData;
		auto DecompressedFile = Yaz0::Decompress(file);
		ParseLegacyTheme(SARC::Unpack(DecompressedFile));
	}

	LegacyEntry(const std::string& fileName, SARC::SarcData&& _SData)
	{
		FileName = fileName;

		auto packed = SARC::Pack(_SData);
		file = move(Yaz0::Compress(packed.data));

		ParseLegacyTheme(std::move(_SData));
	}

	bool IsFolder() override { return false; }
	bool CanInstall() override { return _CanInstall; }
	bool HasPreview() override { return false; }
protected:
	bool DoInstall(bool ShowDialogs = true) override
	{
		if (ShowDialogs)
			ThemeEntry::DisplayInstallDialog(FileName);

		auto patch = SwitchThemesCommon::SzsPatcher::DetectSarc(SData);

		if (!patch)
			throw std::runtime_error("找不到匹配的主题模板");

		if (!PatchMng::ExefsCompatAsk(patch->szsName))
			return false;

		fs::theme::CreateStructure(patch->TitleId);
		fs::WriteFile(fs::path::RomfsFolder(patch->TitleId) + "lyt/" + patch->szsName, file);

		return true;
	}

private:
	bool _CanInstall = true;
	SARC::SarcData SData;

	LoadedImage GetPreview() override
	{
		throw std::runtime_error("（“针对 szs 主题的预览功能尚未实现”）");
	}

	void ParseLegacyTheme(SARC::SarcData&& _Sdata)
	{	
		SData = _Sdata;
		if (FileName == "")
		{
			lblFname = "Unknown.szs";
			lblLine1 = "远程安装";
		}
		else
		{
			lblFname = fs::GetFileName(FileName);
			lblLine1 = FileName;
		}
		auto patch = SwitchThemesCommon::SzsPatcher::DetectSarc(SData);
		if (!patch)
		{
			lblLine2 = "无效主题";
			CannotInstallReason = "找不到兼容的补丁模板";
			_CanInstall = false;
		}

		else lblLine2 = (patch->TemplateName + " 适用于 " + patch->FirmName);
	}
};