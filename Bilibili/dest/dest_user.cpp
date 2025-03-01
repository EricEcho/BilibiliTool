﻿#include "dest_user.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <utility/strconvert.h>
#include "logger/log.h"
#include "api_bl.h" 

dest_user::dest_user() :
	_heartcount(0),
	_usercount(0) {

	GetDir(_cfgfile, sizeof(_cfgfile));
	strcat(_cfgfile, DEF_CONFIG_USER);

	pubkey = "-----BEGIN PUBLIC KEY-----\
\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDqDG9LqVmteJ3xsWv6u/lAt6cU\
\n3KTF4BDNZsSs+zmARQnBnHkaV4nJgRl9IK8b8tCMK6xbFrWa6a9RUENL8vWbclo4\
\nvuqG1/qLtZgo+eFzXbT3tg/XLUrLsdKhU5+w1YJWdw4TQUMbsR0z1F0yOZDwAvRC\
\n3dc9IxriHo2DKIFEqwIDAQAB\
\n-----END PUBLIC KEY-----";
	prikey = "-----BEGIN RSA PRIVATE KEY-----\
\nMIICXQIBAAKBgQDqDG9LqVmteJ3xsWv6u/lAt6cU3KTF4BDNZsSs+zmARQnBnHka\
\nV4nJgRl9IK8b8tCMK6xbFrWa6a9RUENL8vWbclo4vuqG1/qLtZgo+eFzXbT3tg/X\
\nLUrLsdKhU5+w1YJWdw4TQUMbsR0z1F0yOZDwAvRC3dc9IxriHo2DKIFEqwIDAQAB\
\nAoGACggYaRzMHDRUSLy7DRcresukXK+MXHLbJYKnIWbvMwFChsrnIerom/ttlUBm\
\nYQNKTwe8LndNt2MWwZx4FfRG9Jq5KUJJz16Yk+i1JTffFjOmALijyqHdLbc7SZ6p\
\nl82ChNdD8X7k305qULu86itrMMSQ3L6s3IBHoQGypQpf5vECQQD3I1OIzP3rPIo9\
\nzwOXU8LT/vADydW0ttdReesUR2wT2uMDosl5mxh8X36u/Oe72MjW0a/sfLZr5oRa\
\n1ectZMEDAkEA8nDzS+P35BbRI9hIA9X0F/SD8mLe9kVvGUKrriV2sH3FIMdbYkep\
\n9UEFP9ZpQ7lpc0Eq0SCjl8A7vbvqoRMZOQJBAMNDuSu8c9+aTMu7NeYp+yTPKEqF\
\n/YE0efnZL4EtUVp6tqVXyIJ5paYXOZv/HQWRqlX5BVv/yY6FawvuOCLomYsCQQDc\
\n2iH4TzqBsHticOLhg6TxsZAFXSYJKCVV2JM2d/BQRLIv8wt/UxMzVMDob3TC+gNi\
\nt8m+akI8uiRx6d6KTzCZAkBVZQV47puvjcLD75yUhQN5cUO7iqdeFQTFMYcv72DM\
\naIzBAtlfQDwItQM7Ylkquj+Ns2MbYotX5RxWlLmKE15u\
\n-----END RSA PRIVATE KEY-----";

	srand((unsigned int)time(0));
}

dest_user::~dest_user() {
	_ClearUserList();
	BOOST_LOG_SEV(g_logger::get(), debug) << "[UserList] Stop.";
}

int dest_user::ImportUserList() {
	using std::ios;
	using namespace rapidjson;

	_ClearUserList();

	std::ifstream infile;
	infile.open(_cfgfile, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] File does not exist. ";
		return 0;
	}
	infile.seekg(0, ios::end);
	auto inlen = infile.tellg();
	infile.seekg(0, ios::beg);
	char *buff = new char[int(inlen) + 1];
	infile.read(buff, inlen);
	buff[int(inlen)] = 0;
	infile.close();
	rapidjson::Document data;
	data.Parse(buff);
	delete[] buff;
	if (data.GetParseError() != ParseErrorCode::kParseErrorNone) {
		ParseErrorCode ret = data.GetParseError();
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] File error: " << ret;
		return 0;
	}
	try {
		if (!data.IsArray()) {
			throw "Data is not array";
		}
		_user_list.reserve(data.Size());
		for (unsigned i = 0; i < data.Size(); i++) {
			std::shared_ptr<user_info> new_user(new user_info);
			new_user->ReadFileAccount(prikey, data[i], i + 1);
			// 无异常则读取成功
			_usercount++;
			_user_list.push_back(new_user);
			BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import user " << i + 1 << " success.";
		}
	}
	catch (const char* msg) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Import error: " << msg;
		return 0;
	}

	return 0;
}

int dest_user::ExportUserList() {
	using namespace rapidjson;

	if (_usercount == 0) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] No user.";
		return 0;
	}

	try {
		Document data;
		data.SetArray();
		for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
			(*itor)->WriteFileAccount(pubkey, data);
		}
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		data.Accept(writer);
		std::ofstream outfile(_cfgfile);
		outfile << buffer.GetString();
	}
	catch (const char* msg) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Export error: " << msg;
		return 0;
	}

	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Export end.";

	return 0;
}

int dest_user::ShowUserList() {
	printf("[UserList] Count: %d \n", _usercount);
	std::list<user_info*>::iterator itor;
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		printf("%d %s \n", (*itor)->fileid, (*itor)->account.c_str());
	}
	return 0;
}

int  dest_user::AddUser(std::string username, std::string password) {
	if (_IsExistUser(username)) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is already in the list.";
		return -1;
	}
	std::shared_ptr<user_info> new_user(new user_info);
	LOGINRET lret = _ActLogin(new_user, _usercount + 1, username, password);
	if (lret == LOGINRET::NOFAULT) {
		_usercount++;
		_user_list.push_back(new_user);
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " logged in successfully.";
		return 0;
	}
	if (lret == LOGINRET::NOTVALID) {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " is not valid.";
	}
	else {
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " logged in failed.";
	}

	return -1;
}

int  dest_user::DeleteUser(std::string username) {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->account != username) {
			continue;
		}
		// 删除该用户
		itor = _user_list.erase(itor);
		BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " deleted.";
		return 0;
	}
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] " << username << " does not exist.";
	return -1;
}

int dest_user::ReloginAll() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		switch (_ActRelogin(*itor)) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->account.c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->account.c_str());
			break;
		default:
			printf("[UserList] Account %s Login failed. \n", (*itor)->account.c_str());
			break;
		}
	}
	return 0;
}

int dest_user::CheckUserStatusALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		switch (_ActCheckLogin(*itor)) {
		case LOGINRET::NOFAULT:
			printf("[UserList] Account %s Logged in. \n", (*itor)->account.c_str());
			break;
		case LOGINRET::NOTVALID:
			printf("[UserList] Account %s Not valid. \n", (*itor)->account.c_str());
			break;
		default:
			printf("[UserList] Account %s Logged out. \n", (*itor)->account.c_str());
			break;
		}
	}
	return 0;
}

int dest_user::GetUserInfoALL() {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin) {
			continue;
		}
		_ActGetUserInfo(*itor);
	}
	return 0;
}

void dest_user::_ClearUserList() {
	_user_list.clear();
}

bool dest_user::_IsExistUser(std::string username) {
	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if ((*itor)->account == username) {
			return true;
		}
	}
	return false;
}

int dest_user::HeartExp(int firsttime) {
	if (_user_list.empty()) {
		return 0;
	}

	if (firsttime) {
		//初次启动
		_heartcount = 0;
	}
	else {
		//暂定4小时一个周期
		_heartcount++;
		if (_heartcount >= 240) {
			_heartcount = 0;
		}
	}

	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin)
			continue;
		if (_heartcount == 0) {
			_ActHeartFirst(*itor);
		}
		else {
			_ActHeartContinue(*itor);
		}
	}
	return 0;
}

int dest_user::JoinLottery(std::shared_ptr<BILI_LOTTERYDATA> data) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[UserList] Lottery Room: " << data->rrid 
		<< " ID: " << data->loid << " type: " << data->type;
	// 当前没有用户则不领取
	if (!_usercount) {
		return 0;
	}

	// 转换为毫秒
	data->time_start *= 1000;
	data->time_end *= 1000;
	data->time_get *= 1000;

	for (auto itor = _user_list.begin(); itor != _user_list.end(); itor++) {
		if (!(*itor)->islogin) {
			continue;
		}
		// 每个用户使用独立的副本
		std::shared_ptr<BILI_LOTTERYDATA> p(new BILI_LOTTERYDATA(*data));
		switch (p->cmd) {
		case MSG_LOT_STORM: {
			p->time_get += _GetRand(6000, 4000);
			break;
		}
		case MSG_LOT_GIFT: {
			if (p->type == "small_tv" || p->type == "GIFT_30436") {
				// 小电视抽奖需要快速抽争取中奖
				p->time_get += static_cast<long long>(100) + (p->rrid & 0x1ff);
			}
			else {
				// 其它抽奖抽慢点避免中奖
				p->time_get += static_cast<long long>(5000) + (p->rrid & 0xfff);
			}
			break;
		}
		default: {
			p->time_get += _GetRand(5000, 30000);
		}
		}
		(*itor)->post_task(p);
	}

	return 0;
}

LOGINRET dest_user::_ActLogin(std::shared_ptr<user_info>& user, int index, std::string username, std::string password) {
	BILIRET bret;
	user->account = username;
	user->password = password;

	// 将账号转码
	username = toollib::UrlEncode(user->account);

	// 移动端登录
	password = user->password;
	bret = apibl::APIAndGetKey(user, password);
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}
	bret = apibl::APIAndv3Login(user, username, password, "", "");
	if (bret == BILIRET::LOGIN_NEEDVERIFY) {
		// 滑块验证
		std::string challenge, validate;
		std::cout << "Enter the challenge code: ";
		std::cin >> challenge;
		std::cout << "Enter the validate code: ";
		std::cin >> validate;
		// 移动端使用验证码登录
		password = user->password;
		bret = apibl::APIAndGetKey(user, password);
		if (bret != BILIRET::NOFAULT) {
			return LOGINRET::NOTLOGIN;
		}
		bret = apibl::APIAndv3Login(user, username, password, challenge, validate);
	}
	if (bret != BILIRET::NOFAULT) {
		return LOGINRET::NOTLOGIN;
	}

	// 登录成功则获取必要的临时id
	bret = apibl::APIWebGetCaptchaKey(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	user->GetVisitID();
	if (!user->GetToken()) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	user->fileid = index;
	user->islogin = true;

	return LOGINRET::NOFAULT;
}

LOGINRET dest_user::_ActRelogin(std::shared_ptr<user_info>& user) {
	int ret = 0;
	LOGINRET lret;
	ret = user->GetExpiredTime();
	long long rtime;
	rtime = ret - toollib::GetTimeStamp();
	// 有效期小于一周或Token不存在则重新登录
	if ((rtime < 604800) || (user->tokena.empty())) {
		lret = _ActLogin(user, user->fileid, user->account, user->password);
		return lret;
	}
	return LOGINRET::NOFAULT;
}

LOGINRET dest_user::_ActCheckLogin(std::shared_ptr<user_info>& user) {
	BILIRET bret;
	bret = apibl::APIWebGetUserInfo(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	bret = apibl::APIWebGetCaptchaKey(user);
	if (bret != BILIRET::NOFAULT) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	// 生成访问ID
	user->GetVisitID();
	if (!user->GetToken()) {
		user->islogin = false;
		return LOGINRET::NOTLOGIN;
	}
	user->islogin = true;
	return LOGINRET::NOFAULT;
}

int dest_user::_ActGetUserInfo(const std::shared_ptr<user_info>& user) {
	BOOST_LOG_SEV(g_logger::get(), info) << "[User" << user->fileid << "] ";
	apibl::APIWebv1GiftBag(user, 0);
	apibl::APIWebv1CapsuleCheck(user);
	return 0;
}

// 开启经验心跳
void dest_user::_ActHeartFirst(std::shared_ptr<user_info> &user) {
	// 常规心跳
	apibl::APIWebv1HeartBeat(user);
	// 心跳计时标签
	user->heart_deadline = 5;
	// 双端观看奖励
	apibl::APIWebTaskInfo(user);
	// 银瓜子领取信息获取
	apibl::APIAndv1SilverTask(user);
	// 签到
	apibl::APIWebSign(user);
	// 每日礼物
	apibl::APIWebv2GiftDaily(user);
	// 兑换硬币
	if (user->conf_coin == 1) {
		apibl::APIWebv1Silver2Coin(user);
	}
	// 登录硬币
	apibl::APIWebGetCoin(user);
}

// 经验心跳
void dest_user::_ActHeartContinue(std::shared_ptr<user_info> &user) {
	user->heart_deadline--;
	if (user->heart_deadline == 0) {
		user->heart_deadline = 5;
		apibl::APIWebv1HeartBeat(user);
		apibl::APIAndv1Heart(user);
		apibl::APIWebOnlineHeart(user);
	}
	if (user->silver_deadline != -1) {
		user->silver_deadline--;
		if (user->silver_deadline == 0) {
			apibl::APIAndv1SilverAward(user);
			apibl::APIAndv1SilverTask(user);
		}
	}
}

int dest_user::_GetRand(int start, int len)
{
	return rand() % len + start;
}
