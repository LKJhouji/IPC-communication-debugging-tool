// =====================================================================================
//
//       Filename:  ipc_proto.hpp
//
//    Description:  
//                     code edit with vim and and tw=2, default code is under utf-8.
//
//        Version:  $Id$
//        Created:  2023/8/13 11:55:57
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

#include "types.h"

#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#define USE_STDSTL

#ifdef USE_STDSTL
#include <string>
#include <vector>
template<typename x> using vector = std::vector<x>;
using string = std::string;
#else
#include "../thy/adt/string.hpp"
#include "../thy/adt/vector.hpp"

template<typename x> using vector = kk::adt::vector<x>;
using string = kk::adt::string::string;
#endif


using namespace kk;



template<typename x> static string as_str(const x& v) { return string((const char*)&v, sizeof(x)); }
template<typename x> static string as_str(const x* v, size_t c) { return string((const char*)v, sizeof(x) * c); }
template<> string as_str(const string& v) {
	u32 z = v.size();
	return string((const char*)&z, sizeof(z)) + v;
}
template<> string as_str(const string* v, size_t c) {
	string r;
	for (size_t i = 0; i < c; ++i) {
		u32 z = v[i].size();
		r += string((const char*)&z, sizeof(z)) + v[i];
	}
	return r;
}
template<typename x> static size_t as_val(const char* s, x& d) {
	memcpy((char*)&d, s, sizeof(x));
	return sizeof(x);
}
template<typename x> static size_t as_val(const char* s, x* d, size_t c) {
	memcpy((char*)d, s, sizeof(x) * c);
	return sizeof(x) * c;
}
template<> size_t as_val(const char* s, string& d) {
	u32 z = *(const u32*)s;
	try {
		d.resize(z);
	}
	catch (std::exception& e) {
		printf("exception = %s\n", e.what());
	}

	memcpy((void*)d.data(), s + sizeof(u32), z);
	return z + sizeof(z);
}
template<> size_t as_val(const char* s, string* d, size_t c) {
	size_t r = 0;
	for (size_t i = 0; i < c; ++i) {
		u32 z = *(const u32*)(s + r);
		d[i].resize(z);
		memcpy((void*)d[i].data(), s + r + sizeof(u32), z);
		r += sizeof(z) + z;
	}
	return r;
}


struct data_from_c {
	kk::u08 cmd;
	struct list_cmd {
		string pk()const { return ""; }
		size_t uk(const char* p, size_t c) { return 0; }
	};
	struct get_cmd {
		get_cmd() {}
		get_cmd(const char* p, size_t c, kk::u08 t) : k(p, c) {}
		string pk()const {
			string r;
			r += as_str(k);
			return r;
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			z += as_val(p + z, k);
			return z;
		}

		string  k;
	};
	struct set_cmd {
		set_cmd() : v(0) {}
		set_cmd(const char* p, size_t c, kk::u64 v) : k(p, c), v(v) {}
		string pk()const {
			string r;
			r += as_str(k);
			r += as_str(v);
			return r;
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			z += as_val(p + z, k);
			z += as_val(p + z, v);
			return z;
		}

		string  k;
		kk::u64 v;
	};
	struct get_all_cmd {
		get_all_cmd() {}
		void app(const char* p, size_t c) { s.push_back(string(p, c)); }
		string pk()const {
			string r;
			size_t c = s.size();
			r += as_str(c);
			for (size_t i = 0; i < s.size(); ++i) { r += as_str(s[i]); }
			return r;
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			size_t n = 0;
			z += as_val(p + z, n); s.resize(n);
			for (size_t i = 0; i < n; ++i) { z += as_val(p + z, s[i]); }
			return z;
		}

		vector<string> s;
	};

	string pk()const {
		string r;
		r += as_str(cmd);
		if (cmd == 0) {
			r += l.pk();
		}
		else if (cmd == 1) {
			r += g.pk();
		}
		else if (cmd == 2) {
			r += s.pk();
		}
		else if (cmd == 3) {
			r += k.pk();
		}
		else {}
		return r;
	}
	size_t uk(const char* p, size_t c) {
		size_t z = 0;
		z += as_val(p + z, cmd);
		if (cmd == 0) {
			z += l.uk(p + z, c - z);
		}
		else if (cmd == 1) {
			z += g.uk(p + z, c - z);
		}
		else if (cmd == 2) {
			z += s.uk(p + z, c - z);
		}
		else if (cmd == 3) {
			z += k.uk(p + z, c - z);
		}
		else {}
		return z;
	}

	bool is_list()const { return cmd == 0; }
	bool is_get()const { return cmd == 1; }
	bool is_set()const { return cmd == 2; }
	bool is_all()const { return cmd == 3; }

	void set_list() { cmd = 0; }
	void set_get() { cmd = 1; }
	void set_set() { cmd = 2; }
	void set_all() { cmd = 3; }

	list_cmd    l;
	get_cmd     g;
	set_cmd     s;
	get_all_cmd k;
};

struct data_from_s {
	data_from_s() : cmd(0) {}
	kk::u08 cmd;
	struct list_cmd {
		list_cmd() {}
		void app(const char* p, size_t c, u08 type) {
			n.push_back(string(p, c));
			t.push_back(type);
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			size_t k = 0;
			z += as_val(p + z, k); t.resize(k); n.resize(k);
			for (size_t i = 0; i < k; ++i) { z += as_val(p + z, n[i]); }
			for (size_t i = 0; i < k; ++i) { z += as_val(p + z, t[i]); }
			return z;
		}
		string pk()const {
			string r;
			size_t c = n.size();
			r = as_str(c) + r;
			for (size_t i = 0; i < c; ++i) { r += as_str(n[i]); }
			for (size_t i = 0; i < c; ++i) { r += as_str(t[i]); }
			return r;
		}

		vector < string > n;
		vector < kk::u08> t;
	};
	struct get_cmd {
		get_cmd(kk::u64 a = 0, kk::u64 v = 0) : v(v) {}
		string pk()const {
			string r;
			r += as_str(v);
			return r;
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			z += as_val(p + z, v);
			return z;
		}

		kk::u64 v;
	};
	struct get_all_cmd {
		get_all_cmd() : v(0) {}
		void app(u64 v) { this->v.push_back(v); }
		string pk()const {
			string r;
			size_t c = v.size();
			r += as_str(c);
			for (size_t i = 0; i < c; ++i) { r += as_str(v[i]); }
			return r;
		}
		size_t uk(const char* p, size_t c) {
			size_t z = 0;
			size_t n = 0;
			z += as_val(p + z, n); v.resize(n);
			for (size_t i = 0; i < n; ++i) { z += as_val(p + z, v[i]); }
			return z;
		}
		vector<u64> v;
	};

	bool is_list()const { return cmd == 0; }
	bool is_get()const { return cmd == 1; }
	bool is_all()const { return cmd == 2; }
	void set_list() { cmd = 0; }
	void set_get() { cmd = 1; }
	void set_all() { cmd = 2; }
	string pk()const {
		string r;
		r += as_str(cmd);
		if (cmd == 0) {
			r += l.pk();
		}
		else if (cmd == 1) {
			r += g.pk();
		}
		else if (cmd == 2) {
			r += a.pk();
		}
		else {}
		return r;
	}
	size_t uk(const char* p, size_t c) {
		size_t z = 0;
		z += as_val(p + z, cmd);
		if (cmd == 0) {
			z += l.uk(p + z, c - z);
		}
		else if (cmd == 1) {
			z += g.uk(p + z, c - z);
		}
		else if (cmd == 2) {
			z += a.uk(p + z, c - z);
		}
		else {}
		return z;
	}

	list_cmd    l;
	get_cmd     g;
	get_all_cmd a;
};
