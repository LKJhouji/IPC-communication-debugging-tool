// =====================================================================================
//
//       Filename:  ipc_val.h
//
//    Description:  
//                     code edit with vim and and tw=2, default code is under utf-8.
//
//        Version:  $Id$
//        Created:  2023/1/9 20:29:18
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
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>

#define USE_STDSTL

#ifdef USE_STDSTL
#include <string>
#include <vector>
template<typename x> using vector = std::vector<x>;

#else
#include "../thy/adt/string.hpp"
#include "../thy/adt/vector.hpp"

template<typename x> using vector = kk::adt::vector<x>;
#endif

#include "../ipc_com/types.h"

#ifndef __MINGW32__

#ifdef _WIN32
#ifdef IPC_VAL_EXPORT
#define dllemport __declspec(dllexport)
#else
#define dllemport __declspec(dllimport)
#endif
#else
#define dllemport
#endif

#else
#define dllemport
#endif


//use for client 客户端
struct val_info {
#ifdef USE_STDSTL
	using string = std::string;
#else
	using string = kk::adt::string::string;
#endif
	string		 n; //name
	kk::u08    t; //type
	kk::u64    v; //value
	void cout()const {
		printf("n = %.*s, t = %d, v = %d\n", n.size(), n.c_str(), t, v);
	}
};

#ifdef __cplusplus
extern "C" {
#endif

	//for server 服务端

	dllemport int create4local();
	dllemport int create4pid();
	dllemport int create4tcp(unsigned short port);
	dllemport int create4udp(unsigned short port);
	dllemport int create4uds(const char* path);
	dllemport int create4shm(int key);
	dllemport int create4sig();
	dllemport int create4app();
	dllemport int create4npp(const char* pipe);
	dllemport int create4msg();

	dllemport void del4local();
	dllemport void del4pid();
	dllemport void del4tcp();
	dllemport void del4udp();
	dllemport void del4uds();
	dllemport void del4shm();
	dllemport void del4sig();
	dllemport void del4app();
	dllemport void del4npp();
	dllemport void del4msg();


	dllemport int add_val4local(const char* p, int c, int t, void* a);
	dllemport int add_val4pid(const char* p, int c, int t, void* a);
	dllemport int add_val4udp(const char* p, int c, int t, void* a);
	dllemport int add_val4tcp(const char* p, int c, int t, void* a);
	dllemport int add_val4shm(const char* p, int c, int t, void* a);
	dllemport int add_val4npp(const char* p, int c, int t, void* a);

	dllemport int del_val4local(const char* p, int c);
	dllemport int del_val4pid(const char* p, int c);
	dllemport int del_val4udp(const char* p, int c);
	dllemport int del_val4tcp(const char* p, int c);
	dllemport int del_val4shm(const char* p, int c);
	dllemport int del_val4npp(const char* p, int c);


	dllemport void printf_vals4local_server();
	dllemport void printf_vals4pid_server();
	dllemport void printf_vals4udp_server();
	dllemport void printf_vals4tcp_server();
	dllemport void printf_vals4shm_server();
	dllemport void printf_vals4npp_server();

	dllemport void printf_val4local_server(const char* k, size_t kz);
	dllemport void printf_val4pid_server(const char* k, size_t kz);
	dllemport void printf_val4udp_server(const char* k, size_t kz);
	dllemport void printf_val4tcp_server(const char* k, size_t kz);
	dllemport void printf_val4shm_server(const char* k, size_t kz);
	dllemport void printf_val4npp_server(const char* k, size_t kz);

	dllemport int size_of_type(int);

	//local for client
	dllemport kk::u64 read_val4local(const char* k, size_t c);
	dllemport bool set_val4local(const char* k, size_t kz, kk::u64 v);
	dllemport void printf_vals_client();
	dllemport void printf_val_client(const char* k, size_t kz);
	dllemport void set_local();
	dllemport void close_local();
	dllemport bool read_all4local();

#ifdef __cplusplus
}
#endif

dllemport int type_of(char);
dllemport int type_of(short);
dllemport int type_of(int);
dllemport int type_of(long);
dllemport int type_of(long long);
dllemport int type_of(unsigned char);
dllemport int type_of(unsigned short);
dllemport int type_of(unsigned int);
dllemport int type_of(unsigned long);
dllemport int type_of(unsigned long long);


dllemport int type_of(float);
dllemport int type_of(double);

template<class T>
T* createData() { T d; return &d; }
