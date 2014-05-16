"""
Based entirely on privmsg.cpp located at
https://github.com/kylef/znc-contrib because I was too lazy to compile
my own znc.

Copyright (C) 2004-2012

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as published
by the Free Software Foundation.
"""
import znc


class pyprivmsg(znc.Module):
    module_types = [znc.CModInfo.NetworkModule, znc.CModInfo.GlobalModule]
    description = "Send outgoing PRIVMSGs and CTCP ACTIONs to other clients"

    def OnUserMsg(self, target, message):
        net = self.GetNetwork()
        if net.GetIRCSock() and not net.IsChan(target):
            nickmask = net.GetIRCNick().GetNickMask()
            client = self.GetClient()
            self.PutUser(":{} PRIVMSG {} :{}"
                         .format(nickmask, target, message), None, client)

    def OnUserAction(self, target, message):
        net = self.GetNetwork()
        if net.GetIRCSock() and not net.IsChan(target):
            nickmask = net.GetIRCNick().GetNickMask()
            client = self.GetClient()
            self.PutUser(":{} PRIVMSG {} :\x01ACTION {} \x01"
                         .format(nickmask, target, message), None, client)
