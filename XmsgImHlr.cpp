/*
  Copyright 2019 www.dev5.cn, Inc. dev5@qq.com
 
  This file is part of X-MSG-IM.
 
  X-MSG-IM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  X-MSG-IM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU Affero General Public License
  along with X-MSG-IM.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libx-msg-im-hlr-msg.h>
#include <libx-msg-im-hlr-db.h>
#include "XmsgImHlr.h"

XmsgImHlr* XmsgImHlr::inst = new XmsgImHlr();

XmsgImHlr::XmsgImHlr()
{

}

XmsgImHlr* XmsgImHlr::instance()
{
	return XmsgImHlr::inst;
}

bool XmsgImHlr::start(const char* path)
{
	Log::setInfo();
	shared_ptr<XmsgImHlrCfg> cfg = XmsgImHlrCfg::load(path); 
	if (cfg == nullptr)
		return false;
	Log::setLevel(cfg->cfgPb->log().level().c_str());
	Log::setOutput(cfg->cfgPb->log().output());
	Xsc::init();
	shared_ptr<XscTcpServer> priServer(new XscTcpServer(cfg->cfgPb->cgt(), shared_ptr<XmsgImHlrTcpLog>(new XmsgImHlrTcpLog())));
	if (!priServer->startup(XmsgImHlrCfg::instance()->xscServerCfg())) 
		return false;
	shared_ptr<XmsgImN2HMsgMgr> priMsgMgr(new XmsgImN2HMsgMgr(priServer));
	XmsgImHlrMsg::init(priMsgMgr);
	if (!XmsgImHlrDb::instance()->load()) 
		return false;
	XmsgImClientStatusEventQueue::init(); 
	if (!priServer->publish()) 
		return false;
	if (!this->connect2ne(priServer))
		return false;
	Xsc::hold([](ullong now)
	{

	});
	return true;
}

bool XmsgImHlr::connect2ne(shared_ptr<XscTcpServer> priServer)
{
	for (int i = 0; i < XmsgImHlrCfg::instance()->cfgPb->h2n_size(); ++i)
	{
		auto& ne = XmsgImHlrCfg::instance()->cfgPb->h2n(i);
		if (ne.neg() == X_MSG_AP)
		{
			shared_ptr<XmsgAp> ap(new XmsgAp(priServer, ne.addr(), ne.pwd(), ne.alg()));
			ap->connect();
			continue;
		}
		if (ne.neg() == X_MSG_IM_AUTH)
		{
			shared_ptr<XmsgImAuth> auth(new XmsgImAuth(priServer, ne.addr(), ne.pwd(), ne.alg()));
			auth->connect();
			continue;
		}
		LOG_ERROR("unsupported network element group name: %s", ne.ShortDebugString().c_str())
		return false;
	}
	return true;
}

XmsgImHlr::~XmsgImHlr()
{

}

