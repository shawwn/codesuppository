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



#ifndef headerfilettmathparser
#define headerfilettmathparser

/*!
	\file ttmathparser.h
    \brief A mathematical parser
*/

#include <fstream>
#include <vector>
#include <map>
#include <set>

#include "ttmath.h"
#include "ttmathobjects.h"



namespace ttmath
{

/*! 
	\brief Mathematical parser

	let x will be an input string means an expression for converting:
	
	x = [+|-]Value[operator[+|-]Value][operator[+|-]Value]...
	where:
		an operator can be:
			^ (pow)  (the highest priority)

			* (mul) 
			/ (div)   (* and / have the same priority)

			+ (add)
			- (sub)   (+ and - have the same priority)

			< (lower than)
			> (greater than)
			<= (lower or equal than)
			>= (greater or equal than)
			== (equal)
			!= (not equal)   (all above logical operators have the same priority)
			
			&& (logical and)

			|| (logical or) (the lowest priority)


		and Value can be:
			constant e.g. 100
			variable e.g. pi
			another expression between brackets e.g (x)
			function e.g. sin(x)

	for example a correct input string can be:
		"1"
		"2.1234"
		"2,1234"    (they are the same, we can either use a comma or a dot in values)
		            (look at the macro TTMATH_COMMA_CHARACTER_2)
		"1 + 2"
		"(1 + 2) * 3"
		"pi"
		"sin(pi)"
		"(1+2)*(2+3)"
		"log(2;1234)"    there's a semicolon here (not a comma), we use it in functions
		                 for separating parameters
	    "1 < 2"  (the result will be: 1)
	    "4 < 3"  (the result will be: 0)
		etc.

	we can also use a semicolon for separating any 'x' input strings
	for example:
		"1+2;4+5"
	the result will be on the stack as follows:
		"3"
		"9"
*/
template<class ValueType>
class Parser
{

private:

/*!
	there are 5 mathematical operators as follows:
		add (+)
		sub (-)
		mul (*)
		div (/)
		pow (^)
	with their standard priorities
*/
	class MatOperator
	{
	public:

		enum Type
		{
			none,add,sub,mul,div,pow,lt,gt,let,get,eq,neq,lor,land
		};


		Type GetType()     const { return type; }
		NxI32  GetPriority() const { return priority; }


		void SetType(Type t)
		{
			type = t;

			switch( type )
			{		
			case lor:
				priority = 4;
				break;

			case land:
				priority = 5;
				break;

			case eq:
			case neq:
			case lt:
			case gt:
			case let:
			case get:
				priority = 7;
				break;

			case add:
			case sub:
				priority = 10;
				break;

			case mul:
			case div:
				priority = 12;
				break;

			case pow:
				priority = 14;
				break;

			default:
				Error( err_internal_error );
				break;
			}
		}

		MatOperator(): type(none), priority(0)
		{
		}

	private:

		Type type;
		NxI32  priority;

	}; // end of MatOperator class



public:



	/*!
		Objects of type 'Item' we are keeping on our stack
	*/
	struct Item
	{
		enum Type
		{
			none, numerical_value, mat_operator, first_bracket,
			last_bracket, variable, semicolon
		};

		// The kind of type which we're keeping
		Type type;

		// if type == numerical_value
		ValueType value;

		// if type == mat_operator
		MatOperator moperator;

		/*
			if type == first_bracket

			if 'function' is set to true it means that the first recognized bracket
			was the bracket from function in other words we must call a function when
			we'll find the 'last' bracket
		*/
		bool function;

		// if function is true
		std::string function_name;

		/*
			the sign of value

			it can be for type==numerical_value or type==first_bracket
			when it's true it means e.g. that value is equal -value
		*/
		bool sign;

		Item(): type(none), function(false), sign(false)
		{
		}

	}; // end of Item struct


/*!
	stack on which we're keeping the Items

	at the end of parsing we'll have the result on its
	the result don't have to be one value, it can be a list
	of values separated by the 'semicolon item'
*/
std::vector<Item> stack;


private:


/*!
	size of the stack when we're starting parsing of the string

	if it's to small while parsing the stack will be automatically resized
*/
const NxI32 default_stack_size;



/*!
	index of an object in our stack
	it's pointing on the place behind the last element
	for example at the beginning of parsing its value is zero
*/
NxU32 stack_index;


/*!
	code of the last error
*/
ErrorCode error;


/*!
	pointer to the currently reading char

	when an error has occured it may be used to count the index of the wrong character
*/
const char * pstring;


/*!
	the base of the mathematic system (for example it may be '10')
*/
NxI32 base;


/*!
	a pointer to an object which tell us whether we should stop calculating or not
*/
const volatile StopCalculating * pstop_calculating;



/*!
	a pointer to the user-defined variables' table
*/
const Objects * puser_variables;

/*!
	a pointer to the user-defined functions' table
*/
const Objects * puser_functions;


typedef std::map<std::string, ValueType> FunctionLocalVariables;

/*!
	a pointer to the local variables of a function
*/
const FunctionLocalVariables * pfunction_local_variables;


/*!
	a temporary set using during parsing user defined variables
*/
std::set<std::string> visited_variables;


/*!
	a temporary set using during parsing user defined functions
*/
std::set<std::string> visited_functions;




/*!
	pfunction is the type of pointer to a mathematic function

	these mathematic functions are private members of this class,
	they are the wrappers for standard mathematics function

	'pstack' is the pointer to the first argument on our stack
	'amount_of_arg' tell us how many argument there are in our stack
	'result' is the reference for result of function 
*/
typedef void (Parser<ValueType>::*pfunction)(NxI32 pstack, NxI32 amount_of_arg, ValueType & result);


/*!
	pfunction is the type of pointer to a method which returns value of variable
*/
typedef void (ValueType::*pfunction_var)();


/*!
	table of mathematic functions

	this map consists of:
		std::string - function's name
		pfunction   - pointer to specific function
*/
typedef std::map<std::string, pfunction> FunctionsTable;
FunctionsTable functions_table;


/*!
	table of mathematic operators

	this map consists of:
		std::string - operators's name
		MatOperator::Type - type of the operator
*/
typedef std::map<std::string, typename MatOperator::Type> OperatorsTable;
OperatorsTable operators_table;


/*!
	table of mathematic variables

	this map consists of:
		std::string   - variable's name
		pfunction_var - pointer to specific function which returns value of variable
*/
typedef std::map<std::string, pfunction_var> VariablesTable;
VariablesTable variables_table;



/*!
	you can't calculate the factorial if the argument is greater than 'factorial_max'
	default value is zero which means there are not any limitations
*/
ValueType factorial_max;


/*!
	we're using this method for reporting an error
*/
static void Error(ErrorCode code)
{
	throw code;
}




/*!
	this method skips the white character from the string

	it's moving the 'pstring' to the first no-white character
*/
void SkipWhiteCharacters()
{
	while( (*pstring==' ' ) || (*pstring=='\t') )
		++pstring;
}



/*!
	a auxiliary method for RecurrenceParsingVariablesOrFunction(...)
*/
void RecurrenceParsingVariablesOrFunction_CheckStopCondition(bool variable, const std::string & name)
{
	if( variable )
	{
		if( visited_variables.find(name) != visited_variables.end() )
			Error( err_variable_loop );
	}
	else
	{
		if( visited_functions.find(name) != visited_functions.end() )
			Error( err_functions_loop );
	}
}


/*!
	a auxiliary method for RecurrenceParsingVariablesOrFunction(...)
*/
void RecurrenceParsingVariablesOrFunction_AddName(bool variable, const std::string & name)
{
	if( variable )
		visited_variables.insert( name );
	else
		visited_functions.insert( name );
}

/*!
	a auxiliary method for RecurrenceParsingVariablesOrFunction(...)
*/
void RecurrenceParsingVariablesOrFunction_DeleteName(bool variable, const std::string & name)
{
	if( variable )
		visited_variables.erase( name );
	else
		visited_functions.erase( name );
}

/*!
	this method returns the value of a variable or function
	by creating a new instance of the mathematical parser 
	and making the standard parsing algorithm on the given string

	this method is used only during parsing user defined variables or functions

	(there can be a recurrence here therefore we're using 'visited_variables'
	and 'visited_functions' sets to make a stop condition)
*/
ValueType RecurrenceParsingVariablesOrFunction(bool variable, const std::string & name, const char * new_string, FunctionLocalVariables * local_variables = 0)
{
	RecurrenceParsingVariablesOrFunction_CheckStopCondition(variable, name);
	RecurrenceParsingVariablesOrFunction_AddName(variable, name);

	Parser<ValueType> NewParser(*this);
	ErrorCode err;

	NewParser.pfunction_local_variables = local_variables;

	try
	{
		err = NewParser.Parse(new_string);
	}
	catch(...)
	{
		RecurrenceParsingVariablesOrFunction_DeleteName(variable, name);

	throw;
	}

	RecurrenceParsingVariablesOrFunction_DeleteName(variable, name);

	if( err != err_ok )
		Error( err );

	if( NewParser.stack.size() != 1 )
		Error( err_must_be_only_one_value );

	if( NewParser.stack[0].type != Item::numerical_value )
		// I think there shouldn't be this error here
		Error( err_incorrect_value );

return NewParser.stack[0].value;
}


public:


/*!
	this method returns the user-defined value of a variable
*/
bool GetValueOfUserDefinedVariable(const std::string & variable_name,ValueType & result)
{
	if( !puser_variables )
		return false;

	const char * string_value;

	if( puser_variables->GetValue(variable_name, &string_value) != err_ok )
		return false;

	result = RecurrenceParsingVariablesOrFunction(true, variable_name, string_value);

return true;
}


/*!
	this method returns the value of a local variable of a function
*/
bool GetValueOfFunctionLocalVariable(const std::string & variable_name, ValueType & result)
{
	if( !pfunction_local_variables )
		return false;

	typename FunctionLocalVariables::const_iterator i = pfunction_local_variables->find(variable_name);

	if( i == pfunction_local_variables->end() )
		return false;

	result = i->second;

return true;
}


/*!
	this method returns the value of a variable from variables' table

	we make an object of type ValueType then call a method which 
	sets the correct value in it and finally we'll return the object
*/
ValueType GetValueOfVariable(const std::string & variable_name)
{
ValueType result;

	if( GetValueOfFunctionLocalVariable(variable_name, result) )
		return result;

	if( GetValueOfUserDefinedVariable(variable_name, result) )
		return result;


	typename std::map<std::string, pfunction_var>::iterator i =
													variables_table.find(variable_name);

	if( i == variables_table.end() )
		Error( err_unknown_variable );

	(result.*(i->second))();

return result;
}


private:

/*!
	wrappers for mathematic functions

	'sindex' is pointing on the first argument on our stack 
			 (the second argument has 'sindex+2'
			 because 'sindex+1' is guaranted for the 'semicolon' operator)
			 the third artument has of course 'sindex+4' etc.

	'result' will be the result of the function

	(we're using exceptions here for example when function gets an improper argument)
*/


/*!
	'silnia' in polish language
	result = 1 * 2 * 3 * 4 * .... * x
*/
void Factorial(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	
	if( !factorial_max.IsZero() && stack[sindex].value > factorial_max )
		Error( err_too_big_factorial );

	result = ttmath::Factorial(stack[sindex].value, &err, pstop_calculating);

	if(err != err_ok)
		Error( err );
}


void Abs(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::Abs(stack[sindex].value);
}

void Sin(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::Sin(stack[sindex].value);
}

void Cos(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::Cos(stack[sindex].value);
}

void Tan(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Tan(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}

void Cot(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Cot(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}

void Int(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::SkipFraction(stack[sindex].value);
}


void Round(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::Round(stack[sindex].value);
}


void Ln(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Ln(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}

void Log(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Log(stack[sindex].value, stack[sindex+2].value, &err);

	if(err != err_ok)
		Error( err );
}

void Exp(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Exp(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}


void Max(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args == 0 )
	{
		result.SetMax();

	return;
	}

	result = stack[sindex].value;

	for(NxI32 i=1 ; i<amount_of_args ; ++i)
	{
		if( result < stack[sindex + i*2].value )
			result = stack[sindex + i*2].value;
	}
}


void Min(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args == 0 )
	{
		result.SetMin();

	return;
	}

	result = stack[sindex].value;

	for(NxI32 i=1 ; i<amount_of_args ; ++i)
	{
		if( result > stack[sindex + i*2].value )
			result = stack[sindex + i*2].value;
	}
}


void ASin(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ASin(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}


void ACos(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ACos(stack[sindex].value, &err);

	if(err != err_ok)
		Error( err );
}


void ATan(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::ATan(stack[sindex].value);
}


void ACot(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::ACot(stack[sindex].value);
}


void Sgn(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	result = ttmath::Sgn(stack[sindex].value);
}


void Mod(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	if( stack[sindex+2].value.IsZero() )
		Error( err_improper_argument );

	result = stack[sindex].value;
	uint c = result.Mod(stack[sindex+2].value);

	if( c )
		Error( err_overflow );
}


void If(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 3 )
		Error( err_improper_amount_of_arguments );


	if( !stack[sindex].value.IsZero() )
		result = stack[sindex+2].value;
	else
		result = stack[sindex+4].value;
}


void Or(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args < 2 )
		Error( err_improper_amount_of_arguments );

	for(NxI32 i=0 ; i<amount_of_args ; ++i)
	{
		if( !stack[sindex+i*2].value.IsZero() )
		{
			result.SetOne();
			return;
		}
	}

	result.SetZero();
}


void And(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args < 2 )
		Error( err_improper_amount_of_arguments );

	for(NxI32 i=0 ; i<amount_of_args ; ++i)
	{
		if( stack[sindex+i*2].value.IsZero() )
		{
			result.SetZero();
			return;
		}
	}

	result.SetOne();
}


void Not(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );


	if( stack[sindex].value.IsZero() )
		result.SetOne();
	else
		result.SetZero();
}


void DegToRad(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	ErrorCode err;

	if( amount_of_args == 1 )
	{
		result = ttmath::DegToRad(stack[sindex].value, &err);
	}
	else
	if( amount_of_args == 3 )
	{
		result = ttmath::DegToRad(	stack[sindex].value, stack[sindex+2].value,
									stack[sindex+4].value, &err);
	}
	else
		Error( err_improper_amount_of_arguments );


	if( err != err_ok )
		Error( err );
}


void RadToDeg(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	ErrorCode err;

	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );
	
	result = ttmath::RadToDeg(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void DegToDeg(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 3 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::DegToDeg(	stack[sindex].value, stack[sindex+2].value,
								stack[sindex+4].value, &err);

	if( err != err_ok )
		Error( err );
}

void Ceil(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Ceil(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Floor(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Floor(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}

void Sqrt(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Sqrt(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Sinh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Sinh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Cosh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Cosh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Tanh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Tanh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Coth(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Coth(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void Root(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::Root(stack[sindex].value, stack[sindex+2].value, &err);

	if( err != err_ok )
		Error( err );
}



void ASinh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ASinh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void ACosh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ACosh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void ATanh(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ATanh(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void ACoth(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 1 )
		Error( err_improper_amount_of_arguments );

	ErrorCode err;
	result = ttmath::ACoth(stack[sindex].value, &err);

	if( err != err_ok )
		Error( err );
}


void BitAnd(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	uint err;
	result = stack[sindex].value;
	err = result.BitAnd(stack[sindex+2].value);

	switch(err)
	{
	case 1:
		Error( err_overflow );
		break;
	case 2:
		Error( err_improper_argument );
		break;
	}
}

void BitOr(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	uint err;
	result = stack[sindex].value;
	err = result.BitOr(stack[sindex+2].value);

	switch(err)
	{
	case 1:
		Error( err_overflow );
		break;
	case 2:
		Error( err_improper_argument );
		break;
	}
}


void BitXor(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args != 2 )
		Error( err_improper_amount_of_arguments );

	uint err;
	result = stack[sindex].value;
	err = result.BitXor(stack[sindex+2].value);

	switch(err)
	{
	case 1:
		Error( err_overflow );
		break;
	case 2:
		Error( err_improper_argument );
		break;
	}
}


void Sum(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args == 0 )
		Error( err_improper_amount_of_arguments );

	result = stack[sindex].value;

	for(NxI32 i=1 ; i<amount_of_args ; ++i )
		if( result.Add( stack[ sindex + i*2 ].value ) )
			Error( err_overflow );
}	

void Avg(NxI32 sindex, NxI32 amount_of_args, ValueType & result)
{
	if( amount_of_args == 0 )
		Error( err_improper_amount_of_arguments );

	result = stack[sindex].value;

	for(NxI32 i=1 ; i<amount_of_args ; ++i )
		if( result.Add( stack[ sindex + i*2 ].value ) )
			Error( err_overflow );

	if( result.Div( amount_of_args ) )
		Error( err_overflow );
}	


/*!
	this method returns the value from a user-defined function

	(look at the description in 'CallFunction(...)')
*/
bool GetValueOfUserDefinedFunction(const std::string & function_name, NxI32 amount_of_args, NxI32 sindex)
{
	if( !puser_functions )
		return false;

	const char * string_value;
	NxI32 param;

	if( puser_functions->GetValueAndParam(function_name, &string_value, &param) != err_ok )
		return false;

	if( param != amount_of_args )
		Error( err_improper_amount_of_arguments );


	FunctionLocalVariables local_variables;

	if( amount_of_args > 0 )
	{
		char buffer[20];

		// x = x1
		sprintf(buffer,"x");
		local_variables.insert( std::make_pair(buffer, stack[sindex].value) );

		for(NxI32 i=0 ; i<amount_of_args ; ++i)
		{
			sprintf(buffer,"x%d",i+1);
			local_variables.insert( std::make_pair(buffer, stack[sindex + i*2].value) );
		}
	}

	stack[sindex-1].value = RecurrenceParsingVariablesOrFunction(false, function_name, string_value, &local_variables);

return true;
}


/*
	we're calling a specific function

	function_name  - name of the function
	amount_of_args - how many arguments there are on our stack
					 (function must check whether this is a correct value or not)
	sindex         - index of the first argument on the stack (sindex is greater than zero)
  					 if there aren't any arguments on the stack 'sindex' pointing on
					 a non existend element (after the first bracket)

	result will be stored in 'stack[sindex-1].value'
	(we don't have to set the correct type of this element, it'll be set later)
*/
void CallFunction(const std::string & function_name, NxI32 amount_of_args, NxI32 sindex)
{
	if( GetValueOfUserDefinedFunction(function_name, amount_of_args, sindex) )
		return;

	typename FunctionsTable::iterator i = functions_table.find( function_name );

	if( i == functions_table.end() )
		Error( err_unknown_function );

	/*
		calling the specify function
	*/
	(this->*(i->second))(sindex, amount_of_args, stack[sindex-1].value);
}





/*!
	insert a function to the functions' table

	function_name - name of the function
	pf - pointer to the function (to the wrapper)
*/
void InsertFunctionToTable(const std::string & function_name, pfunction pf)
{
	functions_table.insert( std::make_pair(function_name, pf));
}


/*!
	insert a function to the variables' table
	(this function returns value of variable)

	variable_name - name of the function
	pf - pointer to the function
*/
void InsertVariableToTable(const std::string & variable_name, pfunction_var pf)
{
	variables_table.insert( std::make_pair(variable_name, pf));
}


/*!
	this method creates the table of functions
*/
void CreateFunctionsTable()
{
	/*
		names of functions should consist of small letters
	*/
	InsertFunctionToTable(std::string("factorial"), &Parser<ValueType>::Factorial);
	InsertFunctionToTable(std::string("abs"),   	&Parser<ValueType>::Abs);
	InsertFunctionToTable(std::string("sin"),   	&Parser<ValueType>::Sin);
	InsertFunctionToTable(std::string("cos"),   	&Parser<ValueType>::Cos);
	InsertFunctionToTable(std::string("tan"),   	&Parser<ValueType>::Tan);
	InsertFunctionToTable(std::string("tg"),	   	&Parser<ValueType>::Tan);
	InsertFunctionToTable(std::string("cot"),  		&Parser<ValueType>::Cot);
	InsertFunctionToTable(std::string("ctg"),  		&Parser<ValueType>::Cot);
	InsertFunctionToTable(std::string("int"),   	&Parser<ValueType>::Int);
	InsertFunctionToTable(std::string("round"), 	&Parser<ValueType>::Round);
	InsertFunctionToTable(std::string("ln"),    	&Parser<ValueType>::Ln);
	InsertFunctionToTable(std::string("log"),   	&Parser<ValueType>::Log);
	InsertFunctionToTable(std::string("exp"),   	&Parser<ValueType>::Exp);
	InsertFunctionToTable(std::string("max"),   	&Parser<ValueType>::Max);
	InsertFunctionToTable(std::string("min"),   	&Parser<ValueType>::Min);
	InsertFunctionToTable(std::string("asin"),   	&Parser<ValueType>::ASin);
	InsertFunctionToTable(std::string("acos"),   	&Parser<ValueType>::ACos);
	InsertFunctionToTable(std::string("atan"),   	&Parser<ValueType>::ATan);
	InsertFunctionToTable(std::string("atg"),   	&Parser<ValueType>::ATan);
	InsertFunctionToTable(std::string("acot"),   	&Parser<ValueType>::ACot);
	InsertFunctionToTable(std::string("actg"),   	&Parser<ValueType>::ACot);
	InsertFunctionToTable(std::string("sgn"),   	&Parser<ValueType>::Sgn);
	InsertFunctionToTable(std::string("mod"),   	&Parser<ValueType>::Mod);
	InsertFunctionToTable(std::string("if"),   		&Parser<ValueType>::If);
	InsertFunctionToTable(std::string("or"),   		&Parser<ValueType>::Or);
	InsertFunctionToTable(std::string("and"),  		&Parser<ValueType>::And);
	InsertFunctionToTable(std::string("not"),  		&Parser<ValueType>::Not);
	InsertFunctionToTable(std::string("degtorad"),	&Parser<ValueType>::DegToRad);
	InsertFunctionToTable(std::string("radtodeg"),	&Parser<ValueType>::RadToDeg);
	InsertFunctionToTable(std::string("degtodeg"),	&Parser<ValueType>::DegToDeg);
	InsertFunctionToTable(std::string("ceil"),		&Parser<ValueType>::Ceil);
	InsertFunctionToTable(std::string("floor"),		&Parser<ValueType>::Floor);
	InsertFunctionToTable(std::string("sqrt"),		&Parser<ValueType>::Sqrt);
	InsertFunctionToTable(std::string("sinh"),		&Parser<ValueType>::Sinh);
	InsertFunctionToTable(std::string("cosh"),		&Parser<ValueType>::Cosh);
	InsertFunctionToTable(std::string("tanh"),		&Parser<ValueType>::Tanh);
	InsertFunctionToTable(std::string("tgh"),		&Parser<ValueType>::Tanh);
	InsertFunctionToTable(std::string("coth"),		&Parser<ValueType>::Coth);
	InsertFunctionToTable(std::string("ctgh"),		&Parser<ValueType>::Coth);
	InsertFunctionToTable(std::string("root"),		&Parser<ValueType>::Root);
	InsertFunctionToTable(std::string("asinh"),		&Parser<ValueType>::ASinh);
	InsertFunctionToTable(std::string("acosh"),		&Parser<ValueType>::ACosh);
	InsertFunctionToTable(std::string("atanh"),		&Parser<ValueType>::ATanh);
	InsertFunctionToTable(std::string("atgh"),		&Parser<ValueType>::ATanh);
	InsertFunctionToTable(std::string("acoth"),		&Parser<ValueType>::ACoth);
	InsertFunctionToTable(std::string("actgh"),		&Parser<ValueType>::ACoth);
	InsertFunctionToTable(std::string("bitand"),	&Parser<ValueType>::BitAnd);
	InsertFunctionToTable(std::string("bitor"),		&Parser<ValueType>::BitOr);
	InsertFunctionToTable(std::string("bitxor"),	&Parser<ValueType>::BitXor);
	InsertFunctionToTable(std::string("band"),		&Parser<ValueType>::BitAnd);
	InsertFunctionToTable(std::string("bor"),		&Parser<ValueType>::BitOr);
	InsertFunctionToTable(std::string("bxor"),		&Parser<ValueType>::BitXor);
	InsertFunctionToTable(std::string("sum"),		&Parser<ValueType>::Sum);
	InsertFunctionToTable(std::string("avg"),		&Parser<ValueType>::Avg);
}


/*!
	this method create the table of variables
*/
void CreateVariablesTable()
{
	/*
		names of variables should consist of small letters
	*/
	InsertVariableToTable(std::string("pi"), &ValueType::SetPi);
	InsertVariableToTable(std::string("e"),  &ValueType::SetE);
}


/*!
	convert from a big letter to a small one
*/
NxI32 ToLowerCase(NxI32 c)
{
	if( c>='A' && c<='Z' )
		return c - 'A' + 'a';

return c;
}




/*!
	this method read the name of a variable or a function
	
		'result' will be the name of a variable or a function
		function return 'false' if this name is the name of a variable
		or function return 'true' if this name is the name of a function

	what should be returned is tested just by a '(' character that means if there's
	a '(' character after a name that function returns 'true'
*/
bool ReadName(std::string & result)
{
NxI32 character;


	result.erase();
	character = ToLowerCase(*pstring);

	/*
		the first letter must be from range 'a' - 'z'
	*/
	if( character<'a' || character>'z' )
		Error( err_unknown_character );


	do
	{
		result   += character;
		character = ToLowerCase( * ++pstring );
	}
	while(	(character>='a' && character<='z') ||
			(character>='0' && character<='9')	);
	

	SkipWhiteCharacters();
	

	/*
		if there's character '(' that means this name is a name of a function
	*/
	if( *pstring == '(' )
	{
		++pstring;
		return true;
	}
	
	
return false;
}


/*!
	we're checking whether the first character is '-' or '+'
	if it is we'll return 'true' and if it is equally '-' we'll set the 'sign' member of 'result'
*/
bool TestSign(Item & result)
{
	SkipWhiteCharacters();
	result.sign = false;

	if( *pstring == '-' || *pstring == '+' )
	{
		if( *pstring == '-' )
			result.sign = true;

		++pstring;

	return true;
	}

return false;
}


/*!
	we're reading the name of a variable or a function
	if is there a function we'll return 'true'
*/
bool ReadVariableOrFunction(Item & result)
{
std::string name;
bool is_it_name_of_function = ReadName(name);

	if( is_it_name_of_function )
	{
		/*
			we've read the name of a function
		*/
		result.function_name = name;
		result.type     = Item::first_bracket;
		result.function = true;
	}
	else
	{
		/*
			we've read the name of a variable and we're getting its value now
		*/
		result.value = GetValueOfVariable( name );
	}

return is_it_name_of_function;
}


/*!
	we're reading a numerical value directly from the string	
*/
void ReadValue(Item & result)
{
const char * new_stack_pointer;

	NxI32 carry = result.value.FromString(pstring, base, &new_stack_pointer);
	pstring   = new_stack_pointer;

	if( carry )
		Error( err_overflow );
}



/*!
	this method converts the character ascii c into the value in range <0;base-1>

	if the character is incorrect for this base the funcion will return -1
*/
NxI32 CharToDigit(NxI32 c, NxI32 base)
{
	if( c>='0' && c<='9' )
		c=c-'0';
	else
	if( c>='a' && c<='z' )
		c=c-'a'+10;
	else
	if( c>='A' && c<='Z' )
		c=c-'A'+10;
	else
		return -1;

	if( c >= base )
		return -1;

return c;
}


/*!
	we're reading the item
  
	return values:
		0 - all ok, the item is successfully read
		1 - the end of the string (the item is not read)
		2 - the final bracket ')'
*/
NxI32 ReadValueVariableOrFunction(Item & result)
{
bool it_was_sign = false;
NxI32  character;


	if( TestSign(result) )
		// 'result.sign' was set as well
		it_was_sign = true;

	SkipWhiteCharacters();
	character = ToLowerCase( *pstring );


	if( character == 0 )
	{
		if( it_was_sign )
			// at the end of the string a character like '-' or '+' has left
			Error( err_unexpected_end );

		// there's the end of the string here
		return 1;
	}
	else
	if( character == '(' )
	{
		// we've got a normal bracket (not a function)
		result.type = Item::first_bracket;
		result.function = false;
		++pstring;

	return 0;
	}
	else
	if( character == ')' )
	{
		// we've got a final bracket
		// (in this place we can find a final bracket only when there are empty brackets
		// without any values inside or with a sign '-' or '+' inside)

		if( it_was_sign )
			Error( err_unexpected_final_bracket );

		result.type = Item::last_bracket;

		// we don't increment 'pstring', this final bracket will be read next by the 
		// 'ReadOperatorAndCheckFinalBracket(...)' method

	return 2;
	}
	else
	if( character=='#' || character=='&' ||
		character==TTMATH_COMMA_CHARACTER_1 ||
		(character==TTMATH_COMMA_CHARACTER_2 && TTMATH_COMMA_CHARACTER_2 != 0) ||
		CharToDigit(character, base)!=-1 )
	{
		/*
			warning:
			if we're using for example the base equal 16
			we can find a first character like 'e' that is there is not e=2.71..
			but the value 14, for this case we must use something like var::e for variables
			(not implemented yet)
		*/
		ReadValue( result );
	}
	else
	if( character>='a' && character<='z' )
	{
		if( ReadVariableOrFunction(result) )
			// we've read the name of a function
			return 0;
	}
	else
		Error( err_unknown_character );



	/*
		we've got a value in the 'result'
		this value is from a variable or directly from the string
	*/
	result.type = Item::numerical_value;

	if( result.sign )
	{
		result.value.ChangeSign();
		result.sign = false;
	}


return 0;
}


void InsertOperatorToTable(const std::string & name, typename MatOperator::Type type)
{
	operators_table.insert( std::make_pair(name, type) );
}

/*!
	this method creates the table of operators
*/
void CreateMathematicalOperatorsTable()
{
	InsertOperatorToTable(std::string("||"), MatOperator::lor);
	InsertOperatorToTable(std::string("&&"), MatOperator::land);
	InsertOperatorToTable(std::string("!="), MatOperator::neq);
	InsertOperatorToTable(std::string("=="), MatOperator::eq);
	InsertOperatorToTable(std::string(">="), MatOperator::get);
	InsertOperatorToTable(std::string("<="), MatOperator::let);
	InsertOperatorToTable(std::string(">"),  MatOperator::gt);
	InsertOperatorToTable(std::string("<"),  MatOperator::lt);
	InsertOperatorToTable(std::string("-"),  MatOperator::sub);
	InsertOperatorToTable(std::string("+"),  MatOperator::add);
	InsertOperatorToTable(std::string("/"),  MatOperator::div);
	InsertOperatorToTable(std::string("*"),  MatOperator::mul);
	InsertOperatorToTable(std::string("^"),  MatOperator::pow);
}


/*!
	returns true if 'str2' is the substring of str1

	e.g.
	true when str1="test" and str2="te"
*/
bool IsSubstring(const std::string & str1, const std::string & str2)
{
	if( str2.length() > str1.length() )
		return false;

	for(std::string::size_type i=0 ; i<str2.length() ; ++i)
		if( str1[i] != str2[i] )
			return false;

return true;
}


/*!
	this method reads a mathematical (or logical) operator
*/
void ReadMathematicalOperator(Item & result)
{
std::string oper;
typename OperatorsTable::iterator iter_old, iter_new;

	iter_old = operators_table.end();

	for( ; true ; ++pstring )
	{
		oper += *pstring;
		iter_new = operators_table.lower_bound(oper);
		
		if( iter_new == operators_table.end() || !IsSubstring(iter_new->first, oper) )
		{
			oper.erase( --oper.end() ); // we've got mininum one element

			if( iter_old != operators_table.end() && iter_old->first == oper )
			{
				result.type = Item::mat_operator;
				result.moperator.SetType( iter_old->second );
				break;
			}
			
			Error( err_unknown_operator );
		}
	
		iter_old = iter_new;
	}
}


/*!
	this method reads a mathematic operators
	or the final bracket or the semicolon operator

	return values:
		0 - ok
		1 - the string is finished
*/
NxI32 ReadOperator(Item & result)
{
	SkipWhiteCharacters();
	
	if( *pstring == 0 )
		return 1;
	else
	if( *pstring == ')' )
	{
		result.type = Item::last_bracket;
		++pstring;
	}
	else
	if( *pstring == ';' )
	{
		result.type = Item::semicolon;
		++pstring;
	}
	else
		ReadMathematicalOperator(result);

return 0;
}



/*!
	this method is making the standard mathematic operation like '-' '+' '*' '/' and '^'

	the operation is made between 'value1' and 'value2'
	the result of this operation is stored in the 'value1'
*/
void MakeStandardMathematicOperation(ValueType & value1, typename MatOperator::Type mat_operator,
									const ValueType & value2)
{
NxI32 res;

	switch( mat_operator )
	{
	case MatOperator::land:
		(!value1.IsZero() && !value2.IsZero()) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::lor:
		(!value1.IsZero() || !value2.IsZero()) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::eq:
		(value1 == value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::neq:
		(value1 != value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::lt:
		(value1 < value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::gt:
		(value1 > value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::let:
		(value1 <= value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::get:
		(value1 >= value2) ? value1.SetOne() : value1.SetZero();
		break;

	case MatOperator::sub:
		if( value1.Sub(value2) ) Error( err_overflow );
		break;

	case MatOperator::add:
		if( value1.Add(value2) ) Error( err_overflow );
		break;

	case MatOperator::mul:
		if( value1.Mul(value2) ) Error( err_overflow );
		break;

	case MatOperator::div:
		if( value2.IsZero() )    Error( err_division_by_zero );
		if( value1.Div(value2) ) Error( err_overflow );
		break;

	case MatOperator::pow:
		res = value1.Pow( value2 );

		if( res == 1 ) Error( err_overflow );
		else
		if( res == 2 ) Error( err_improper_argument );

		break;

	default:
		/*
			on the stack left an unknown operator but we had to recognize its before
			that means there's an error in our algorithm
		*/
		Error( err_internal_error );
	}
}




/*!
	this method is trying to roll the stack up with the operator's priority

	for example if there are:
		"1 - 2 +" 
	we can subtract "1-2" and the result store on the place where is '1' and copy the last
	operator '+', that means there'll be '-1+' on our stack

	but if there are:
		"1 - 2 *"
	we can't roll the stack up because the operator '*' has greater priority than '-'
*/
void TryRollingUpStackWithOperatorPriority()
{
	while(	stack_index>=4 &&
			stack[stack_index-4].type == Item::numerical_value &&
			stack[stack_index-3].type == Item::mat_operator &&
			stack[stack_index-2].type == Item::numerical_value &&
			stack[stack_index-1].type == Item::mat_operator &&
			stack[stack_index-3].moperator.GetPriority() >= stack[stack_index-1].moperator.GetPriority()
		 )
	{
		MakeStandardMathematicOperation(stack[stack_index-4].value,
										stack[stack_index-3].moperator.GetType(),
										stack[stack_index-2].value);


		/*
			copying the last operator and setting the stack pointer to the correct value
		*/
		stack[stack_index-3] = stack[stack_index-1];
		stack_index -= 2;
	}
}


/*!
	this method is trying to roll the stack up without testing any operators

	for example if there are:
		"1 - 2" 
	there'll be "-1" on our stack
*/
void TryRollingUpStack()
{
	while(	stack_index >= 3 &&
			stack[stack_index-3].type == Item::numerical_value &&
			stack[stack_index-2].type == Item::mat_operator &&
			stack[stack_index-1].type == Item::numerical_value )
	{
		MakeStandardMathematicOperation(	stack[stack_index-3].value,
											stack[stack_index-2].moperator.GetType(),
											stack[stack_index-1].value );

		stack_index -= 2;
	}
}


/*!
	this method is reading a value or a variable or a function
	(the normal first bracket as well) and push it into the stack
*/
NxI32 ReadValueVariableOrFunctionAndPushItIntoStack(Item & temp)
{
NxI32 kod = ReadValueVariableOrFunction( temp );
	
	if( kod == 0 )
	{
		if( stack_index < stack.size() )
			stack[stack_index] = temp;
		else
			stack.push_back( temp );

		++stack_index;
	}

	if( kod == 2 )
		// there was a final bracket, we didn't push it into the stack 
		// (it'll be read by the 'ReadOperatorAndCheckFinalBracket' method next)
		kod = 0;


return kod;
}



/*!
	this method calculate how many parameters there are on the stack
	and index of the first parameter

	if there aren't any parameters on the stack this method returns
	'size' equals zero and 'index' pointing after the first bracket
	(on non-existend element)
*/
void HowManyParameters(NxI32 & size, NxI32 & index)
{
	size  = 0;
	index = stack_index;

	if( index == 0 )
		// we haven't put a first bracket on the stack
		Error( err_unexpected_final_bracket );


	if( stack[index-1].type == Item::first_bracket )
		// empty brackets
		return;

	for( --index ; index>=1 ; index-=2 )
	{
		if( stack[index].type != Item::numerical_value )
		{
			/*
				this element must be 'numerical_value', if not that means 
				there's an error in our algorithm
			*/
			Error( err_internal_error );
		}

		++size;

		if( stack[index-1].type != Item::semicolon )
			break;
	}

	if( index<1 || stack[index-1].type != Item::first_bracket )
	{
		/*
			we haven't put a first bracket on the stack
		*/
		Error( err_unexpected_final_bracket );
	}
}


/*!
	this method is being called when the final bracket ')' is being found

	this method's rolling the stack up, counting how many parameters there are
	on the stack and if there was a function it's calling the function
*/
void RollingUpFinalBracket()
{
NxI32 amount_of_parameters;
NxI32 index;

	
	if( stack_index<1 ||
		(stack[stack_index-1].type != Item::numerical_value &&
		 stack[stack_index-1].type != Item::first_bracket)
	  )
		Error( err_unexpected_final_bracket );
	

	TryRollingUpStack();
	HowManyParameters(amount_of_parameters, index);

	// 'index' will be greater than zero
	// 'amount_of_parameters' can be zero


	if( amount_of_parameters==0 && !stack[index-1].function )
		Error( err_unexpected_final_bracket );


	bool was_sign = stack[index-1].sign;


	if( stack[index-1].function )
	{
		// the result of a function will be on 'stack[index-1]'
		// and then at the end we'll set the correct type (numerical value) of this element
		CallFunction(stack[index-1].function_name, amount_of_parameters, index);
	}
	else
	{
		/*
			there was a normal bracket (not a funcion)
		*/
		if( amount_of_parameters != 1 )
			Error( err_unexpected_semicolon_operator );


		/*
			in the place where is the bracket we put the result
		*/
		stack[index-1] = stack[index];
	}


	/*
		if there was a '-' character before the first bracket
		we change the sign of the expression
	*/
	stack[index-1].sign = false;

	if( was_sign )
		stack[index-1].value.ChangeSign();

	stack[index-1].type = Item::numerical_value;
	

	/*
		the pointer of the stack will be pointing on the next (non-existing now) element
	*/
	stack_index = index;
}


/*!
	this method is putting the operator on the stack
*/

void PushOperatorIntoStack(Item & temp)
{
	if( stack_index < stack.size() )
		stack[stack_index] = temp;
	else
		stack.push_back( temp );

	++stack_index;
}



/*!
	this method is reading a operator and if it's a final bracket
	it's calling RollingUpFinalBracket() and reading a operator again
*/
NxI32 ReadOperatorAndCheckFinalBracket(Item & temp)
{
	do
	{
		if( ReadOperator(temp) == 1 )
		{
			/*
				the string is finished
			*/
		return 1;
		}

		if( temp.type == Item::last_bracket )
			RollingUpFinalBracket();

	}
	while( temp.type == Item::last_bracket );

return 0;
}


/*!
	we check wheter there are only numerical value's or 'semicolon' operators on the stack
*/
void CheckIntegrityOfStack()
{
	for(NxU32 i=0 ; i<stack_index; ++i)
	{
		if( stack[i].type != Item::numerical_value &&
			stack[i].type != Item::semicolon)
		{
			/*
				on the stack we must only have 'numerical_value' or 'semicolon' operator
				if there is something another that means
				we probably didn't close any of the 'first' bracket
			*/
			Error( err_stack_not_clear );
		}
	}
}



/*!
	the main loop of parsing
*/
void Parse()
{
Item item;	
NxI32 result_code;


	while( true )
	{
		if( pstop_calculating && pstop_calculating->WasStopSignal() )
			Error( err_interrupt );

		result_code = ReadValueVariableOrFunctionAndPushItIntoStack( item );

		if( result_code == 0 )
		{
			if( item.type == Item::first_bracket )
				continue;
			
			result_code = ReadOperatorAndCheckFinalBracket( item );
		}
	
		
		if( result_code==1 || item.type==Item::semicolon )
		{
			/*
				the string is finished or the 'semicolon' operator has appeared
			*/

			if( stack_index == 0 )
				Error( err_nothing_has_read );
			
			TryRollingUpStack();

			if( result_code == 1 )
			{
				CheckIntegrityOfStack();

			return;
			}
		}			
	

		PushOperatorIntoStack( item );
		TryRollingUpStackWithOperatorPriority();
	}
}

/*!
	this method is called at the end of the parsing process

	on our stack we can have another value than 'numerical_values' for example
	when someone use the operator ';' in the global scope or there was an error during
	parsing and the parser hasn't finished its job

	if there was an error the stack is cleaned up now
	otherwise we resize stack and leave on it only 'numerical_value' items
*/
void NormalizeStack()
{
	if( error!=err_ok || stack_index==0 )
	{
		stack.clear();
		return;
	}
	
	
	/*
		'stack_index' tell us how many elements there are on the stack,
		we must resize the stack now because 'stack_index' is using only for parsing
		and stack has more (or equal) elements than value of 'stack_index'
	*/
	stack.resize( stack_index );

	for(uint i=stack_index-1 ; i!=uint(-1) ; --i)
	{
		if( stack[i].type != Item::numerical_value )
			stack.erase( stack.begin() + i );
	}
}


public:


/*!
	the default constructor
*/
Parser(): default_stack_size(100)
{
	pstop_calculating = 0;
	puser_variables = 0;
	puser_functions = 0;
	pfunction_local_variables = 0;
	base = 10;
	error = err_ok;
	factorial_max.SetZero();

	CreateFunctionsTable();
	CreateVariablesTable();
	CreateMathematicalOperatorsTable();
}


/*!
	the assignment operator
*/
Parser<ValueType> & operator=(const Parser<ValueType> & p)
{
	pstop_calculating = p.pstop_calculating;
	puser_variables   = p.puser_variables;
	puser_functions   = p.puser_functions;
	pfunction_local_variables = 0;
	base = p.base;
	error = err_ok;
	factorial_max = p.factorial_max;

	/*
		we don't have to call 'CreateFunctionsTable()' etc.
		we can only copy these tables
	*/
	functions_table   = p.functions_table;
	variables_table   = p.variables_table;
	operators_table   = p.operators_table;

	visited_variables = p.visited_variables;
	visited_functions = p.visited_functions;

return *this;
}


/*!
	the copying constructor
*/
Parser(const Parser<ValueType> & p): default_stack_size(p.default_stack_size)
{
	operator=(p);
}


/*!
	the new base of mathematic system		
*/
void SetBase(NxI32 b)
{
	if( b>=2 && b<=16 )
		base = b;
}


/*!
	this method sets a pointer to the object which tell us whether we should stop
	calculations
*/
void SetStopObject(const volatile StopCalculating * ps)
{
	pstop_calculating = ps;
}


/*!
	this method sets the new table of user-defined variables
	if you don't want any other variables just put zero value into the 'puser_variables' variable

	(you can have only one table at the same time)
*/
void SetVariables(const Objects * pv)
{
	puser_variables = pv;
}


/*!
	this method sets the new table of user-defined functions
	if you don't want any other functions just put zero value into the 'puser_functions' variable

	(you can have only one table at the same time)
*/
void SetFunctions(const Objects * pf)
{
	puser_functions = pf;
}


/*!
	you will not be allowed to calculate the factorial 
	if its argument is greater than 'm'
	there'll be: ErrorCode::err_too_big_factorial
	default 'factorial_max' is zero which means you can calculate what you want to
*/
void SetFactorialMax(const ValueType & m)
{
	factorial_max = m;
}


/*!
	the main method using for parsing string
*/
ErrorCode Parse(const char * str)
{
	stack_index  = 0;
	pstring      = str;
	error        = err_ok;
	
	stack.resize( default_stack_size );

	try
	{
		Parse();
	}
	catch(ErrorCode c)
	{
		error = c;
	}

	NormalizeStack();

return error;
}





};

} // namespace


#endif
