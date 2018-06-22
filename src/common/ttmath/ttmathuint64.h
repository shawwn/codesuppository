/*
 * This file is a part of TTMath Mathematical Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@slimaczek.pl>
 */

/* 
 * Copyright (c) 2006-2007, Tomasz Sowa
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *    
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    
 *  * Neither the name Tomasz Sowa nor the names of contributors to this
 *    project may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */



/*!
	\file ttmathuint.h
    \brief template class UInt<uint> for 64bit processors
*/


namespace ttmath
{

	/*!
	*
	*	basic mathematic functions
	*
	*/

#ifdef TTMATH_PLATFORM64



	/*!
		in 64bit platforms we must define additional operators and contructors
		in order to allow a user initializing the objects in this way:
			UInt<...> type = 20;
		or
			UInt<...> type; 
			type = 30;

		decimal constants such as 20, 30 etc. are integer literal of type int,
		if the value is greater it can even be long int,
		0 is an octal integer of type int
		(ISO 14882 p2.13.1 Integer literals)
	*/

	/*!
		this operator converts the unsigned int type to this class

		***this operator is created only on a 64bit platform***
		it takes one argument of 32bit
	*/
	template<uint value_size>
	UInt<value_size> & UInt<value_size>::operator=(NxU32 i)
	{
		FromUInt(uint(i));

	return *this;
	}


	/*!
		a constructor for converting the unsigned int to this class

		***this constructor is created only on a 64bit platform***
		it takes one argument of 32bit
	*/
	template<uint value_size>
	UInt<value_size>::UInt(NxU32 i)
	{
		FromUInt(uint(i));
	}


	/*!
		an operator for converting the signed int to this class

		***this constructor is created only on a 64bit platform***
		it takes one argument of 32bit

		look at the description of UInt::operator=(sint)
	*/
	template<uint value_size>
	UInt<value_size> & UInt<value_size>::operator=(NxI32 i)
	{
		FromUInt(uint(i));

	return *this;
	}


	/*!
		a constructor for converting the signed int to this class

		***this constructor is created only on a 64bit platform***
		it takes one argument of 32bit

		look at the description of UInt::operator=(sint)
	*/
	template<uint value_size>
	UInt<value_size>::UInt(NxI32 i)
	{
		FromUInt(uint(i));
	}



	/*!
		this method copies the value stored in an another table
		(warning: first values in temp_table are the highest words -- it's different
		from our table)

		***this method is created only on a 64bit platform***

		we copy as many words as it is possible
		
		if temp_table_len is bigger than value_size we'll try to round 
		the lowest word from table depending on the last not used bit in temp_table
		(this rounding isn't a perfect rounding -- look at the description below)

		and if temp_table_len is smaller than value_size we'll clear the rest words
		in the table

		warning: we're using 'temp_table' as a pointer at 32bit words
	*/
	template<uint value_size>
	void UInt<value_size>::SetFromTable(const NxU32 * temp_table, uint temp_table_len)
	{
		uint temp_table_index = 0;
		sint i; // 'i' with a sign

		for(i=value_size-1 ; i>=0 && temp_table_index<temp_table_len; --i, ++temp_table_index)
		{
			table[i] = uint(temp_table[ temp_table_index ]) << 32;

			++temp_table_index;

			if( temp_table_index<temp_table_len )
				table[i] |= temp_table[ temp_table_index ];
		}


		// rounding mantissa
		if( temp_table_index < temp_table_len )
		{
			if( (temp_table[temp_table_index] & TTMATH_UINT_HIGHEST_BIT) != 0 )
			{
				/*
					very simply rounding
					if the bit from not used last word from temp_table is set to one
					we're rouding the lowest word in the table

					in fact there should be a normal addition but
					we don't use Add() or AddTwoInts() because these methods 
					can set a carry and then there'll be a small problem
					for optimization
				*/
				if( table[0] != TTMATH_UINT_MAX_VALUE )
					++table[0];
			}
		}

		// cleaning the rest of the mantissa
		for( ; i >= 0 ; --i)
			table[i] = 0;
	}



	/*!
		this method adding ss2 to the this and adding carry if it's defined
		(this = this + ss2 + c)

		***this method is created only on a 64bit platform***

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it was)
	*/
	template<uint value_size>
	uint UInt<value_size>::Add(const UInt<value_size> & ss2, uint c)
	{
	register uint b = value_size;
	register uint * p1 = table;
	register uint * p2 = const_cast<uint*>(ss2.table);


		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif
			

		#ifdef __GNUC__
			/*
				this part should be compiled with gcc
			*/
			__asm__ __volatile__(
			
				"push %%rbx				\n"
				"push %%rcx				\n"
				"push %%rdx				\n"
			
				"movq $0, %%rax			\n"
				"subq %%rsi, %%rax		\n"

			"1:							\n"
				"movq (%%rbx),%%rax		\n"
				"adcq (%%rdx),%%rax		\n"
				"movq %%rax,(%%rbx)		\n"
				
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				
			"loop 1b					\n"

				"movq $0, %%rax			\n"
				"adcq %%rax,%%rax		\n"
				"movq %%rax, %%rsi		\n" 

				"pop %%rdx				\n"
				"pop %%rcx				\n"
				"pop %%rbx				\n"

				: "=S" (c)
				: "0" (c), "c" (b), "b" (p1), "d" (p2)
				: "%rax", "cc", "memory" );

		#endif


	
	return c;
	}



	/*!
		this method adds one word (at a specific position)
		and returns a carry (if it was)

		***this method is created only on a 64bit platform***


		if we've got (value_size=3):
			table[0] = 10;
			table[1] = 30;
			table[2] = 5;	
		and we call:
			AddInt(2,1)
		then it'll be:
			table[0] = 10;
			table[1] = 30 + 2;
			table[2] = 5;

		of course if there was a carry from table[3] it would be returned
	*/
	template<uint value_size>
	uint UInt<value_size>::AddInt(uint value, uint index)
	{
	register uint b = value_size;
	register uint * p1 = table;
	register uint c;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__ __volatile__(
				"push %%rbx						\n"
				"push %%rcx						\n"
				"push %%rdx						\n"

				"subq %%rdx, %%rcx 				\n"

				"leaq (%%rbx,%%rdx,8), %%rbx 	\n"

				"movq %%rsi, %%rdx				\n"
				"clc							\n"
			"1:									\n"

				"movq (%%rbx), %%rax			\n"
				"adcq %%rdx, %%rax				\n"
				"movq %%rax, (%%rbx)			\n"

			"jnc 2f								\n"

				"movq $0, %%rdx					\n"

				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"

			"loop 1b							\n"

			"2:									\n"

				"movq $0, %%rax					\n"
				"adcq %%rax,%%rax				\n"

				"pop %%rdx						\n"
				"pop %%rcx						\n"
				"pop %%rbx						\n"

				: "=a" (c)
				: "c" (b), "d" (index), "b" (p1), "S" (value)
				: "cc", "memory" );

		#endif

	
	return c;
	}



	/*!
		this method adds only two unsigned words to the existing value
		and these words begin on the 'index' position
		(it's used in the multiplication algorithm 2)

		***this method is created only on a 64bit platform***

		index should be equal or smaller than value_size-2 (index <= value_size-2)
		x1 - lower word, x2 - higher word

		for example if we've got value_size equal 4 and:
			table[0] = 3
			table[1] = 4
			table[2] = 5
			table[3] = 6
		then let
			x1 = 10
			x2 = 20
		and
			index = 1

		the result of this method will be:
			table[0] = 3
			table[1] = 4 + x1 = 14
			table[2] = 5 + x2 = 25
			table[3] = 6
		
		and no carry at the end of table[3]

		(of course if there was a carry in table[2](5+20) then 
		this carry would be passed to the table[3] etc.)
	*/
	template<uint value_size>
	uint UInt<value_size>::AddTwoInts(uint x2, uint x1, uint index)
	{
	register uint b = value_size;
	register uint * p1 = table;
	register uint c;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__ __volatile__(
			
				"push %%rbx						\n"
				"push %%rcx						\n"
				"push %%rdx						\n"

				"subq %%rdx, %%rcx 				\n"
				
				"leaq (%%rbx,%%rdx,8), %%rbx 	\n"

				"movq $0, %%rdx					\n"

				"movq (%%rbx), %%rax			\n"
				"addq %%rsi, %%rax				\n"
				"movq %%rax, (%%rbx)			\n"

				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"

				"movq (%%rbx), %%rax			\n"
				"adcq %%rdi, %%rax				\n"
				"movq %%rax, (%%rbx)			\n"
			"jnc 2f								\n"

				"dec %%rcx						\n"
				"dec %%rcx						\n"
			"jz 2f								\n"

			"1:									\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"

				"movq (%%rbx), %%rax			\n"
				"adcq %%rdx, %%rax				\n"
				"movq %%rax, (%%rbx)			\n"

			"jnc 2f								\n"

			"loop 1b							\n"

			"2:									\n"

				"movq $0, %%rax					\n"
				"adcq %%rax,%%rax				\n"

				"pop %%rdx						\n"
				"pop %%rcx						\n"
				"pop %%rbx						\n"

				: "=a" (c)
				: "c" (b), "d" (index), "b" (p1), "S" (x1), "D" (x2)
				: "cc", "memory" );

		#endif

	
	return c;
	}





	/*!
		this method's subtracting ss2 from the 'this' and subtracting
		carry if it has been defined
		(this = this - ss2 - c)

		***this method is created only on a 64bit platform***

		c must be zero or one (might be a bigger value than 1)
		function returns carry (1) (if it was)
	*/
	template<uint value_size>
	uint UInt<value_size>::Sub(const UInt<value_size> & ss2, uint c)
	{
	register uint b = value_size;
	register uint * p1 = table;
	register uint * p2 = const_cast<uint*>(ss2.table);

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__  __volatile__(
			
				"push %%rbx				\n"
				"push %%rcx				\n"
				"push %%rdx				\n"

				"movq $0, %%rax			\n"
				"subq %%rsi, %%rax		\n"

			"1:							\n"
				"movq (%%rbx),%%rax		\n"
				"sbbq (%%rdx),%%rax		\n"
				"movq %%rax,(%%rbx)		\n"
				
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				"inc %%rbx				\n"
				
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				"inc %%rdx				\n"
				
			"loop 1b					\n"

				"movq $0, %%rax			\n"
				"adcq %%rax,%%rax		\n"
				"movq %%rax, %%rsi		\n" 

				"pop %%rdx				\n"
				"pop %%rcx				\n"
				"pop %%rbx				\n"

				: "=S" (c)
				: "0" (c), "c" (b), "b" (p1), "d" (p2)
				: "%rax", "cc", "memory" );

		#endif


	return c;
	}


	/*!
		this method subtracts one word (at a specific position)
		and returns a carry (if it was)

		***this method is created only on a 64bit platform***

		if we've got (value_size=3):
			table[0] = 10;
			table[1] = 30;
			table[2] = 5;	
		and we call:
			SubInt(2,1)
		then it'll be:
			table[0] = 10;
			table[1] = 30 - 2;
			table[2] = 5;

		of course if there was a carry from table[3] it would be returned
	*/
	template<uint value_size>
	uint UInt<value_size>::SubInt(uint value, uint index)
	{
	register uint b = value_size;
	register uint * p1 = table;
	register uint c;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__ __volatile__(
			
				"push %%rbx						\n"
				"push %%rcx						\n"
				"push %%rdx						\n"

				"subq %%rdx, %%rcx 				\n"

				"leaq (%%rbx,%%rdx,8), %%rbx 	\n"

				"movq %%rsi, %%rdx					\n"
				"clc							\n"
			"1:									\n"

				"movq (%%rbx), %%rax			\n"
				"sbbq %%rdx, %%rax				\n"
				"movq %%rax, (%%rbx)			\n"

			"jnc 2f								\n"

				"movq $0, %%rdx					\n"

				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"
				"inc %%rbx						\n"

			"loop 1b							\n"

			"2:									\n"

				"movq $0, %%rax					\n"
				"adcq %%rax,%%rax				\n"

				"pop %%rdx						\n"
				"pop %%rcx						\n"
				"pop %%rbx						\n"

				: "=a" (c)
				: "c" (b), "d" (index), "b" (p1), "S" (value)
				: "cc", "memory" );

		#endif

	
	return c;
	}


	/*!
		this method moves all bits into the left hand side
		return value <- this <- c

		the lowest *bits* will be held the 'c' and
		the state of one additional bit (on the left hand side)
		will be returned

		for example:
		let this is 001010000
		after Rcl2(3, 1) there'll be 010000111 and Rcl2 returns 1
	
		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcl2(uint bits, uint c)
	{
		if( bits == 0 )
			return 0;

	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )

	register sint b = value_size;
	register uint * p1 = table;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
		__asm__  __volatile__(
		
			"push %%rsi			\n"
			
		
		"2:						\n"

			"xorq %%rax,%%rax	\n"
			"subq %%rdx,%%rax	\n"

			"push %%rbx			\n"
			"push %%rcx			\n"

		"1:						\n"
			"rclq $1,(%%rbx)	\n"
			
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			"inc %%rbx			\n"
			
		"loop 1b				\n"
			
			"pop %%rcx			\n"
			"pop %%rbx			\n"

			"decq %%rsi			\n"

		"jnz 2b					\n"

			"movq $0, %%rdx		\n"
			"adcq %%rdx, %%rdx	\n"

			"pop %%rsi			\n"

			: "=d" (c)
			: "0" (c), "c" (b), "b" (p1), "S" (bits)
			: "%rax", "cc", "memory" );

		#endif


	return c;
	}


	/*!
		this method moves all bits into the right hand side
		C -> this -> return value

		the highest *bits* will be held the 'c' and
		the state of one additional bit (on the right hand side)
		will be returned

		for example:
		let this is 000000010
		after Rcr2(2, 1) there'll be 110000000 and Rcr2 returns 1

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	uint UInt<value_size>::Rcr2(uint bits, uint c)
	{
		if( bits == 0 )
			return 0;

	TTMATH_ASSERT( bits>0 && bits<TTMATH_BITS_PER_UINT )

	register sint b = value_size;
	register uint * p1 = table;


		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif


		#ifdef __GNUC__
			__asm__  __volatile__(

			"push %%rsi			\n"


		"2:						\n"


			"push %%rbx			\n"
			"push %%rcx			\n"

			"leaq (%%rbx,%%rcx,8),%%rbx  \n"

			"xorq %%rax, %%rax	\n"
			"subq %%rdx, %%rax	\n"

		"1:						\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			"dec %%rbx			\n"
			
			"rcrq $1,(%%rbx)	\n"
			
		"loop 1b				\n"

			"pop %%rcx			\n"
			"pop %%rbx			\n"

			"decq %%rsi			\n"

		"jnz 2b					\n"

			"movq $0, %%rdx		\n"
			"adcq %%rdx,%%rdx	\n"

			"pop %%rsi			\n"

			: "=d" (c)
			: "0" (c), "c" (b), "b" (p1), "S" (bits)
			: "%rax", "cc", "memory" );

		#endif


	return c;
	}


	/*
		this method returns the number of the highest set bit in one 32-bit word
		if the 'x' is zero this method returns '-1'

		***this method is created only on a 64bit platform***
	*/
	template<uint value_size>
	sint UInt<value_size>::FindLeadingBitInWord(uint x)
	{
	register sint result;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__  __volatile__(


			"bsrq %%rbx, %%rax	\n"
			"jnz 1f				\n"
			"movq $-1, %%rax	\n"
		"1:						\n"

			: "=a" (result)
			: "b" (x)
			: "cc" );

		#endif


	return result;
	}


	/*!
		this method sets a special bit in the 'value'
		and returns the result

		***this method is created only on a 64bit platform***

		bit is from <0,31>

		e.g.
		SetBitInWord(0,0) = 1
		SetBitInWord(0,2) = 4
		SetBitInWord(10, 8) = 266
	*/
	template<uint value_size>
	uint UInt<value_size>::SetBitInWord(uint value, uint bit)
	{
		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
			__asm__  __volatile__(

			"btsq %%rbx,%%rax	\n"

			: "=a" (value)
			: "0" (value), "b" (bit)
			: "cc" );

		#endif

	return value;
	}


	/*!
	 *
	 * Multiplication
	 *
	 *
	*/


	/*!
		multiplication: result2:result1 = a * b
		result2 - higher word
		result1 - lower word of the result
	
		this methos never returns a carry

		***this method is created only on a 64bit platform***

		it is an auxiliary method for version two of the multiplication algorithm
	*/
	template<uint value_size>
	void UInt<value_size>::MulTwoWords(uint a, uint b, uint * result2, uint * result1)
	{
	/*
		we must use these temporary variables in order to inform the compilator
		that value pointed with result1 and result2 has changed

		this has no effect in visual studio but it's usefull when
		using gcc and options like -O
	*/
	register uint result1_;
	register uint result2_;

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__

		__asm__ __volatile__(
		
			"mulq %%rdx			\n"

			: "=a" (result1_), "=d" (result2_)
			: "0" (a), "1" (b)
			: "cc" );

		#endif


		*result1 = result1_;
		*result2 = result2_;
	}




	/*!
	 *
	 * Division
	 *
	 *
	*/
	

	/*!
		this method calculates 64bits word a:b / 32bits c (a higher, b lower word)
		r = a:b / c and rest - remainder
		
		***this method is created only on a 64bit platform***

		*
		* WARNING:
		* if r (one word) is too small for the result or c is equal zero
		* there'll be a hardware interruption (0)
		* and probably the end of your program
		*
	*/
	template<uint value_size>
	void UInt<value_size>::DivTwoWords(uint a,uint b, uint c, uint * r, uint * rest)
	{
		register uint r_;
		register uint rest_;
		/*
			these variables have similar meaning like those in
			the multiplication algorithm MulTwoWords
		*/

		#ifndef __GNUC__
			#error "another compiler than GCC is currently not supported in 64bit mode"
		#endif

		#ifdef __GNUC__
		
			__asm__ __volatile__(

			"divq %%rcx				\n"

			: "=a" (r_), "=d" (rest_)
			: "d" (a), "a" (b), "c" (c)
			: "cc" );

		#endif


		*r = r_;
		*rest = rest_;
	}

#endif

} //namespace

