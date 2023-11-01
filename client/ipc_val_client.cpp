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


#include "../ipc_com/sock_basic.h"
#include "../ipc_com/types.h"
#include "ipc_val_client.h"


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

#include "../ipc_com/ipc_proto.hpp"



using namespace kk;

static vector<val_info> vals4client;



static int pid4pid = -1;

static int sid4tcp = -1;
static string tcp_ip = "127.0.0.1";
static kk::u16 tcp_port = 31921;

static int sid4udp = -1;
static string udp_ip = "127.0.0.1";
static kk::u16 udp_port = 31921;



#ifdef _WIN32
static HANDLE ph4pid = INVALID_HANDLE_VALUE;
static kk::u64 da4pid = 0;
#else
static kk::u64 da4pid = 0;
#endif

static int key4shm = 0;
static char* ba4shm = NULL;
#ifdef _WIN32
static HANDLE ph4shm = INVALID_HANDLE_VALUE;
#else
static int id4shm = -1;
#endif


static string na4npp;
static char* buf4npp = NULL;
#ifdef _WIN32
static HANDLE ph4npp = INVALID_HANDLE_VALUE;

#else
static int id4npp = -1;
#endif


static const size_t shm_fz = 1024 * 1024 * 64;
static const size_t app_fz = 1024 * 1024 * 64;
static const size_t npp_fz = 1024 * 1024 * 64;
static const size_t pid_fz = 1024 * 1024 * 64;

static const size_t max_bfz = 1024 * 1024 * 64;	//kept >= xxx_fz

static char* buf4local = new char[max_bfz];
static char* buf4udp = new char[max_bfz];
static char* buf4tcp = new char[max_bfz];
//static char* buf4pid = new char[max_bfz];	//not need at all.
//static char* buf4shm = new char[max_bfz]; //not need at all.
//static char* buf4npp = new char[max_bfz]; //not need at all.





#ifdef __cplusplus
extern "C" {
#endif

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


static bool read_list4tcp() {
	using namespace kk;
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4tcp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return false;
	}
	if (recv(sid4tcp, (char*)&c, sizeof(c), 0) != sizeof(c)) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return false;
	}
	if (recv(sid4tcp, buf4tcp, c, 0) != (int)c) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return false;
	}
	data_from_s z;
	z.uk(buf4tcp, c);
	c = z.l.t.size();
	vals4client.clear(); vals4client.resize(c);
	for (size_t i = 0; i < c; ++i) {
		vals4client[i].n = z.l.n[i];
		vals4client[i].t = z.l.t[i];

	}
	return true;
}

static bool read_list4udp() {
	using namespace kk;
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4udp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return false;
	}
	if (recv(sid4udp, buf4udp, max_bfz, 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return false;
	}
	data_from_s z;
	z.uk(buf4udp + sizeof(u32), *(u32*)buf4udp);
	c = z.l.t.size();
	vals4client.clear(); vals4client.resize(c);
	for (size_t i = 0; i < c; ++i) {
		vals4client[i].n = z.l.n[i];
		vals4client[i].t = z.l.t[i];

	}
	return true;
}

static bool read_list4pid() {

#ifdef _WIN32
	if (ph4pid == INVALID_HANDLE_VALUE || ph4pid == NULL || da4pid == 0) { return false; }
	using namespace kk;
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.c_str(), s.size(), 0);
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		size_t r;
		ReadProcessMemory(ph4pid, (LPVOID)da4pid, (LPVOID)s.data(), 5, &r);
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			ReadProcessMemory(ph4pid, (LPVOID)(da4pid + 5), (LPVOID)s.data(), c, &r);
			data_from_s z;
			z.uk(s.data(), c);
			vals4client.clear(); vals4client.resize(z.l.t.size());
			for (size_t i = 0; i < z.l.t.size(); ++i) {
				vals4client[i].n = z.l.n[i];
				vals4client[i].t = z.l.t[i];
				//vals4client.back().cout();
			}
			s[0] = 0;
			WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.c_str(), 1, 0);
			return true;
			break;
		}
		else if (s[0] == 1) {
			break;
		}
		else { break; }
	}

	return false;

#else
	if (pid4pid == -1 || da4pid == 0) { return false; }
	using namespace kk;
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	// pid question ?
	string memPath = "/proc/" + std::to_string(pid4pid) + "/mem";
	int memfd = open(memPath.c_str(), O_RDWR);
	if (memfd == -1) { return false; }
	if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
		close(memfd);
		return false;
	}
	if (write(memfd, s.c_str(), s.size()) == -1) {
		close(memfd);
		return false;
	}
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
			close(memfd);
			return false;
		}
		if (read(memfd, (void*)s.data(), 5) == -1) {
			close(memfd);
			return false;
		}
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			if (lseek(memfd, (off_t)(da4pid + 5), SEEK_SET) == -1) {
				close(memfd);
				return false;
			}
			if (read(memfd, (void*)s.data(), c) == -1) {
				close(memfd);
				return false;
			}
			data_from_s z;
			z.uk(s.data(), c);
			vals4client.clear(); vals4client.resize(z.l.t.size());
			for (size_t i = 0; i < z.l.t.size(); ++i) {
				vals4client[i].n = z.l.n[i];
				vals4client[i].t = z.l.t[i];
				//vals4client.back().cout();
			}
			s[0] = 0;
			if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
				close(memfd);
				return false;
			}
			if (write(memfd, s.c_str(), 1) == -1) {
				close(memfd);
				return false;
			}
			return true;
			break;
		}
		else if (s[0] == 1) {
			break;
		}
		else { break; }
	}
	close(memfd);
	return false;
#endif
}

static bool read_list4shm() {
	using namespace kk;
	//as reconnect.
	if (key4shm != -1) { set_shm(key4shm); }
#ifdef _WIN32
#else
	if (id4shm != -1) { return false; }
#endif
	if (ba4shm == NULL) { return false; }
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	memcpy(ba4shm, s.c_str(), s.size());
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (ba4shm[0] == 2) {
			u32 c = 0; as_val(ba4shm + 1, c);
			data_from_s z;
			z.uk(ba4shm + 5, c);
			vals4client.clear(); vals4client.resize(z.l.t.size());
			for (size_t i = 0; i < z.l.t.size(); ++i) {
				vals4client[i].n = z.l.n[i];
				vals4client[i].t = z.l.t[i];
				//vals4client.back().cout();
			}
			ba4shm[0] = 0;
			return true;
			break;
		}
		else if (ba4shm[0] == 1) {
			break;
		}
		else { break; }
	}

	return false;


}

static bool read_list4npp() {
	using namespace kk;
	string s;
	data_from_c d; d.set_list();
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
#ifdef _WIN32
	DWORD dwRead, dwWrite;
	if (!WriteFile(ph4npp, s.c_str(), s.size(), &dwWrite, NULL)) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (!ReadFile(ph4npp, buf4npp, npp_fz, &dwRead, NULL)) {
		delete[] buf4npp;
		return false;
	}
#else
	if (write(id4npp, s.c_str(), s.size()) == -1) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (read(id4npp, buf4npp, npp_fz) == -1) {
		delete[] buf4npp;
		return false;
	}
#endif
	data_from_s z;
	z.uk(buf4npp + sizeof(u32), *(u32*)buf4npp);
	c = z.l.t.size();
	vals4client.clear(); vals4client.resize(c);
	for (size_t i = 0; i < c; ++i) {
		vals4client[i].n = z.l.n[i];
		vals4client[i].t = z.l.t[i];
		//vals4client[i].cout();
	}
	return true;
}



//note, this different with differnt process, because of it can access all memory, so control cv directly for better performance.

u64 read_val4tcp(const char* k, size_t kz) {
	if (vals4client.empty()) { read_list4tcp(); }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.g.k = string(k, kz);
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4tcp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}
	if (recv(sid4tcp, (char*)&c, sizeof(c), 0) != sizeof(c)) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}
	if (recv(sid4tcp, buf4tcp, c, 0) != (int)c) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}
	data_from_s z; z.uk(buf4tcp, c);
	//printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
	for (auto& x : vals4client)
		if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
	return z.g.v;
}

u64 read_val4udp(const char* k, size_t kz) {
	if (vals4client.empty()) { read_list4udp(); }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.g.k = string(k, kz);
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4udp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return 0;
	}
	if (recv(sid4udp, buf4udp, max_bfz, 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return 0;
	}
	data_from_s z; z.uk(buf4udp + sizeof(u32), *(u32*)buf4udp);
	//printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
	for (auto& x : vals4client)
		if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
	return z.g.v;
}



u64 read_val4pid(const char* k, size_t kz) {
#ifdef _WIN32
	if (vals4client.empty()) { if (!read_list4pid()) { return false; } }
	if (ph4pid == INVALID_HANDLE_VALUE || ph4pid == NULL || da4pid == 0) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.g.k = string(k, kz);
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.c_str(), s.size(), 0);
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		size_t r;
		ReadProcessMemory(ph4pid, (LPVOID)da4pid, (LPVOID)s.data(), 5, &r);
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			ReadProcessMemory(ph4pid, (LPVOID)(da4pid + 5), (LPVOID)s.data(), c, &r);
			data_from_s z; z.uk(s.data(), c);
			//printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
			for (auto& x : vals4client)
				if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
			return z.g.v;
			break;
		}
		else if (s[0] == 1) {
		}
		else { break; }
	}

	return 0;

#else
	if (vals4client.empty()) { if (!read_list4pid()) { return false; } }
	if (pid4pid == -1 || da4pid == 0) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.g.k = string(k, kz);
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	// pid question ?
	string memPath = "/proc/" + std::to_string(pid4pid) + "/mem";
	int memfd = open(memPath.c_str(), O_RDWR);
	if (memfd == -1) { return false; }
	if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
		close(memfd);
		return false;
	}
	if (write(memfd, s.c_str(), s.size()) == -1) {
		close(memfd);
		return false;
	}
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
			close(memfd);
			return false;
		}
		if (read(memfd, (void*)s.data(), 5) == -1) {
			close(memfd);
			return false;
		}
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			if (lseek(memfd, (off_t)(da4pid + 5), SEEK_SET) == -1) {
				close(memfd);
				return false;
			}
			if (read(memfd, (void*)s.data(), c) == -1) {
				close(memfd);
				return false;
			}
			data_from_s z; z.uk(s.data(), c);
			//printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
			for (auto& x : vals4client)
				if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
			return z.g.v;
			break;
		}
		else if (s[0] == 1) {
		}
		else { break; }
	}
	close(memfd);
	return 0;
#endif
}

u64 read_val4shm(const char* k, size_t kz) {
	if (vals4client.empty()) { read_list4shm(); }
	if (ba4shm == NULL) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_get();
	d.g.k = string(k, kz);
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	memcpy(ba4shm, s.c_str(), s.size());
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (ba4shm[0] == 2) {
			u32 c = 0; as_val(ba4shm + 1, c);
			data_from_s z; z.uk(ba4shm + 5, c);
			printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
			ba4shm[0] == 0;
			for (auto& x : vals4client)
				if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
			return z.g.v;
			break;
		}
		else if (ba4shm[0] == 1) {
		}
		else { break; }
	}

	return 0;
}

u64 read_val4npp(const char* k, size_t kz) {

	if (vals4client.empty()) { read_list4npp(); }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.g.k = string(k, kz);
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;

#ifdef _WIN32
	DWORD dwRead, dwWrite;
	if (!WriteFile(ph4npp, s.c_str(), s.size(), &dwWrite, NULL)) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (!ReadFile(ph4npp, buf4npp, npp_fz, &dwRead, NULL)) {
		delete[]buf4npp;
		return false;
	}
#else
	if (write(id4npp, s.c_str(), s.size()) == -1) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (read(id4npp, buf4npp, npp_fz) == -1) {
		delete[] buf4npp;
		return false;
	}
#endif
	data_from_s z; z.uk(buf4npp + sizeof(u32), *(u32*)buf4npp);
	for (auto& x : vals4client)
		if (strcmp(x.n.c_str(), k) == 0 && x.n.size() == kz) x.v = z.g.v;
	//printf("value is: %d(%.16f)\n", z.g.v, *(double*)&z.g.v);
	return z.g.v;


}

bool set_val4tcp(const char* k, size_t kz, kk::u64 v) {
	if (vals4client.empty()) { read_list4tcp(); }
	using namespace kk;
	string s; data_from_c d;
	d.set_set(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4tcp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return false;
	}
	return true;
}

bool set_val4udp(const char* k, size_t kz, kk::u64 v) {
	if (vals4client.empty()) { read_list4udp(); }
	using namespace kk;
	string s; data_from_c d;
	d.set_get(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
	if (send(sid4udp, s.c_str(), s.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return false;
	}
	return true;
}

bool set_val4pid(const char* k, size_t kz, kk::u64 v) {
#ifdef _WIN32
	if (vals4client.empty()) { if (!read_list4pid()) { return false; } }
	if (ph4pid == INVALID_HANDLE_VALUE || ph4pid == NULL || da4pid == 0) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_set(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.c_str(), s.size(), 0);
	return true;
#else
	if (vals4client.empty()) { if (!read_list4pid()) { return false; } }
	if (da4pid == 0) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_set(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	string memPath = "/proc/" + std::to_string(pid4pid) + "/mem";
	int memfd = open(memPath.c_str(), O_RDWR);
	if (memfd == -1) { return false; }
	if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
		close(memfd);
		return false;
	}
	if (write(memfd, s.c_str(), s.size()) == -1) {
		close(memfd);
		return false;
	}
	close(memfd);
	return true;
#endif
}

bool set_val4shm(const char* k, size_t kz, kk::u64 v) {
	if (vals4client.empty()) { read_list4shm(); }
	if (ba4shm == NULL) { return false; }
	using namespace kk;
	string s; data_from_c d;
	d.set_set(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u08 c = 1;
	s = as_str(c) + s;
	memcpy(ba4shm, s.c_str(), s.size());
	return true;
}



bool set_val4npp(const char* k, size_t kz, kk::u64 v) {
	using namespace kk;
	string s; data_from_c d;
	d.set_set(); d.s.k = string(k, kz); d.s.v = v;
	s = d.pk(); u32 c = s.size();
	s = as_str(c) + s;
#ifdef _WIN32
	DWORD dwRead, dwWrite;
	if (!WriteFile(ph4npp, s.c_str(), s.size(), &dwWrite, NULL)) { return false; }
#else
	if (write(id4npp, s.c_str(), s.size()) == -1) { return false; }
#endif
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

static bool reconnect4tcp() {
	if (sid4tcp == -1) {
		sid4tcp = connect_to_timeout(tcp_ip.c_str(), tcp_port, 1, true);
		if (sid4tcp == -1) { printf("read from tcp with error!\n"); return false; }
#ifdef _WIN32
		DWORD rcvtimeo = 1000;
#else
		unsigned long rcvtimeo = 1000;
#endif
		if (setsockopt(sid4tcp, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, sizeof(rcvtimeo)) < 0) {
			printf("setsockopt rcvtimeo with error info %s!\n", strerror(errno));
			return false;
		}
	}

	return true;
}

static bool reconnect4udp() {
	if (sid4udp == -1) {
		sid4udp = connect_to_timeout(udp_ip.c_str(), udp_port, 1, false);
		if (sid4udp == -1) { printf("read from udp with error!\n"); return false; }
#ifdef _WIN32
		DWORD rcvtimeo = 1000;
#else
		unsigned long rcvtimeo = 1000;
#endif
		if (setsockopt(sid4udp, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvtimeo, sizeof(rcvtimeo)) < 0) {
			printf("setsockopt rcvtimeo with error info %s!\n", strerror(errno));
			return false;
		}
	}
	return true;
}


void set_udp(const char* ip, unsigned short port) {
	close_sid(sid4udp); sid4udp = -1;
	udp_ip = ip; udp_port = port;
	reconnect4udp();
}
void set_tcp(const char* ip, unsigned short port) {
	close_sid(sid4tcp); sid4tcp = -1;
	tcp_ip = ip; tcp_port = port;
	reconnect4tcp();
}
void set_pid(int pid, u64 data_addr) {
	pid4pid = pid;
#ifdef _WIN32
	ph4pid = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid4pid);
	da4pid = data_addr;
#else

	da4pid = data_addr;
#endif
}

void set_shm(int key) {
	key4shm = key;
#ifdef _WIN32

#ifdef USE_STDSTL
	string s = std::to_string(key);
#else
	string s = string::to_string(key);
#endif

	HANDLE h = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, s.c_str());
	LPVOID b = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, shm_fz);
	ph4shm = h;
	ba4shm = (char*)b;
#else
	int shmid = shmget(key, shm_fz, 0666 | IPC_CREAT);
	if (shmid != -1) {
		ba4shm = (char*)shmat(shmid, NULL, 0);
	}
	id4shm = shmid;
#endif
}

void set_npp(const char* name) {
#ifdef _WIN32
	na4npp = "\\\\.\\pipe\\";
	na4npp += name;
	if (!WaitNamedPipe(na4npp.c_str(), NMPWAIT_WAIT_FOREVER))
	{
		printf("named pipe not exists!\n");
		return;
	}
	HANDLE h = CreateFile(na4npp.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == na4npp.c_str())
	{
		printf("open pipe failed!\n");
		return;
	}
	ph4npp = h;
	buf4npp = new char[npp_fz + 1];

#else
	na4npp = name;


	buf4npp = new char[npp_fz + 1];
#endif
}




void close_pid() {
#ifdef _WIN32
	CloseHandle(ph4pid); ph4pid = INVALID_HANDLE_VALUE;
#else

#endif
}

void close_shm() {
#ifdef _WIN32
	UnmapViewOfFile((LPVOID)ba4shm); ba4shm = NULL;
	CloseHandle(ph4shm); ph4shm = INVALID_HANDLE_VALUE;
#else
	shmdt(ba4shm);
	//shmctl(id4shm, IPC_RMID, nullptr);
#endif
}

void close_udp() {
	close_sid(sid4udp); sid4udp = -1;
}

void close_tcp() {
	close_sid(sid4tcp); sid4tcp = -1;
}

void close_npp() {
#ifdef _WIN32
	CloseHandle(ph4npp); ph4npp = INVALID_HANDLE_VALUE;
	delete[]buf4npp;
#else
#endif
}



bool read_all4pid() {
	vals4client.clear();
	if (vals4client.empty()) { read_list4pid(); }
	//as reconnect
	if (pid4pid != -1) { set_pid(pid4pid, da4pid); }
	if (pid4pid == -1) { return false; }
#ifdef _WIN32
	if (ph4pid == INVALID_HANDLE_VALUE || ph4pid == NULL) { return false; }
	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string s; s = d.pk();
	u08 c = 1;
	s = as_str(c) + s;
	WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.c_str(), s.size(), 0);
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		size_t r;
		ReadProcessMemory(ph4pid, (LPVOID)da4pid, (LPVOID)s.data(), 5, &r);
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			ReadProcessMemory(ph4pid, (LPVOID)(da4pid + 5), (LPVOID)s.data(), c, &r);
			data_from_s z;
			z.uk(s.data(), c);
			for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = z.a.v[i]; }
			s[0] = 0;
			WriteProcessMemory(ph4pid, (LPVOID)da4pid, s.data(), 1, 0);
			return true;
			break;
		}
		else if (s[0] == 1) {
			break;
		}
		else { break; }
	}

	return false;

#else
	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string s; s = d.pk();
	u08 c = 1;
	s = as_str(c) + s;
	string memPath = "/proc/" + std::to_string(pid4pid) + "/mem";
	int memfd = open(memPath.c_str(), O_RDWR);
	if (memfd == -1) { return false; }
	if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
		close(memfd);
		return false;
	}
	if (write(memfd, s.c_str(), s.size()) == -1) {
		close(memfd);
		return false;
	}
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
			close(memfd);
			return false;
		}
		if (read(memfd, (void*)s.data(), 5) == -1) {
			close(memfd);
			return false;
		}
		if (s[0] == 2) {
			u32 c = 0; as_val(s.data() + 1, c); s.resize(c);
			if (lseek(memfd, (off_t)(da4pid + 5), SEEK_SET) == -1) {
				close(memfd);
				return false;
			}
			if (read(memfd, (void*)s.data(), c) == -1) {
				close(memfd);
				return false;
			}
			data_from_s z;
			z.uk(s.data(), c);
			for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = z.a.v[i]; }
			s[0] = 0;
			if (lseek(memfd, (off_t)da4pid, SEEK_SET) == -1) {
				close(memfd);
				return false;
			}
			if (write(memfd, s.c_str(), 1) == -1) {
				close(memfd);
				return false;
			}
			break;
		}
		else if (s[0] == 1) {
			break;
		}
		else { break; }
	}

	close(memfd);
	return false;
#endif
}

bool read_all4shm() {
	vals4client.clear();
	if (vals4client.empty()) { read_list4shm(); }
#ifdef _WIN32
	if (ph4shm == INVALID_HANDLE_VALUE || ph4shm == NULL || ba4shm == NULL) { return false; }
#else
	if (id4shm == -1) { return false; }
#endif
	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string s; s = d.pk();
	u08 c = 1;
	s = as_str(c) + s;
	memcpy(ba4shm, s.c_str(), s.size());
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (ba4shm[0] == 2) {
			u32 c = 0; as_val(ba4shm + 1, c);
			data_from_s z;
			z.uk(ba4shm + 5, c);
			for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = z.a.v[i]; }
			ba4shm[0] = 0;
			return true;
			break;
		}
		else if (ba4shm[0] == 1) {
			break;
		}
		else { break; }
	}

	return false;

}

bool read_all4npp() {
	vals4client.clear();
#ifdef _WIN32
	if (ph4npp == INVALID_HANDLE_VALUE || ph4npp == NULL) { return false; }
#else
	if (id4npp == -1) { return false; }
#endif
	if (vals4client.empty()) { read_list4npp(); }

	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string r; r = d.pk();
	u32 k = r.size();
	r = as_str(k) + r;
#ifdef _WIN32
	DWORD dwRead, dwWrite;
	if (!WriteFile(ph4npp, r.c_str(), r.size(), &dwWrite, NULL)) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (!ReadFile(ph4npp, buf4npp, npp_fz, &dwRead, NULL)) {
		delete[] buf4npp;
		return false;
	}
#else
	if (write(id4npp, r.c_str(), r.size()) == -1) { return false; }
	memset(buf4npp, 0, npp_fz);
	if (read(id4npp, buf4npp, npp_fz) == -1) {
		delete[] buf4npp;
		return false;
	}
#endif
	data_from_s s;
	s.uk(buf4npp + sizeof(u32), *(u32*)buf4npp);
	for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = s.a.v[i]; }
	return true;
}

bool read_all4udp() {
	vals4client.clear();
	if (sid4udp == -1) { reconnect4udp(); }
	if (sid4udp == -1) { return false; }
	if (vals4client.empty()) { read_list4udp(); }

	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string r; r = d.pk();
	u32 k = r.size();
	r = as_str(k) + r;
	if (send(sid4udp, r.c_str(), r.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return 0;
	}

	data_from_s s;
	if (recv(sid4udp, buf4udp, max_bfz, 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4udp); sid4udp = -1; }
		return 0;
	}
	s.uk(buf4udp + sizeof(u32), *(u32*)buf4udp);
	for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = s.a.v[i]; }
	return true;

}

bool read_all4tcp() {
	vals4client.clear();
	if (sid4tcp == -1) { reconnect4tcp(); }
	if (sid4tcp == -1) { return false; }
	if (vals4client.empty()) { read_list4tcp(); }
	data_from_c d; d.set_all();
	for (size_t i = 0; i < vals4client.size(); ++i) {
		d.k.app(vals4client[i].n.c_str(), vals4client[i].n.size());
	}
	string r; r = d.pk();
	u32 k = r.size();
	r = as_str(k) + r;
	if (send(sid4tcp, r.c_str(), r.size(), 0) <= 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}

	if (recv(sid4tcp, (char*)&k, sizeof(k), 0) != sizeof(k)) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}

	data_from_s s;
	if (recv(sid4tcp, buf4tcp, k, 0) != (int)k) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { close_sid(sid4tcp); sid4tcp = -1; }
		return 0;
	}
	s.uk(buf4tcp, k);
	for (size_t i = 0; i < vals4client.size(); ++i) { vals4client[i].v = s.a.v[i]; }
	return true;

}

vector<val_info>& client_vals() { return vals4client; }