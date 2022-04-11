//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Status
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include "status.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "luascript.h"
#include <sstream>
#include "game.h"
#include "networkmessage.h"

extern LuaScript g_config;
extern Game g_game;
#define YUR_VERSION "0.9.4f"

Status* Status::_Status = NULL;

Status* Status::instance(){
	if(_Status == NULL)
		_Status = new Status();
	return _Status;
}

Status::Status(){
	this->playersonline = 0;
	this->playersmax    = 0;
	this->playerspeak   = 0;

#ifdef YUR_LOGIN_QUEUE
	this->playerswaiting = 0;
#endif //YUR_LOGIN_QUEUE

	this->start=OTSYS_TIME();
}

void Status::addPlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
	const uint32_t player_ip,
	const std::string player_name
#endif
){
	this->playersonline++;
#ifdef HHB_STATUS_MAX_4_PER_IP
{
	std::unique_lock lock(this->ip_counts_mutex);
	this->ip_counts[player_ip][player_name] +=1;
}
#endif
	if(playerspeak < playersonline)
	  playerspeak = playersonline;
}
void Status::removePlayer(
#ifdef HHB_STATUS_MAX_4_PER_IP
		const uint32_t player_ip,
		const std::string player_name
#endif
){
	this->playersonline--;
#ifdef HHB_STATUS_MAX_4_PER_IP
{
	std::unique_lock lock(this->ip_counts_mutex);

	// IN THEORY WE CAN JUST DO: this->ip_counts[player_ip][player_name] -=1;
	// BUT IT DOESN'T WORK, LEADS TO INTEGER UNDERFLOW AND 999999 PLAYERS AND SHIT,
	// SO WE HAVE TO DO IT THIS SUPER CAREFUL WAY. (WHY? IDFK!)
	if(this->ip_counts.count(player_ip) < 1){
		// this should be impossible, but it does happen anyway, idk why and its confusing as fuk...
		// do nothing.
	}else{
		if(this->ip_counts[player_ip].count(player_name) < 1){
			// this should be impossible, but it does happen anyway, idk why and its confusing as fuk...
			// just check if ip is empty to delete and free ram
			if(this->ip_counts[player_ip].size() == 0){
				// free ip ram.
				this->ip_counts.erase(player_ip);
			}
		} else {
			if(this->ip_counts[player_ip][player_name] > 1){
				this->ip_counts[player_ip][player_name] -=1;
			} else {
				// free player_name ram
				this->ip_counts[player_ip].erase(player_name);
				if(this->ip_counts[player_ip].size() == 0){
					// free ip ram.
					this->ip_counts.erase(player_ip);
				}
			}
		}
	}
}
#endif
}

std::string Status::getStatusString(
#ifdef HHB_STATUS_MAX_4_PER_IP
		const bool print_debug_info
#endif
){
	std::string xml;

	std::stringstream ss;

	xmlDocPtr doc;
	xmlNodePtr p, root;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
	root=doc->children;

	xmlSetProp(root, (const xmlChar*) "version", (const xmlChar*)"1.0");


	p=xmlNewNode(NULL,(const xmlChar*)"serverinfo");
	uint64_t running = (OTSYS_TIME() - this->start)/1000;
	ss << running;
	xmlSetProp(p, (const xmlChar*) "uptime", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlSetProp(p, (const xmlChar*) "ip", (const xmlChar*)g_config.getGlobalString("ip", "").c_str());
	xmlSetProp(p, (const xmlChar*) "servername", (const xmlChar*)g_config.getGlobalString("servername", "").c_str());
	xmlSetProp(p, (const xmlChar*) "port", (const xmlChar*)g_config.getGlobalString("port", "").c_str());
	xmlSetProp(p, (const xmlChar*) "location", (const xmlChar*)g_config.getGlobalString("location", "").c_str());
	xmlSetProp(p, (const xmlChar*) "url", (const xmlChar*)g_config.getGlobalString("url", "").c_str());
	xmlSetProp(p, (const xmlChar*) "server", (const xmlChar*)"YurOTS");
	xmlSetProp(p, (const xmlChar*) "version", (const xmlChar*)YUR_VERSION);
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"owner");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)g_config.getGlobalString("ownername", "").c_str());
	xmlSetProp(p, (const xmlChar*) "email", (const xmlChar*)g_config.getGlobalString("owneremail", "").c_str());
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"players");
#ifdef HHB_STATUS_MAX_4_PER_IP
{
	std::shared_lock lock(this->ip_counts_mutex);
	size_t otservlist_legal_online_count = 0;
	size_t debug_count=0;
	for (const auto& [ip, players] : this->ip_counts) {
		size_t count_for_this_ip = 0;
		for(const auto& [player, count] : players){
			count_for_this_ip += count;
			++debug_count;
			if(print_debug_info){
				const std::string debug_name = std::string("debug_")+std::to_string(debug_count);
				const std::string debug_text = std::string("ip: ")+std::to_string(ip)+std::string(" player: ")+std::string(player)+std::string(" count: ")+std::to_string(count);
				xmlSetProp(p, (const xmlChar*) debug_name.c_str(), (const xmlChar*)debug_text.c_str());
			}
		}
		otservlist_legal_online_count += (count_for_this_ip > 4 ? 4 : count_for_this_ip);
    }
	ss << otservlist_legal_online_count;
	xmlSetProp(p, (const xmlChar*) "unique", (const xmlChar*)std::to_string(this->ip_counts.size()).c_str());

}
#else
	ss << this->playersonline;
#endif
	xmlSetProp(p, (const xmlChar*) "online", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << this->playersmax;
	xmlSetProp(p, (const xmlChar*) "max", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << this->playerspeak;
	xmlSetProp(p, (const xmlChar*) "peak", (const xmlChar*)ss.str().c_str());
	ss.str("");
#ifdef YUR_LOGIN_QUEUE
	ss << this->playerswaiting;
	xmlSetProp(p, (const xmlChar*) "waiting", (const xmlChar*)ss.str().c_str());
	ss.str("");
#endif //YUR_LOGIN_QUEUE
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"monsters");
	ss << g_game.getMonstersOnline();
	xmlSetProp(p, (const xmlChar*) "total", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"map");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)this->mapname.c_str());
	xmlSetProp(p, (const xmlChar*) "author", (const xmlChar*)this->mapauthor.c_str());
	xmlSetProp(p, (const xmlChar*) "width", (const xmlChar*)"");
	xmlSetProp(p, (const xmlChar*) "height", (const xmlChar*)"");
	xmlAddChild(root, p);

	xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getGlobalString("motd", "").c_str());

	char *s = NULL;
	int len = 0;
	xmlDocDumpMemory(doc, (xmlChar**)&s, &len);

	if(s)
		xml = std::string(s, len);
	else
		xml = "";

	xmlFreeOTSERV(s);
	xmlFreeDoc(doc);

	return xml;
}

void Status::getInfo(NetworkMessage &nm) {
  // the client selects which information may be
  // sent back, so we'll save some bandwidth and
  // make many
  bool bserverinfo0 = nm.GetByte() == 1;
  bool bserverinfo1 = nm.GetByte() == 1;
  bool bserverinfo2 = nm.GetByte() == 1;
  bool bplayersinfo = nm.GetByte() == 1;
  bool bmapinfo     = nm.GetByte() == 1;

  nm.Reset();
  uint64_t running = (OTSYS_TIME() - this->start) / 1000;
  // since we haven't all the things on the right place like map's
  // creator/info and other things, i'll put the info chunked into
  // operators, so the httpd server will only receive the existing
  // properties of the server, such serverinfo, playersinfo and so

  if (bserverinfo0) {
    nm.AddByte(0x10); // server info
    nm.AddString(g_config.getGlobalString("servername", "").c_str());
    nm.AddString(g_config.getGlobalString("ip", "").c_str());
    nm.AddString(g_config.getGlobalString("port", "").c_str());
  }

  if (bserverinfo1) {
    nm.AddByte(0x11); // server info - owner info
 	  nm.AddString(g_config.getGlobalString("ownername", "").c_str());
    nm.AddString(g_config.getGlobalString("owneremail", "").c_str());
  }

  if (bserverinfo2) {
    nm.AddByte(0x12); // server info - misc
    nm.AddString(g_config.getGlobalString("motd", "").c_str());
    nm.AddString(g_config.getGlobalString("location", "").c_str());
    nm.AddString(g_config.getGlobalString("url", "").c_str());
    nm.AddU32((uint32_t)(running >> 32)); // this method prevents a big number parsing
    nm.AddU32((uint32_t)(running));       // since servers can be online for months ;)
    nm.AddString(YUR_VERSION);
    //nm.AddString("0.4.1");
  }

  if (bplayersinfo) {
    nm.AddByte(0x20); // players info
    nm.AddU32(this->playersonline);
    nm.AddU32(this->playersmax);
    nm.AddU32(this->playerspeak);
  }

  if (bmapinfo) {
    nm.AddByte(0x30); // map info
    nm.AddString(this->mapname.c_str());
    nm.AddString(this->mapauthor.c_str());
    int mw, mh;
    g_game.getMapDimensions(mw, mh);
    nm.AddU16(mw);
    nm.AddU16(mh);
  }

  return;
  // just one thing, I'm good with monospaced text, right?
  // if you haven't undertood the joke, look at the top ^^
}

bool Status::hasSlot() {
  return this->playersonline < this->playersmax;
}

