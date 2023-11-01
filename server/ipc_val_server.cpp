// =====================================================================================
//
//       Filename:  ipc_val.cpp
//
//    Description:  
//                     code edit with vim and and tw=2, default code is under utf-8.
//
//        Version:  $Id$
//        Created:  2023/1/9 20:29:31
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

#include "../ipc_com/types.h"
#include "../ipc_com/sock_basic.h"
#include "../ipc_com/ipc_proto.hpp"
#include "ipc_val_server.h"



#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef USE_STDSTL
#include <map>
#include <set>

template<typename x> using set = std::set<x>;
template<typename x, typename y> using map = std::map<x, y>;
#else
#include "../thy/adt/map.hpp"
#include "../thy/adt/set.hpp"

template<typename x> using set = kk::adt::set<x>;
template<typename x, typename y> using map = kk::adt::map<x, y>;
#endif

#ifdef _WIN32
#include <Windows.h>

#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <thread>
#include <iostream>

using namespace kk;

//空间大小
static const size_t shm_fz = 1024 * 1024 * 64;
static const size_t app_fz = 1024 * 1024 * 64;
static const size_t npp_fz = 1024 * 1024 * 64;
static const size_t pid_fz = 1024 * 1024 * 64;

static const size_t max_bfz = 1024 * 1024 * 64;	//kept >= xxx_fz

static char* buf4local = nullptr;
static char* buf4udp = nullptr;
static char* buf4tcp = nullptr;
//static char* buf4pid = nullptr;	//not need at all.
//static char* buf4shm = nullptr; //not need at all.
static char* buf4npp = nullptr;

static vector<val_info> vals4client;



// store data in server
struct vi {
	kk::u08 t;
	void* a;
};

//namepipe
static string                 npp_name;
#ifdef _WIN32
static HANDLE npp_h = INVALID_HANDLE_VALUE;
#else
static int                    npp_fd = -1;
#endif

//pid
static string                   data4pid;	//cmd and data as exchange data in memory.

//shared memory
char* shm_addr = NULL;						//shared memory.
#ifdef _WIN32
static HANDLE shm_h = INVALID_HANDLE_VALUE;
#else
static int           shm_id = 0;
#endif


//real store data in server.
static map<string, vi> ipc_vals;

static fd_set rds;
static fd_set wds;
static fd_set eds;
static int max_fds = 0;


static int cls_close(int sd) {

	if (errno == EAGAIN || errno == EWOULDBLOCK) { return sd; }
	FD_CLR(sd, &rds);
	FD_CLR(sd, &wds);
	FD_CLR(sd, &eds);
	close_sid(sd);

	return -1;

}



set<int> sds;
//: cope all accepted socket.
//: return value, 0 exit normal, 1 closed.
int cp4tcp(int sd) {
	int rsz = recv(sd, buf4tcp, sizeof(u32), 0);
	if (rsz == -1 || rsz == 0) { cls_close(sd); return 1; }
	u32 pc = *(u32*)buf4tcp;
	rsz = recv(sd, buf4tcp, pc, 0);
	if (rsz == -1 || rsz == 0) { cls_close(sd); return 1; }
	data_from_c d;
	d.uk(buf4tcp, pc);
	if (d.is_list()) {
		string r;
		data_from_s s; s.set_list();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			s.l.app(i->first.c_str(), i->first.size(), i->second.t);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		send(sd, r.c_str(), r.size(), 0);
		return 0;
	}
	else if (d.is_get()) {
		auto i = ipc_vals.find(d.g.k);
		if (i == ipc_vals.end()) { return 0; }
		string r;
		data_from_s s; s.set_get();
		memcpy(&s.g.v, (void*)i->second.a, size_of_type(i->second.t));
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		send(sd, r.c_str(), r.size(), 0);
		return 0;
	}
	else if (d.is_set()) {
		auto i = ipc_vals.find(d.s.k);
		if (i == ipc_vals.end()) { return 0; }
		memcpy((void*)i->second.a, &d.s.v, size_of_type(i->second.t));
		return 0;
	}
	else if (d.is_all()) {
		string r;
		data_from_s s; s.set_all();
		for (size_t i = 0; i < d.k.s.size(); ++i) {
			auto j = ipc_vals.find(d.k.s[i]);
			if (j == ipc_vals.end()) { return 0; }
			u64 v = 0;
			memcpy(&v, (void*)j->second.a, size_of_type(j->second.t));
			s.a.app(v);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		send(sd, r.c_str(), r.size(), 0);
		return 0;
	}
	else {}

	return 0;
}

int cp4udp(int sd) {
	struct sockaddr_in sa;
	memset(&sa, 0x0, sizeof(sa));
	int saz = sizeof(sockaddr);

	int rsz = recvfrom(sd, buf4udp, max_bfz, 0, (struct sockaddr*)&sa, (socklen_t*)&saz);
	u32 pc = *(u32*)buf4udp;
	data_from_c d;
	d.uk(buf4udp + sizeof(pc), pc);
	if (d.is_list()) {
		string r;
		data_from_s s; s.set_list();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			s.l.app(i->first.c_str(), i->first.size(), i->second.t);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		sendto(sd, r.c_str(), r.size(), 0, (struct sockaddr*)&sa, saz);
		return 0;
	}
	else if (d.is_get()) {
		auto i = ipc_vals.find(d.g.k);
		if (i == ipc_vals.end()) { return 0; }
		string r;
		data_from_s s; s.set_get();
		memcpy(&s.g.v, (void*)i->second.a, size_of_type(i->second.t));
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		sendto(sd, r.c_str(), r.size(), 0, (struct sockaddr*)&sa, saz);
		return 0;
	}
	else if (d.is_set()) {
		auto i = ipc_vals.find(d.s.k);
		if (i == ipc_vals.end()) { return 0; }
		memcpy((void*)i->second.a, &d.s.v, size_of_type(i->second.t));
		return 0;
	}
	else if (d.is_all()) {
		string r;
		data_from_s s; s.set_all();
		for (size_t i = 0; i < d.k.s.size(); ++i) {
			auto j = ipc_vals.find(d.k.s[i]);
			if (j == ipc_vals.end()) { return 0; }
			u64 v = 0;
			memcpy(&v, (void*)j->second.a, size_of_type(j->second.t));
			s.a.app(v);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
		sendto(sd, r.c_str(), r.size(), 0, (struct sockaddr*)&sa, saz);
		return 0;
	}
	else {}

	return 0;
}

int cp4npp() {
#ifdef _WIN32
	DWORD dwRead, dwWrite;
	memset(buf4npp, 0, npp_fz);
	if (!ReadFile(npp_h, buf4npp, npp_fz, &dwRead, NULL)) { return -1; }
#else
	memset(buf4npp, 0, npp_fz);
	if (read(npp_fd, buf4npp, npp_fz) == -1) { return -1; }
#endif

	u32 pc = *(u32*)buf4npp;
	data_from_c d;
	d.uk(buf4npp + sizeof(pc), pc);
	if (d.is_list()) {
		string r;
		data_from_s s; s.set_list();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			s.l.app(i->first.c_str(), i->first.size(), i->second.t);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
#ifdef _WIN32
		if (!WriteFile(npp_h, r.c_str(), r.size(), &dwWrite, NULL)) { return -1; }
#else
		if (write(npp_fd, r.c_str(), r.size()) == -1) { return -1; }
#endif
		return 0;
	}
	else if (d.is_get()) {
		auto i = ipc_vals.find(d.g.k);
		if (i == ipc_vals.end()) { return 0; }
		string r;
		data_from_s s; s.set_get();
		memcpy(&s.g.v, (void*)i->second.a, size_of_type(i->second.t));
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
#ifdef _WIN32
		if (!WriteFile(npp_h, r.c_str(), r.size(), &dwWrite, NULL)) { return -1; }
#else
		if (write(npp_fd, r.c_str(), r.size()) == -1) { return -1; }
#endif
		return 0;
	}
	else if (d.is_set()) {
		auto i = ipc_vals.find(d.s.k);
		if (i == ipc_vals.end()) { return 0; }
		memcpy((void*)i->second.a, &d.s.v, size_of_type(i->second.t));
		return 0;
	}
	else if (d.is_all()) {
		string r;
		data_from_s s; s.set_all();
		for (size_t i = 0; i < d.k.s.size(); ++i) {
			auto j = ipc_vals.find(d.k.s[i]);
			if (j == ipc_vals.end()) { return 0; }
			u64 v = 0;
			memcpy(&v, (void*)j->second.a, size_of_type(j->second.t));
			s.a.app(v);
		}
		r = s.pk();
		u32 c = r.size();
		r = as_str(c) + r;
#ifdef _WIN32
		if (!WriteFile(npp_h, r.c_str(), r.size(), &dwWrite, NULL)) { return -1; }
#else
		if (write(npp_fd, r.c_str(), r.size()) == -1) { return -1; }
#endif
		return 0;
	}
	else {}

	return 0;
}

int cp4local() {
	//not need at all, only keep struct same as other.
	return 0;
}

//: cope for pid.
//: return value, 0 exit normal, 1 closed.
int cp4pid() {
	if (data4pid[0] == 0) {				//init
		return 0;
	}
	else if (data4pid[0] == 1) {	//coped by client(then should be cope by server)
		data_from_c d; size_t k = 1;
		d.uk(data4pid.data() + k, data4pid.size() - k);
		if (d.is_list()) {
			data_from_s s; s.set_list();
			for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) { s.l.app(i->first.c_str(), i->first.size(), i->second.t); }
			data4pid = s.pk();
			u32 z = data4pid.size(); data4pid = as_str(z) + data4pid;
			kk::u08 c = 2;
			data4pid = as_str(c) + data4pid;
			return 0;
		}
		else if (d.is_get()) {
			auto i = ipc_vals.find(d.g.k);
			if (i == ipc_vals.end()) { data4pid[0] = 0; return 0; }
			string r;
			data_from_s s; s.set_get();
			memcpy(&s.g.v, (void*)i->second.a, size_of_type(i->second.t));
			data4pid = s.pk();
			u32 z = data4pid.size(); data4pid = as_str(z) + data4pid;
			kk::u08 c = 2;
			data4pid = as_str(c) + data4pid;
			return 0;
		}
		else if (d.is_set()) {
			data4pid[0] = 0;
			auto i = ipc_vals.find(d.s.k);
			if (i == ipc_vals.end()) { return 0; }
			memcpy((void*)i->second.a, &d.s.v, size_of_type(i->second.t));
			return 0;
		}
		else if (d.is_all()) {
			string r;
			data_from_s s; s.set_all();
			for (size_t i = 0; i < d.k.s.size(); ++i) {
				auto j = ipc_vals.find(d.k.s[i]);
				if (j == ipc_vals.end()) { return 0; }
				u64 v = 0;
				memcpy(&v, (void*)j->second.a, size_of_type(j->second.t));
				s.a.app(v);
			}
			data4pid = s.pk();
			u32 z = data4pid.size(); data4pid = as_str(z) + data4pid;
			kk::u08 c = 2;
			data4pid = as_str(c) + data4pid;
			return 0;
		}
		else {}
	}
	else {												//coped by server(then should be cope by client)
	}

	return 0;
}

//: cope for shared memory.
//: return value, 0 exit normal, 1 closed.
int cp4shm() {
	if (shm_addr[0] == 0) {				//init
		return 0;
	}
	else if (shm_addr[0] == 1) {	//coped by client(then should be cope by server)
		data_from_c d;
		d.uk(shm_addr + 1, shm_fz - 1);
		if (d.is_list()) {
			data_from_s s; s.set_list();
			for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) { s.l.app(i->first.c_str(), i->first.size(), i->second.t); }
			string r = s.pk();
			u32 z = r.size(); r = as_str(z) + r;
			kk::u08 c = 2;
			r = as_str(c) + r;
			memcpy(shm_addr, r.data(), r.size());
			return 0;
		}
		else if (d.is_get()) {
			auto i = ipc_vals.find(d.g.k);
			if (i == ipc_vals.end()) { shm_addr[0] = 0; return 0; }
			string r;
			data_from_s s; s.set_get();
			memcpy(&s.g.v, (void*)i->second.a, size_of_type(i->second.t));
			r = s.pk();
			u32 z = r.size(); r = as_str(z) + r;
			kk::u08 c = 2;
			r = as_str(c) + r;
			memcpy(shm_addr, r.data(), r.size());
			return 0;
		}
		else if (d.is_set()) {
			shm_addr[0] = 0;
			auto i = ipc_vals.find(d.s.k);
			if (i == ipc_vals.end()) { return 0; }
			memcpy((void*)i->second.a, &d.s.v, size_of_type(i->second.t));
			return 0;
		}
		else if (d.is_all()) {
			string r;
			data_from_s s; s.set_all();
			for (size_t i = 0; i < d.k.s.size(); ++i) {
				auto j = ipc_vals.find(d.k.s[i]);
				if (j == ipc_vals.end()) { return 0; }
				u64 v = 0;
				memcpy(&v, (void*)j->second.a, size_of_type(j->second.t));
				s.a.app(v);
			}
			r = s.pk();
			u32 z = r.size(); r = as_str(z) + r;
			kk::u08 c = 2;
			r = as_str(c) + r;
			memcpy(shm_addr, r.data(), r.size());
			return 0;
		}
		else {}
	}
	else {												//coped by server(then should be cope by client)
	}

	return 0;
}



static std::thread* td4local = nullptr;
static std::thread* td4udp = nullptr;
static std::thread* td4tcp = nullptr;
static std::thread* td4pid = nullptr;
static std::thread* td4shm = nullptr;
static std::thread* td4npp = nullptr;

static std::condition_variable cv4local;
static std::condition_variable cv4udp;
static std::condition_variable cv4tcp;
static std::condition_variable cv4pid;
static std::condition_variable cv4shm;
static std::condition_variable cv4npp;

static std::mutex mt4local;
static std::mutex mt4udp;
static std::mutex mt4tcp;
static std::mutex mt4pid;
static std::mutex mt4shm;
static std::mutex mt4npp;

const static int status_waiting = 0;
const static int status_working = 1;

static int           status4local = status_waiting;
static int           status4udp = status_waiting;
static int           status4tcp = status_waiting;
static int           status4pid = status_waiting;
static int           status4shm = status_waiting;
static int           status4npp = status_waiting;

#include <atomic>

static std::atomic_bool running4local;
static std::atomic_bool running4tcp;
static std::atomic_bool running4udp;
static std::atomic_bool running4pid;
static std::atomic_bool running4shm;
static std::atomic_bool running4npp;

int run_local_s() {
	buf4local = new char[max_bfz];
	while (running4local) {
		//note: this is no work at all for cope, here just for keept struct.
		std::this_thread::sleep_for(std::chrono::seconds(1000));

		/*
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unique_lock lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		cp4local();
		status4local = status_waiting; cv4local.notify_all();
		cv4local.notify_all();
		*/
	}
	delete[] buf4local; buf4local = nullptr;
	delete td4local; td4local = nullptr;
	return 0;
}

int run_pid_s() {
	while (running4pid) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unique_lock<std::mutex> lk(mt4pid);
		cv4pid.wait(lk, [] { return status4pid == status_waiting;  });
		status4pid = status_working; cv4pid.notify_all();
		cp4pid();
		status4pid = status_waiting; cv4pid.notify_all();
		cv4pid.notify_all();
	}

	delete td4pid;
	td4pid = nullptr;
	return 0;
}

int run_shm_s() {
	while (running4shm) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unique_lock<std::mutex> lk(mt4shm);
		cv4shm.wait(lk, [] { return status4shm == status_waiting;  });
		status4shm = status_working; cv4shm.notify_all();
		cp4shm();
		status4shm = status_waiting; cv4shm.notify_all();
	}

	delete td4shm;
	td4shm = nullptr;
	return 0;
}

int run_npp_s() {

#ifdef _WIN32
	if (ConnectNamedPipe(npp_h, NULL)) {}
#endif
	buf4npp = new char[max_bfz];
	while (running4npp) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unique_lock<std::mutex> lk(mt4npp);
		cv4npp.wait(lk, [] { return status4npp == status_waiting;  });
		status4npp = status_working; cv4npp.notify_all();
		cp4npp();
		status4npp = status_waiting; cv4npp.notify_all();
	}

	delete[] buf4npp; buf4npp = nullptr;
	delete td4npp; td4npp = nullptr;
	return 0;
}

int run_tcp_s(const uint16_t& port) {
	buf4tcp = new char[max_bfz];

#ifdef _WIN32
	WSADATA wsad;
	int srt = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (srt != 0) {
		printf("select error %s!\n", strerror(errno));
		return 1;
	}
#endif

	int sd;
	sd = (int)socket(AF_INET, SOCK_STREAM, 0);
	sds.insert(sd);
	//set_noblock_mode( sd );
	printf("setsockopt with the follow paramter.\n");
	init_sockopt(sd, true, max_bfz, max_bfz);
	//sockopt_info(sd);
	struct sockaddr_in sa, pa;
	memset(&sa, 0x0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa.sin_port = htons(port);

	if (bind(sd, (struct sockaddr*)&sa, sizeof(struct sockaddr)) == -1) {
		perror("bind error!\n");
		return -1;
	}

	if (listen(sd, 1000) == -1) {
		printf("listen error %s!\n", strerror(errno));
		return -1;
	}
	FD_ZERO(&rds); FD_ZERO(&wds); FD_ZERO(&eds);
	FD_SET(sd, &rds); FD_SET(sd, &wds); FD_SET(sd, &eds);
	fd_set trds; fd_set twds; fd_set teds;
	timeval tv;
	socklen_t asz;
	std::vector<int> deled;
	while (running4tcp) {
		trds = rds; twds = wds; teds = eds;
		//:here not use wds, because when the data sent the wds alway can be used.
		int rt = select(0, &trds, NULL, &teds, NULL);
		if (rt == -1) {
			printf("select error %s!\n", strerror(errno));
			continue;
		}
		deled.clear();
		for (auto i = sds.begin(); i != sds.end(); ++i) {
			if (FD_ISSET(*i, &trds)) {
				if (*i == sd) {	//: new connection.
					asz = sizeof(struct sockaddr);
					int asd = (int)accept(sd, (struct sockaddr*)&pa, &asz);
					//set_noblock_mode( asd ); //: not effect the cope ether block or nonblock.
					if (asd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
						//no new accept.
					}
					else {
#ifdef _WIN32
						DWORD rcvtimeo = 2000;
						if (setsockopt(asd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, sizeof(rcvtimeo)) < 0) {
							printf("setsockopt rcvtimeo with error info %s!\n", strerror(errno));
						}
#else
						int rcvtimeo = 2000;
						if (setsockopt(asd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, sizeof(rcvtimeo)) < 0) {
							printf("setsockopt rcvtimeo with error info %s!\n", strerror(errno));
						}
#endif
						printf("peer ip = \"%s\" with port = \"%d\" come in!\n", inet_ntoa(pa.sin_addr), ntohs(pa.sin_port));
						FD_SET(asd, &rds); FD_SET(asd, &wds); FD_SET(asd, &eds);
						sds.insert(asd);
					}
				}
				else {	//:recv data.
					std::unique_lock<std::mutex> lk(mt4tcp);
					cv4tcp.wait(lk, [] { return status4tcp == status_waiting;  });
					status4tcp = status_working; cv4tcp.notify_all();
					if (cp4tcp(*i) > 0) { deled.push_back(*i); }
					status4tcp = status_waiting; cv4tcp.notify_all();
				}
			}
			//if( FD_ISSET( *i, &twds ) ){ printf( "ready for write\n" ); }
			if (FD_ISSET(*i, &teds)) {
				printf("select detect error at %d with err %s\n.", *i, strerror(errno));
			}
		}
		for (auto i = deled.begin(); i != deled.end(); ++i) { sds.erase(sds.find(*i)); }
	}
	delete[] buf4tcp; buf4tcp = nullptr;
	delete td4tcp; td4tcp = nullptr;
	return 0;
}

int run_udp_s(const uint16_t& port) {
	buf4udp = new char[max_bfz];

#ifdef _WIN32
	WSADATA wsad;
	int srt = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (srt != 0) {
		printf("select error %s!\n", strerror(errno));
		return 1;
	}
#endif

	int sd;
	sd = (int)socket(AF_INET, SOCK_DGRAM, 0);
	sds.insert(sd);
	//set_noblock_mode( sd );
	printf("setsockopt with the follow paramter.\n");
	init_sockopt(sd, false, max_bfz, max_bfz);
	//sockopt_info(sd);
	struct sockaddr_in sa, pa;
	memset(&sa, 0x0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa.sin_port = htons(port);

	if (bind(sd, (struct sockaddr*)&sa, sizeof(struct sockaddr)) == -1) { perror("bind error!\n"); return -1; }

	FD_ZERO(&rds); FD_ZERO(&wds); FD_ZERO(&eds);
	FD_SET(sd, &rds); FD_SET(sd, &wds); FD_SET(sd, &eds);
	fd_set trds; fd_set twds; fd_set teds;
	timeval tv;
	socklen_t asz;
	std::vector<int> deled;
	while (running4udp) {
		trds = rds; twds = wds; teds = eds;
		//:here not use wds, because when the data sent the wds alway can be used.
		int rt = select(0, &trds, NULL, &teds, NULL);
		if (rt == -1) { printf("select error %s!\n", strerror(errno)); continue; }
		deled.clear();
		for (auto i = sds.begin(); i != sds.end(); ++i) {
			if (FD_ISSET(*i, &trds)) {
				std::unique_lock<std::mutex> lk(mt4udp);
				cv4udp.wait(lk, [] { return status4udp == status_waiting;  });
				status4udp = status_working; cv4udp.notify_all();
				if (cp4udp(*i) > 0) { deled.push_back(*i); }
				status4udp = status_waiting; cv4udp.notify_all();
			}
			//if( FD_ISSET( *i, &twds ) ){ printf( "ready for write\n" ); }
			if (FD_ISSET(*i, &teds)) {
				printf("select detect error at %d with err %s\n.", *i, strerror(errno));
			}
		}
		for (auto i = deled.begin(); i != deled.end(); ++i) { sds.erase(sds.find(*i)); }
	}

	delete[] buf4udp; buf4udp = nullptr;
	delete td4udp; td4udp = nullptr;
	return 0;
}


#ifdef __cplusplus
extern "C" {
#endif

	int create4local() {
		if (td4local == NULL) {
			running4local = true;
			td4local = new std::thread(&run_local_s);
			return 0;
		}
		else { return -1; }
	}

	int create4pid() {
		if (td4pid == NULL) {
			running4pid = true;
			td4pid = new std::thread(&run_pid_s);
			data4pid.resize(pid_fz);
#ifdef _WIN32
			DWORD pid = GetCurrentProcessId();
			printf("now pid = %lu, data addr is: %p\n", pid, data4pid.c_str());
#else
			pid_t pid = getpid();
			printf("now pid = %d, data addr is: %p\n", pid, data4pid.c_str());
#endif
			return 0;
		}
		else { return -1; }
	}
	int create4tcp(unsigned short port) {
		if (td4tcp == NULL) {
			running4tcp = true;
			td4tcp = new std::thread(&run_tcp_s, port);
			return 0;
		}
		else { return -1; }
	}
	int create4udp(unsigned short port) {
		if (td4udp == NULL) {
			running4udp = true;
			td4udp = new std::thread(&run_udp_s, port);
			return 0;
		}
		else { return -1; }
	}
	int create4uds(const char* path) { return -1; }
	int create4shm(int key) {
#ifdef _WIN32

#ifdef USE_STDSTL
		string s = std::to_string(key);
#else
		string s = string::to_string(key);
#endif

		HANDLE h = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shm_fz, s.c_str());
		if (h == INVALID_HANDLE_VALUE) {
			printf("%d handle create error!\n", key);
			return -1;
		}
		LPVOID b = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, shm_fz);
		memset(b, 0, shm_fz);
		shm_h = h;
		shm_addr = (char*)b;
#else
		int shmid = shmget(key, shm_fz, 0666 | IPC_CREAT);
		if (shmid != -1) {
			shm_addr = (char*)shmat(shmid, NULL, 0);
			shm_id = shmid;
		}
		else { return -1; }
#endif

		if (td4shm == NULL) {
			running4shm = true;
			td4shm = new std::thread(&run_shm_s);
			return 0;
		}
		else { return -1; }
		return 0;
	}
	int create4sig() { return -1; }
	int create4app() { return -1; }
	int create4npp(const char* name) {
#ifdef _WIN32
		npp_name = "\\\\.\\pipe\\";
		npp_name += name;
		HANDLE h = CreateNamedPipe(npp_name.c_str(), PIPE_ACCESS_DUPLEX
			, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, npp_fz, npp_fz, 0, NULL);
		if (h == INVALID_HANDLE_VALUE) {
			printf("%s handle create error!\n", npp_name.c_str());
			return -1;
		}
		npp_h = h;
#else
		npp_name = name;
		if (mkfifo(npp_name.c_str(), 0666) < 0) { return -1; }
		npp_fd = open(npp_name.c_str(), O_RDWR);
#endif
		if (td4npp == NULL) {
			running4npp = true;
			td4npp = new std::thread(&run_npp_s);
			return 0;
		}
		else { return -1; }
		return 0;
	}

	int create4msg() { return -1; }

	void del4local() { running4local = false; return; }
	void del4pid() { running4pid = false; return; }
	void del4tcp() { running4tcp = false; return; }
	void del4udp() { running4udp = false; return; }
	void del4uds() { return; }
	void del4shm() {
#ifdef _WIN32
		UnmapViewOfFile((LPVOID)shm_addr); shm_addr = NULL;
		CloseHandle(shm_h); shm_h = INVALID_HANDLE_VALUE;
#else
		shmdt(shm_addr);
		shmctl(shm_id, IPC_RMID, nullptr);
#endif
	}
	void del4sig() { return; }
	void del4app() { return; }
	void del4npp() {
#ifdef _WIN32
		CloseHandle(npp_h);
		npp_h = INVALID_HANDLE_VALUE;
#else
#endif
		running4npp = false; npp_name = ""; return;
	}
	void del4msg() { return; }

	int add_val4local(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4local = status_waiting; cv4local.notify_all();
		return 0;
	}
	int add_val4udp(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4udp);
		cv4udp.wait(lk, [] { return status4udp == status_waiting;  });
		status4udp = status_working; cv4udp.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4udp = status_waiting; cv4udp.notify_all();
		return 0;
	}
	int add_val4tcp(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4tcp);
		cv4tcp.wait(lk, [] { return status4tcp == status_waiting;  });
		status4tcp = status_working; cv4tcp.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4tcp = status_waiting; cv4tcp.notify_all();
		return 0;
	}
	int add_val4pid(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4pid);
		cv4pid.wait(lk, [] { return status4pid == status_waiting;  });
		status4pid = status_working; cv4pid.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4pid = status_waiting; cv4pid.notify_all();
		return 0;
	}

	int add_val4shm(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4shm);
		cv4shm.wait(lk, [] { return status4shm == status_waiting;  });
		status4shm = status_working; cv4shm.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4shm = status_waiting; cv4shm.notify_all();
		return 0;
	}

	int add_val4npp(const char* p, int c, int t, void* a) {
		std::unique_lock<std::mutex> lk(mt4npp);
		cv4npp.wait(lk, [] { return status4npp == status_waiting;  });
		status4npp = status_working; cv4npp.notify_all();
		vi v; v.t = t; v.a = a;
		ipc_vals[string(p, c)] = v;
		status4npp = status_waiting; cv4npp.notify_all();
		return 0;
	}

	int del_val4local(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4local = status_waiting; cv4local.notify_all();
		return 0;
	}

	int del_val4pid(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4pid);
		cv4pid.wait(lk, [] { return status4pid == status_waiting;  });
		status4pid = status_working; cv4pid.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4pid = status_waiting; cv4pid.notify_all();
		return 0;
	}

	int del_val4udp(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4udp);
		cv4udp.wait(lk, [] { return status4udp == status_waiting;  });
		status4udp = status_working; cv4udp.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4udp = status_waiting; cv4udp.notify_all();
		return 0;
	}

	int del_val4tcp(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4tcp);
		cv4tcp.wait(lk, [] { return status4tcp == status_waiting;  });
		status4tcp = status_working; cv4tcp.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4tcp = status_waiting; cv4tcp.notify_all();
		return 0;
	}

	int del_val4shm(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4shm);
		cv4shm.wait(lk, [] { return status4shm == status_waiting;  });
		status4shm = status_working; cv4shm.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4shm = status_waiting; cv4shm.notify_all();
		return 0;
	}

	int del_val4npp(const char* p, int c)
	{
		std::unique_lock<std::mutex> lk(mt4npp);
		cv4npp.wait(lk, [] { return status4npp == status_waiting;  });
		status4npp = status_working; cv4npp.notify_all();
		auto i = ipc_vals.find(string(p, c));
		ipc_vals.erase(i);
		status4npp = status_waiting; cv4npp.notify_all();
		return 0;
	}


	void printf_vals4local_server() {
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4local = status_waiting; cv4local.notify_all();
	}

	void printf_vals4pid_server() {
		std::unique_lock<std::mutex> lk(mt4pid);
		cv4pid.wait(lk, [] { return status4pid == status_waiting;  });
		status4pid = status_working; cv4pid.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4pid = status_waiting; cv4pid.notify_all();
	}

	void printf_vals4udp_server() {
		std::unique_lock<std::mutex> lk(mt4udp);
		cv4udp.wait(lk, [] { return status4udp == status_waiting;  });
		status4udp = status_working; cv4udp.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4udp = status_waiting; cv4udp.notify_all();
	}

	void printf_vals4tcp_server() {
		std::unique_lock<std::mutex> lk(mt4tcp);
		cv4tcp.wait(lk, [] { return status4tcp == status_waiting;  });
		status4tcp = status_working; cv4tcp.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4tcp = status_waiting; cv4tcp.notify_all();
	}

	void printf_vals4shm_server() {
		std::unique_lock<std::mutex> lk(mt4shm);
		cv4shm.wait(lk, [] { return status4shm == status_waiting;  });
		status4shm = status_working; cv4shm.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4shm = status_waiting; cv4shm.notify_all();
	}
	void printf_vals4npp_server() {
		std::unique_lock<std::mutex> lk(mt4npp);
		cv4npp.wait(lk, [] { return status4npp == status_waiting;  });
		status4npp = status_working; cv4npp.notify_all();
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i) {
			printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		}
		status4npp = status_waiting; cv4npp.notify_all();
	}

	void printf_val4local_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4local = status_waiting; cv4local.notify_all();
	}

	void printf_val4pid_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4pid);
		cv4pid.wait(lk, [] { return status4pid == status_waiting;  });
		status4pid = status_working; cv4pid.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4pid = status_waiting; cv4pid.notify_all();
	}

	void printf_val4udp_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4udp);
		cv4udp.wait(lk, [] { return status4udp == status_waiting;  });
		status4udp = status_working; cv4udp.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4udp = status_waiting; cv4udp.notify_all();
	}

	void printf_val4tcp_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4tcp);
		cv4tcp.wait(lk, [] { return status4tcp == status_waiting;  });
		status4tcp = status_working; cv4tcp.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4tcp = status_waiting; cv4tcp.notify_all();
	}

	void printf_val4shm_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4shm);
		cv4shm.wait(lk, [] { return status4shm == status_waiting;  });
		status4shm = status_working; cv4shm.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4shm = status_waiting; cv4shm.notify_all();
	}

	void printf_val4npp_server(const char* k, size_t kz)
	{
		std::unique_lock<std::mutex> lk(mt4npp);
		cv4npp.wait(lk, [] { return status4npp == status_waiting;  });
		status4npp = status_working; cv4npp.notify_all();
		auto i = ipc_vals.find(string(k, kz));
		if (i == ipc_vals.end()) return;
		printf("'%.*s'->0x%p\n", (int)i->first.size(), i->first.c_str(), i->second);
		status4npp = status_waiting; cv4npp.notify_all();
	}

	static bool read_list4local() {
		using namespace kk;
		vals4client.clear();
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		vals4client.resize(ipc_vals.size()); size_t j = 0;
		for (auto i = ipc_vals.begin(); i != ipc_vals.end(); ++i, ++j) {
			vals4client[j].n = i->first;
			vals4client[j].t = i->second.t;
			//vals4client.back().cout();
		}
		status4local = status_waiting; cv4local.notify_all();
		return true;
	}

	u64 read_val4local(const char* k, size_t c) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();

		auto i = ipc_vals.find(string(k, c));
		if (i != ipc_vals.end()) {
			u64 v = 0;
			memcpy(&v, (void*)i->second.a, size_of_type(i->second.t));
			status4local = status_waiting; cv4local.notify_all();
			cv4local.notify_all();
			return v;
		}

		status4local = status_waiting; cv4local.notify_all();
		cv4local.notify_all();

		return 0;
	}

	bool set_val4local(const char* k, size_t kz, kk::u64 v) {
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();

		auto i = ipc_vals.find(string(k, kz));
		if (i != ipc_vals.end()) {
			memcpy((void*)i->second.a, &v, size_of_type(i->second.t));
			status4local = status_waiting; cv4local.notify_all();
			cv4local.notify_all();
		}
		else return false;

		status4local = status_waiting; cv4local.notify_all();
		cv4local.notify_all();
		return true;
	}

	void set_local() { return; }

	void close_local() { return; }

	bool read_all4local() {
		vals4client.clear();
		if (vals4client.empty()) { read_list4local(); }
		std::unique_lock<std::mutex> lk(mt4local);
		cv4local.wait(lk, [] { return status4local == status_waiting;  });
		status4local = status_working; cv4local.notify_all();
		for (size_t i = 0; i < vals4client.size(); ++i) {
			auto j = ipc_vals.find(vals4client[i].n);
			if (j != ipc_vals.end()) {
				memcpy(&vals4client[i].v, (void*)j->second.a, size_of_type(j->second.t));
			}
		}
		status4local = status_waiting; cv4local.notify_all();
		return true;
	}

	void printf_vals_client()
	{
		for (auto& i : vals4client) {
			i.cout();
		}
	}

	void printf_val_client(const char* k, size_t kz)
	{
		for (auto& i : vals4client) {
			if (i.n.c_str() == k && i.n.size() == kz) {
				i.cout();
			}
		}
	}

	int size_of_type(int t) {
		if (t == 0 || t == 5) { return sizeof(char); }
		if (t == 1 || t == 6) { return sizeof(short); }
		if (t == 2 || t == 7) { return sizeof(int); }
		if (t == 3 || t == 8) { return sizeof(long); }
		if (t == 4 || t == 9) { return sizeof(long long); }
		if (t == 18) { return sizeof(float); }
		if (t == 19) { return sizeof(double); }

		return 0;
	}



#ifdef __cplusplus
}
#endif

int type_of(char) { return 0; }
int type_of(short) { return 1; }
int type_of(int) { return 2; }
int type_of(long) { return 3; }
int type_of(long long) { return 4; }
int type_of(unsigned char) { return 5; }
int type_of(unsigned short) { return 6; }
int type_of(unsigned int) { return 7; }
int type_of(unsigned long) { return 8; }
int type_of(unsigned long long) { return 9; }

int type_of(float) { return 18; }
int type_of(double) { return 19; }

