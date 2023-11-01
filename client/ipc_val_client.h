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
#ifdef __cplusplus
}
#endif

//for client

dllemport kk::u64 read_val4pid(const char* k, size_t c);
dllemport kk::u64 read_val4tcp(const char* k, size_t c);
dllemport kk::u64 read_val4udp(const char* k, size_t c);
dllemport kk::u64 read_val4shm(const char* k, size_t c);
dllemport kk::u64 read_val4npp(const char* k, size_t c);

dllemport bool set_val4tcp(const char* k, size_t kz, kk::u64 v);
dllemport bool set_val4udp(const char* k, size_t kz, kk::u64 v);
dllemport bool set_val4pid(const char* k, size_t kz, kk::u64 v);
dllemport bool set_val4shm(const char* k, size_t kz, kk::u64 v);

dllemport bool set_val4npp(const char* k, size_t kz, kk::u64 v);

dllemport void printf_vals_client();
dllemport void printf_val_client(const char* k, size_t kz);


dllemport void set_udp(const char* ip, unsigned short port);
dllemport void set_tcp(const char* ip, unsigned short port);
dllemport void set_pid(int pid, kk::u64 data_addr);
dllemport void set_shm(int key);
dllemport void set_npp(const char* name);


dllemport void close_udp();
dllemport void close_tcp();
dllemport void close_pid();
dllemport void close_shm();
dllemport void close_npp();



dllemport bool read_all4pid();
dllemport bool read_all4udp();
dllemport bool read_all4tcp();
dllemport bool read_all4shm();
dllemport bool read_all4npp();

dllemport vector<val_info>& client_vals();