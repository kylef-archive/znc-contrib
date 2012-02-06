/*
 * Copyright (C) 2004-2012  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <znc/IRCNetwork.h>
#include <znc/Modules.h>

class CPrivMsgMod : public CModule {
public:
	MODCONSTRUCTOR(CPrivMsgMod) {}

	virtual EModRet OnUserMsg(CString& sTarget, CString& sMessage) {
		if (m_pNetwork && m_pNetwork->GetIRCSock() && !m_pNetwork->IsChan(sTarget)) {
			m_pNetwork->PutUser(":" + m_pNetwork->GetIRCNick().GetNickMask() + " PRIVMSG " + sTarget + " :" + sMessage, NULL, m_pClient);
		}

		return CONTINUE;
	}
};

USERMODULEDEFS(CPrivMsgMod, "Send privmsgs")

