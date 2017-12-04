#pragma once
//#include "stdafx.h"
class UserMessage
{
public:
	UserMessage(char *username, char *user_ip_addr) :user_name(username), user_ip_address(user_ip_addr) {}
	UserMessage() :user_name(NULL), user_ip_address(NULL) {}
	char *input_rtsp_url;
	char *out_rtmp_url; 
	bool HasCreateUrl() const { return has_Create_url; }
	char *Get_user_name() const { return user_name; }
	char *Get_user_ipaddr() const { return user_ip_address; }
	static int getRtmpURLByInput(const char*in_fileName, const char *out_fileName);

private:
	bool has_Create_url = false;
	char *user_name;
	char *user_ip_address;
};