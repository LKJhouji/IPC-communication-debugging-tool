// =====================================================================================
//
//       Filename:  sock_basic.h
//
//    Description:  socket basic for all os. move the code from lic_reg.h to here.
//                     code edit with vim and and tw=2, default code is under utf-8.
//
//        Version:  $Id$
//        Created:  2023/1/13 0:24:50
//       Revision:  none
//       Compiler:  modern c/c++ compiler, at least c++17 is requiment.
//                        ( test under gcc 10.2.1 and vs2019 ).
//
//         Author:  lk (yy), lk@shucantech.com
//				Company:  sct
//   Organization:  sct-p
//
//				License:  sct-cl v0
//
// =====================================================================================


#pragma once


#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef	_WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif



#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <thread>
#include <ctime>

#define USE_STDSTL
#ifdef USE_STDSTL
#include <string>

using string = std::string;
#else
#include "../thy/adt/string.hpp"

using string = kk::adt::string::string;
#endif

#ifdef _WIN32
#pragma comment(lib,"Ws2_32.lib")
#endif

#ifndef _WIN32
using SOCKET = int;
#endif



static const int def_sbz = 1024 * 512;
static const int def_rbz = 1024 * 512;

static void close_sid(int sd) {	 //close socket id
#ifdef _WIN32
	closesocket(sd);
#else
	close(sd);
#endif
}

static void clear_socket(SOCKET sd) {
#ifdef _WIN32
	closesocket(sd);
	WSACleanup();
#else
	close(sd);
#endif
}

static string get_ip(const string& dn) {	// 获取给定域名ip地址
#ifdef _WIN32
	WSADATA wsad;
	int srt = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (srt != 0) { return ""; }
#endif
	struct hostent* h = gethostbyname(dn.c_str());
	if (h == NULL) { return ""; }
	return inet_ntoa(*(struct in_addr*)h->h_addr_list[0]);
}

static void sockopt_info(int sd) {	//获取套接字信息并打印出来
	int reuse;	//是否允许地址重用
	int broadcast;	//是否套接字可以广播
	linger lin;	//套接字关闭的行为
	int rcvbuf;	//接受缓冲区大小
	int sndbuf;	//发送缓冲区大小
	int rcvtimeo;	//接受超时时间
	int sndtimeo;	//发送超时时间
	int keepalive;	//keep-alive功能
#ifdef _WIN32
	int sz;
#else
	socklen_t sz;
#endif
	sz = sizeof(reuse);
	getsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, &sz);
	sz = sizeof(broadcast);
	getsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, &sz);
	sz = sizeof(lin);
	getsockopt(sd, SOL_SOCKET, SO_LINGER, (char*)&lin, &sz);
	sz = sizeof(rcvbuf);
	getsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&rcvbuf, &sz);
	sz = sizeof(sndbuf);
	getsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char*)&sndbuf, &sz);
	sz = sizeof(rcvtimeo);
	getsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, &sz);
	sz = sizeof(sndtimeo);
	getsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char*)&sndtimeo, &sz);
	sz = sizeof(keepalive);
	getsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepalive, &sz);

	printf("reuse = %s\n", reuse ? "true" : "false");
	printf("broacast = %s\n", broadcast ? "true" : "false");
#ifdef _WIN32
	printf("linger onoff, linger = %hu, %hu\n", lin.l_onoff, lin.l_linger);
#else
	printf("linger onoff, linger = %d, %d\n", (int)lin.l_onoff, (int)lin.l_linger);
#endif
	printf("rcvbuf = %d\n", rcvbuf);
	printf("sndbuf = %d\n", sndbuf);
	printf("rcvtimeo = %d\n", rcvtimeo);
	printf("sndtimeo = %d\n", sndtimeo);
	printf("keepalive = %s\n", keepalive ? "true" : "false");
}

static void init_sockopt(SOCKET sd, bool tcp = true, int rcv_bfz = def_rbz, int snd_bfz = def_sbz) {
#ifdef _WIN32
	BOOL 	reuse = FALSE;
	BOOL broadcast = FALSE;
	DWORD rcvtimeo = 0; //:block send data unit ms.
	DWORD sndtimeo = 0; //:block send data unit ms.
	BOOL keepalive = TRUE;
#else
	int reuse = 0;
	int broadcast = 0;
	int rcvtimeo = 0; //:block send data unit ms.
	int sndtimeo = 0; //:block send data unit ms.
	int keepalive = 1;
#endif
	int rcvbuf = def_rbz;
	int sndbuf = def_sbz;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
		printf("setsockopt reuseaddr with error info %s!\n", strerror(errno));
	}
	if (!tcp) {
		if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) < 0) {
			printf("setsockopt broacast with error info %s!\n", strerror(errno));
		}
		/*
		if (setsockopt(sd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) < 0) {
			printf("setsockopt linger with error info %s!\n", strerror(errno));
		}
		*/
	}
	if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&rcvbuf, sizeof(rcvbuf)) < 0) {
		printf("setsockopt rcvbuf with error info %s!\n", strerror(errno));
	}
	if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char*)&sndbuf, sizeof(sndbuf)) < 0) {
		printf("setsockopt sndbuf with error info %s!\n", strerror(errno));
	}
	/*
	if( setsockopt( sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, sizeof( rcvtimeo ) ) < 0 ){
	printf("setsockopt rcvtimeo with error info %s!\n", strerror(errno));
	}
	if( setsockopt( sd, SOL_SOCKET, SO_SNDTIMEO, (char*)&sndtimeo, sizeof( sndtimeo ) ) < 0 ){
	printf("setsockopt sndtimeo with error info %s!\n", strerror(errno));
	}
	*/
	if (tcp) {
		if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepalive, sizeof(keepalive)) < 0) {
			printf("setsockopt keepalive with error info %s!\n", strerror(errno));
		}
	}
}

static void set_noblock_mode(SOCKET sd) {	//设置套接字为非阻塞状态
#ifdef _WIN32
	unsigned long m = 1;//:nonzero non-blocking mode, zero is blocking mode.
	ioctlsocket(sd, FIONBIO, &m);
#else
	int flags = fcntl(sd, F_GETFL, 0);
	if (flags >= 0) { fcntl(sd, F_SETFL, flags | O_NONBLOCK); }
#endif
}

static void set_block_mode(SOCKET sd) {
#ifdef _WIN32
	unsigned long m = 0;//:nonzero non-blocking mode, zero is blocking mode.
	ioctlsocket(sd, FIONBIO, &m);
#else
	int flags = fcntl(sd, F_GETFL, 0); flags &= (~O_NONBLOCK);
	if (flags >= 0) { fcntl(sd, F_SETFL, flags); }
#endif
}

static SOCKET connect_to(const char* ip, const uint16_t& p, bool tcp = true) { //创建一个套接字并与指定的 IP 地址和端口进行连接。
#ifdef _WIN32
	WSADATA wsad;
	int srt = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (srt != 0) { return -1; }
#endif

	SOCKET sd;
	sd = socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
	//printf("setsockopt with the follow paramter.\n");
	init_sockopt(sd);
	//sockopt_info(sd);
	struct sockaddr_in sa;	//地址连接结构体？
	memset(&sa, 0x0, sizeof(sa));
	sa.sin_family = AF_INET;	//地址族
	sa.sin_addr.s_addr = inet_addr(ip);
	sa.sin_port = htons(p);

	int r = connect(sd, (struct sockaddr*)&sa, sizeof(struct sockaddr));
	if (r < 0) { return -1; }
	return sd;
}

static SOCKET connect_to_timeout(const char* ip, const uint16_t& p, int timeout, bool tcp = true) {	//超时阻塞
	SOCKET sd = -1;
#ifdef _WIN32
	WSADATA wsad;
	int srt = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (srt != 0) { return sd; }
#endif
	sd = socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, tcp ? IPPROTO_TCP : IPPROTO_UDP);
	//printf("setsockopt with the follow paramter.\n");
	init_sockopt(sd, tcp);
	//sockopt_info(sd);
	struct sockaddr_in sa;
	memset(&sa, 0x0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ip);
	sa.sin_port = htons(p);
	set_noblock_mode(sd);
	int r = connect(sd, (struct sockaddr*)&sa, sizeof(struct sockaddr));
	if (!tcp) { bind(sd, (struct sockaddr*)&sa, sizeof(struct sockaddr)); }
	if (r == 0) { set_block_mode(sd); return sd; }
#ifdef _WIN32
	if (r == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
#else
	if (r < 0 && errno == EINPROGRESS) {
#endif
		timeval t;
		t.tv_sec = timeout;
		t.tv_usec = 0;
		fd_set w, e;
		FD_ZERO(&w);
		FD_ZERO(&e);
		FD_SET(sd, &w);
		FD_SET(sd, &e);
		int max_fds = (int)sd;
		int rc = select(max_fds + 1, NULL, &w, &e, &t);
		if (rc >= 1 && FD_ISSET(sd, &w)) {
			set_block_mode(sd);
		}
		else { sd = -1; }
		FD_CLR(sd, &w);
		FD_CLR(sd, &e);
	}
	else { sd = -1; }
	return sd;
}


