/*
 * Copyright (C) 2004-2012  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <znc/Modules.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>
#include <znc/Chan.h>
#if (VERSION_MAJOR >= 1) && (VERSION_MINOR >= 5)
#include <znc/Query.h>
#endif
using std::vector;

#define AWAY_DEFAULT_REASON "Auto away at %time%"

class CClientAwayMod : public CModule {
public:
	CString GetAwayReason() const {
		CString sAway = GetNV("reason");

		if (sAway.empty()) {
			sAway = AWAY_DEFAULT_REASON;
		}

		if (m_pNetwork) {
			return m_pNetwork->ExpandString(sAway);
		}

		return m_pUser->ExpandString(sAway);
	}

	bool GetAutoAway() const {
		return GetNV("autoaway").ToBool();
	}

	void ListCommand(const CString &sLine) {
		CTable Table;
		Table.AddColumn("Host");
		Table.AddColumn("Network");
		Table.AddColumn("Away");

		const vector<CClient*> vClients = m_pUser->GetAllClients();

		for (vector<CClient*>::const_iterator it = vClients.begin(); it != vClients.end(); ++it) {
			CClient *pClient = *it;

			Table.AddRow();
			Table.SetCell("Host", pClient->GetRemoteIP());
			if (pClient->GetNetwork()) {
				Table.SetCell("Network", pClient->GetNetwork()->GetName());
			}
			Table.SetCell("Away", CString(pClient->IsAway()));
		}

		PutModule(Table);
	}

	void SetAwayReasonCommand(const CString &sLine) {
		CString sReason = sLine.Token(1, true);

		if (!sReason.empty()) {
			SetNV("reason", sReason);
			PutModule("Away reason set to [" + sReason + "]");
		}

		PutModule("Away message will be expanded to [" + GetAwayReason() + "]");
	}

	void AutoAwayCommand(const CString &sLine) {
		SetNV("autoaway", sLine.Token(1));

		if (GetAutoAway()) {
			PutModule("Auto away when last client goes away or disconnects enabled.");
		} else {
			PutModule("Auto away when last client goes away or disconnects disabled.");
		}
	}

	void SetAwayCommand(const CString &sLine) {
		const vector<CClient*> vClients = m_pUser->GetAllClients();

		CString sHostname = sLine.Token(1);
		unsigned int count = 0;

		for (vector<CClient*>::const_iterator it = vClients.begin(); it != vClients.end(); ++it) {
			CClient *pClient = *it;

			if (sHostname.empty() || pClient->GetRemoteIP().Equals(sHostname)) {
				pClient->SetAway(true);
				++count;
			}
		}

		if (count == 1) {
			PutModule(CString(count) + " client has been set away");
		} else {
			PutModule(CString(count) + " clients have been set away");
		}
	}

	MODCONSTRUCTOR(CClientAwayMod) {
		AddHelpCommand();
		AddCommand("List", static_cast<CModCommand::ModCmdFunc>(&CClientAwayMod::ListCommand),
			"", "List all clients");
		AddCommand("Reason", static_cast<CModCommand::ModCmdFunc>(&CClientAwayMod::SetAwayReasonCommand),
			"[reason]", "Prints and optionally sets the away reason.");
		AddCommand("AutoAway", static_cast<CModCommand::ModCmdFunc>(&CClientAwayMod::AutoAwayCommand),
			"yes or no", "Should we auto away you when the last client goes away or disconnects");
		AddCommand("SetAway", static_cast<CModCommand::ModCmdFunc>(&CClientAwayMod::SetAwayCommand),
				"hostname", "Set any clients matching hostname as away/unaway");
	}

	virtual void OnClientLogin() {
		if (GetAutoAway() && m_pNetwork && m_pNetwork->IsIRCAway()) {
			PutIRC("AWAY");
		}
	}

	virtual void OnClientDisconnect() {
		if (GetAutoAway() && m_pNetwork && !m_pNetwork->IsIRCAway() && !m_pNetwork->IsUserOnline()) {
			PutIRC("AWAY :" + GetAwayReason());
		}
	}

	virtual void OnIRCConnected() {
		if (GetAutoAway() && !m_pNetwork->IsUserOnline()) {
			PutIRC("AWAY :" + GetAwayReason());
		}
	}

	virtual EModRet OnUserRaw(CString& sLine) {
		CString sCmd = sLine.Token(0);

		if (sCmd.Equals("AWAY")) {
			if (m_pClient->IsAway()) {
				m_pClient->SetAway(false);
				m_pClient->PutClient(":irc.znc.in 305 " + m_pClient->GetNick() + " :You are no longer marked as being away");

				if (m_pNetwork) {
					const vector<CChan*>& vChans = m_pNetwork->GetChans();
					vector<CChan*>::const_iterator it;

					for (it = vChans.begin(); it != vChans.end(); ++it) {
						// Skip channels which are detached or we don't use keepbuffer
						if (!(*it)->IsDetached() && (*it)->AutoClearChanBuffer()) {
							(*it)->ClearBuffer();
						}
					}

#if (VERSION_MAJOR >= 1) && (VERSION_MINOR >= 5)
					for (CQuery* pQuery : m_pNetwork->GetQueries()) {
						m_pNetwork->DelQuery(pQuery->GetName());
					}
#else
					m_pNetwork->ClearQueryBuffer();
#endif

					if (GetAutoAway() && m_pNetwork->IsIRCAway()) {
						PutIRC("AWAY");
					}
				}
			} else {
				m_pClient->SetAway(true);
				m_pClient->PutClient(":irc.znc.in 306 " + m_pClient->GetNick() + " :You have been marked as being away");

				if (GetAutoAway() && m_pNetwork && !m_pNetwork->IsIRCAway() && !m_pNetwork->IsUserOnline()) {
					// Use the supplied reason if there was one
					CString sAwayReason = sLine.Token(1, true).TrimPrefix_n();

					if (sAwayReason.empty()) {
						sAwayReason = GetAwayReason();
					}

					PutIRC("AWAY :" + sAwayReason);
				}
			}

			return HALTCORE;
		}

		return CONTINUE;
	}

	virtual EModRet OnRaw(CString& sLine) {
		// We do the same as ZNC would without the OnRaw hook,
		// except we do not forward 305's or 306's to clients

		CString sCmd = sLine.Token(1);

		if (sCmd.Equals("305")) {
			m_pNetwork->SetIRCAway(false);
			return HALTCORE;
		} else if (sCmd.Equals("306")) {
			m_pNetwork->SetIRCAway(true);
			return HALTCORE;
		}

		return CONTINUE;
	}
};

template<> void TModInfo<CClientAwayMod>(CModInfo& Info) {
	Info.SetWikiPage("clientaway");
	Info.AddType(CModInfo::NetworkModule);
}

USERMODULEDEFS(CClientAwayMod, "This module allows you to set clients away independently, and auto away")

