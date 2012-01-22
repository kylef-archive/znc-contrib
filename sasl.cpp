/*
 * Copyright (C) 2004-2012  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <znc/Modules.h>
#include <znc/IRCNetwork.h>
#include <znc/IRCSock.h>

class CSASLMod : public CModule {
public:
	MODCONSTRUCTOR(CSASLMod) {
		AddHelpCommand();
		AddCommand("Set", static_cast<CModCommand::ModCmdFunc>(&CSASLMod::Set),
			"username password");
	}

	void Set(const CString& sLine) {
		SetNV("username", sLine.Token(1));
		SetNV("password", sLine.Token(2));

		PutModule("Username has been set to [" + GetNV("username") + "]");
		PutModule("Password has been set to [" + GetNV("password") + "]");
	}

	virtual bool OnServerCapAvailable(const CString& sCap) {
		return sCap.Equals("sasl");
	}

	virtual void OnServerCapResult(const CString& sCap, const bool bSucces) {
		if (sCap.Equals("sasl") && bSucces) {
			m_pNetwork->GetIRCSock()->PauseCap();
			PutIRC("AUTHENTICATE PLAIN");
		}
	}

	virtual EModRet OnRaw(CString &sLine) {
		if (sLine.Equals("AUTHENTICATE +")) {
			CString sAuthLine = GetNV("username") + '\0' + GetNV("username")  + '\0' + GetNV("password");
			sAuthLine.Base64Encode();
			PutIRC("AUTHENTICATE " + sAuthLine);
			return HALT;
		} else if (sLine.Token(1).Equals("903") || sLine.Token(1).Equals("904") ||
				sLine.Token(1).Equals("905") || sLine.Token(1).Equals("906") ||
				sLine.Token(1).Equals("907")) {
			m_pNetwork->GetIRCSock()->ResumeCap();
			return HALT;
		}

		return CONTINUE;
	}
};

NETWORKMODULEDEFS(CSASLMod, "Adds support for sasl authentication capability to authenticate to an IRC server")
