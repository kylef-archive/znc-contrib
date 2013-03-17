/*
 * Copyright (C) 2004-2012  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <znc/IRCNetwork.h>
#include <znc/Modules.h>

using std::vector;

#define ISIGNORED(NICK, TYPE) if (IsIgnored(NICK, TYPE)) {  \
			return HALT;  \
		}  \
		return CONTINUE;  \

class CIgnore {
public:
	CIgnore(const CIgnore& Ignore) {
		m_sHostMask = Ignore.GetHostmask();
		m_sType = Ignore.GetType();
	}

	CIgnore(const CString& sHostMask, const CString& sType) {
		CNick Nick;
		Nick.Parse(sHostMask);

		m_sHostMask = ((Nick.GetNick().empty()) ? "*" : Nick.GetNick());

		m_sHostMask += "!";
		m_sHostMask += (Nick.GetIdent().empty()) ? "*" : Nick.GetIdent();

		m_sHostMask += "@";
		m_sHostMask += (Nick.GetHost().empty()) ? "*" : Nick.GetHost();

		m_sType = sType;
	}

	CIgnore(const CString& sString) {
		m_sHostMask = sString.Token(0);
		m_sType = sString.Token(1);
	}

	bool IsMatch(const CNick& Nick, const CString& sType) const {
		return (GetType().Equals("all") || GetType().Equals(sType)) &&
			   (Nick.GetHostMask().WildCmp(m_sHostMask));
	}

	const CString ToString() const {
		return m_sHostMask + " " + m_sType;
	}

	const CString& GetHostmask() const {
		return m_sHostMask;
	}

	const CString& GetType() const {
		return m_sType;
	}

protected:
	CString m_sHostMask;
	CString m_sType;
};

class CIgnoreMod : public CModule {
public:
	MODCONSTRUCTOR(CIgnoreMod) {
		AddCommand("help",     static_cast<CModCommand::ModCmdFunc>(&CIgnoreMod::PrintHelp));
		AddCommand("ignore",   static_cast<CModCommand::ModCmdFunc>(&CIgnoreMod::HandleIgnoreCommand),
			"hostmask [type]", "Ignore a hostmask");
		AddCommand("unignore", static_cast<CModCommand::ModCmdFunc>(&CIgnoreMod::HandleUnignoreCommand),
			"hostmask", "Unignore a hostmask");
		AddCommand("list",     static_cast<CModCommand::ModCmdFunc>(&CIgnoreMod::HandleListCommand));
	}

	void PrintHelp(const CString& sLine) {
		HandleHelpCommand(sLine);

		PutModule("The following types are availible:");

		CTable Table;
		Table.AddColumn("Type");

		Table.AddRow();
		Table.SetCell("Type", "all");

		Table.AddRow();
		Table.SetCell("Type", "privmsg");

		Table.AddRow();
		Table.SetCell("Type", "ctcp");

		Table.AddRow();
		Table.SetCell("Type", "notice");

		PutModule(Table);
	}

	void HandleIgnoreCommand(const CString& sLine) {
		CString sHostmask = sLine.Token(1);
		CString sType = sLine.Token(2);

		if (sType.empty()) {
			sType = "all";
		} else if (sType.Equals("*")) {
			sType = "all";
		}

		if (!(sType.Equals("all") || sType.Equals("privmsg") || sType.Equals("notice") || sType.Equals("ctcp"))) {
			PutModule("Unknown type");
			return;
		}

		CIgnore Ignore(sHostmask, sType);
		m_vIgnores.push_back(Ignore);

		PutModule("Ignore added (" + Ignore.GetHostmask() + ")");
	}

	void HandleUnignoreCommand(const CString& sLine) {
		CString sHostmask = sLine.Token(1);

		unsigned int uiIgnores = m_vIgnores.size();

		for (vector<CIgnore>::iterator it = m_vIgnores.begin(); it != m_vIgnores.end();) {
			CIgnore& Ignore = *it;

			if (Ignore.GetHostmask().Equals(sHostmask)) {
				it = m_vIgnores.erase(it);
			} else {
				++it;
			}
		}

		unsigned int uiRemoved = uiIgnores - m_vIgnores.size();
		PutModule(CString(uiRemoved) + " hostmask" + (uiRemoved == 1? "" : "s") + " deleted.");
	}

	void HandleListCommand(const CString& sLine) {
		CTable Table;
		Table.AddColumn("Hostmask");
		Table.AddColumn("Type");

		for (vector<CIgnore>::const_iterator it = m_vIgnores.begin(); it != m_vIgnores.end(); ++it) {
			const CIgnore& Ignore = *it;

			Table.AddRow();
			Table.SetCell("Hostmask", Ignore.GetHostmask());
			Table.SetCell("Type",     Ignore.GetType());
		}

		if (!PutModule(Table)) {
			PutModule("You are not ignoring anyone");
		}
	}

	bool IsIgnored(const CNick& Nick, const CString& sType) const {
		for (vector<CIgnore>::const_iterator it = m_vIgnores.begin(); it != m_vIgnores.end(); ++it) {
			CIgnore Ignore = *it;
			if (Ignore.IsMatch(Nick, sType)) {
				return true;
			}
		}

		return false;
	}

	virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage) {
		ISIGNORED(Nick, "privmsg");
	}

	virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {
		ISIGNORED(Nick, "privmsg");
	}

	virtual EModRet OnChanNotice(CNick& Nick, CChan& Channel, CString& sMessage) {
		ISIGNORED(Nick, "notice");
	}

	virtual EModRet OnPrivNotice(CNick& Nick, CString& sMessage) {
		ISIGNORED(Nick, "notice");
	}

	virtual EModRet OnPrivCTCP(CNick& Nick, CString& sMessage) {
		ISIGNORED(Nick, "ctcp");
	}

	virtual EModRet OnChanCTCP(CNick& Nick, CChan& Channel, CString& sMessage) {
		ISIGNORED(Nick, "ctcp");
	}

protected:
	vector<CIgnore> m_vIgnores;
};

NETWORKMODULEDEFS(CIgnoreMod, "Ignore a hostmask")
