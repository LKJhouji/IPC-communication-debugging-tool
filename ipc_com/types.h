// =====================================================================================
//
//       Filename:  types.h
//
//    Description:  
//
//        Version:  $Id$
//        Created:  2018/2/14 13:38:21
//       Revision:  none
//       Compiler:  modern c++ compiler, better compatible with c++17( test on g++ 7.2.0 ).
//
//         Author:  yjj (lk), forshoujian@163.com
//				Company:  nullptr ? nullptr : my company.
//   Organization:  6k
//
//				License:  BSD-2
//
// =====================================================================================


#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstddef>
#include <cstdint>

namespace kk {
	using ub = std::uint8_t;
	using uw = std::uint16_t;
	using ud = std::uint32_t;
	using uq = std::uint64_t;

	using ib = std::int8_t;
	using iw = std::int16_t;
	using id = std::int32_t;
	using iq = std::int64_t;

	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;
	using u08 = u8;

	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;
	using i64 = std::int64_t;
	using i08 = i8;

	using ft = float;
	using dl = double;

	using cubr = const ub&;
	using cuwr = const uw&;
	using cudr = const ud&;
	using cuqr = const uq&;

	using cibr = const ib&;
	using ciwr = const iw&;
	using cidr = const id&;
	using ciqr = const iq&;

	using cu8r = const u8&;
	using cu16r = const u16&;
	using cu32r = const u32&;
	using cu64r = const u64&;
	using cu08r = cu8r;

	using ci8r = const i8&;
	using ci16r = const i16&;
	using ci32r = const i32&;
	using ci64r = const i64&;
	using ci08r = ci8r;

	using cfr = const ft&;
	using cdr = const dl&;

	using size_t = u64;
	//using size_t = u32;
	using sz_t = size_t;


	using pos_t = i64;
	//using pos_t  = i32;

	using dis_t = size_t;

	namespace abbr {
		using a = std::uint8_t;
		using b = std::uint16_t;
		using c = std::uint32_t;
		using d = std::uint64_t;

		using w = std::int8_t;
		using x = std::int16_t;
		using y = std::int32_t;
		using z = std::int64_t;
	};

	namespace consts {
		//:ref from: https://zh.wikipedia.org/wiki/%E6%95%B0%E5%AD%A6%E5%B8%B8%E6%95%B0
		// pi: 3.14159 26535 89793 23846 26433 83279 50288 41971 69399
		// e:  2.71828 18284 59045 23536 02874 71352 66249
		const dl pi = 3.14159265358979323846;
		const dl e = 2.718281828459045235360;
		const dl pi_2 = 0.5 * pi;
		const dl pi_4 = 0.25 * pi;
		const dl pi_8 = 0.5 * pi_4;

		const dl pi2 = 2.0 * pi;
		const dl pi4 = 4.0 * pi;

		const dl pi_p2 = pi * pi;

		static ft r2d(const ft& a) { return 180.0f / (ft)pi * a; }
		static ft d2r(const ft& a) { return (ft)pi / 180.0f * a; }

		static dl r2d(const dl& a) { return 180.0 / pi * a; }
		static dl d2r(const dl& a) { return pi / 180.0 * a; }

		static const std::nullptr_t nptr = nullptr;
		//static const size_t size_max = UINT32_MAX;
		static const size_t size_max = UINT64_MAX;
		static const size_t npos = size_max;
		static const pos_t  ipos = -1;			//invalid position.

	}
}
