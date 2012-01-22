extern  "C" {
	#include <ykclient.h>
}

#include "Modules.h"
#include "znc.h"
#include "User.h"

#define CLIENT_ID 1
#define DEFAULT_TOKEN_ID_LEN 12

class CYubikeyMod : public CModule {
public:
	MODCONSTRUCTOR(CYubikeyMod) {
		AddHelpCommand();
		AddCommand("Add",  static_cast<CModCommand::ModCmdFunc>(&CYubikeyMod::AddTokenCommand), "<token>");
		AddCommand("Del",  static_cast<CModCommand::ModCmdFunc>(&CYubikeyMod::DelTokenCommand), "<token>");
		AddCommand("List", static_cast<CModCommand::ModCmdFunc>(&CYubikeyMod::ListTokens));
	}

	virtual ~CYubikeyMod() {}

	virtual EModRet OnLoginAttempt(CSmartPtr<CAuthBase> Auth) {
		CString const sPassword = Auth->GetPassword();
		CUser *pUser = CZNC::Get().FindUser(Auth->GetUsername());

		if (pUser && CheckToken(pUser, sPassword.Left(DEFAULT_TOKEN_ID_LEN))) {
			DEBUG("yubikey: Lookup for " << sPassword.Left(DEFAULT_TOKEN_ID_LEN));
			// The following call is blocking.
			//int result = ykclient_verify_otp(sPassword.c_str(), CLIENT_ID, NULL);
			int result = ykclient_verify_otp_v2(NULL, sPassword.c_str(), CLIENT_ID, NULL, 0, NULL, NULL);
			DEBUG("yubikey: " << ykclient_strerror(result));

			if (result == YKCLIENT_OK) {
				Auth->AcceptLogin(*pUser);
			} else {
				Auth->RefuseLogin(ykclient_strerror(result));
			}

			return HALT;
		}

		return CONTINUE;
	}

	SCString GetUserTokens(CUser *pUser) {
		SCString ssTokens;
		GetNV(pUser->GetUserName()).Split(" ", ssTokens);
		return ssTokens;
	}

	void SetUserTokens(CUser *pUser, SCString ssTokens) {
		CString sVal;

		for (SCString::const_iterator it = ssTokens.begin(); it != ssTokens.end(); ++it) {
			sVal += *it + " ";
		}

		SetNV(pUser->GetUserName(), sVal);
	}

	bool CheckToken(CUser *pUser, CString sToken) {
		SCString ssTokens = GetUserTokens(pUser);
		return ssTokens.find(sToken) != ssTokens.end();
	}

	void AddToken(CString sToken) {
		SCString ssTokens = GetUserTokens(m_pUser);
		SCString::iterator it = ssTokens.find(sToken);

		if (it == ssTokens.end()) {
			ssTokens.insert(sToken);
			SetUserTokens(m_pUser, ssTokens);
		}
	}

	void DelToken(const CString sToken) {
		SCString ssTokens = GetUserTokens(m_pUser);
		SCString::iterator it = ssTokens.find(sToken);

		if (it != ssTokens.end()) {
			ssTokens.erase(it);
			SetUserTokens(m_pUser, ssTokens);
		}
	}

	void AddTokenCommand(const CString& sLine) {
		CString sToken = sLine.Token(1).Left(DEFAULT_TOKEN_ID_LEN);

		if (sToken.length() != 12) {
			PutModule("Invalid token ID");
			return;
		}

		AddToken(sToken);
		PutModule(sToken + " added");
	}

	void DelTokenCommand(const CString& sLine) {
		CString sToken = sLine.Token(1).Left(DEFAULT_TOKEN_ID_LEN);

		if (sToken.length() != 12) {
			PutModule("Invalid token ID");
			return;
		}

		DelToken(sToken);
		PutModule(sToken + " removed");
	}

	void ListTokens(const CString& sLine) {
		SCString ssTokens = GetUserTokens(m_pUser);

		CTable table;
		table.AddColumn("Tolken");

		for (SCString::const_iterator it = ssTokens.begin(); it != ssTokens.end(); ++it) {
			table.AddRow();
			table.SetCell("Token", *it);
		}

		if (PutModule(table) == 0) {
			PutModule("No tokens set for your user");
		}
	}
};

GLOBALMODULEDEFS(CYubikeyMod, "Allow users to authenticate with a yubikey")
