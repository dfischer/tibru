#ifndef KCON_TYPES_H
#define KCON_TYPES_H

#include <cstdint>
#include "common.h"

template<class T> struct Tag;

typedef uintptr_t slot_t;
const slot_t TAG_MASK = 2 * sizeof(slot_t) - 1;
const slot_t ADDR_MASK = ~TAG_MASK;
const slot_t TAIL_MASK = 1;
const slot_t HEAD_MASK = 2;
const slot_t TYPE_MASK = HEAD_MASK | TAIL_MASK;
//const slot_t MARK_ADDR_BIT = 1 << 2;
//const slot_t MARK_BYTE_BIT = 1 << 8;
//const slot_t MARK_MASK = MARK_BYTE_BIT | MARK_ADDR_BIT;

typedef uint8_t byte_t;

ASSERT( sizeof(slot_t) == sizeof(void*) );

template<class H, class T>
struct Cell
{
	const H head;
	const T tail;

	static const short TYPECODE = (Tag<H>::CODE << 1) | Tag<T>::CODE;

	ASSERT( sizeof(H) == sizeof(slot_t) );
	ASSERT( sizeof(T) == sizeof(slot_t) );
};

class pcell_t
{
	uintptr_t _addr_and_type;

	pcell_t()
		: _addr_and_type( 0 ) {}
public:
	template<class H, class T>
	pcell_t( const Cell<H,T>* pcell )
		: _addr_and_type( reinterpret_cast<uintptr_t>( pcell ) | (Tag<H>::CODE << 1) | Tag<T>::CODE ) {}

	static pcell_t null() { return pcell_t(); }

	bool is_null() const { return _addr_and_type == 0; }

	short typecode() const
	{
		return _addr_and_type & TYPE_MASK;
	}

	short headcode() const
	{
		return _addr_and_type & HEAD_MASK;
	}

	short tailcode() const
	{
		return _addr_and_type & TAIL_MASK;
	}

	template<class H, class T>
	const Cell<H,T>* cast() const
	{
		assert( typecode() == Cell<H,T>::TYPECODE, "Invalid cast" );
		return reinterpret_cast<const Cell<H,T>*>( _addr_and_type & ADDR_MASK );
	}

	void dispatch( struct CellVisitor& visitor );
};

typedef uintptr_t value_t;

ASSERT( sizeof(pcell_t) == sizeof(slot_t) );
ASSERT( sizeof(value_t) == sizeof(slot_t) );

template<> struct Tag<pcell_t> { enum { CODE = 0 }; };
template<> struct Tag<value_t> { enum { CODE = 1 }; };

class elem_t
{
    bool _is_cell;
	union
	{
		value_t _value;
		pcell_t _pcell;
	};
public:
	elem_t( value_t v )
		: _is_cell( false ), _value( v ) {}

	elem_t( pcell_t p=pcell_t::null() )
		: _is_cell( true ), _pcell( p ) {}

	template<class H,class T>
	elem_t( const Cell<H,T>* p )
		: _is_cell( true ), _pcell( p ) {}

    pcell_t pcell() const { return _pcell; }
    byte_t byte_value() const { return _value; }

	bool is_cell() const { return _is_cell; }
	bool is_byte() const { return !_is_cell; }
	bool is_null() const { return is_cell() && pcell().is_null(); }
};

inline elem_t head( pcell_t pcell )
{
	switch( pcell.typecode() )
	{
		case Cell<pcell_t,pcell_t>::TYPECODE:
			return pcell.cast<pcell_t,pcell_t>()->head;
		case Cell<pcell_t,value_t>::TYPECODE:
			return pcell.cast<pcell_t,value_t>()->head;
		case Cell<value_t,pcell_t>::TYPECODE:
			return pcell.cast<value_t,pcell_t>()->head;
		case Cell<value_t,value_t>::TYPECODE:
			return pcell.cast<value_t,value_t>()->head;
		default:
			throw Error<Runtime>( "head dispatch failed" );
	}
}

inline elem_t tail( pcell_t pcell )
{
	switch( pcell.typecode() )
	{
		case Cell<pcell_t,pcell_t>::TYPECODE:
			return pcell.cast<pcell_t,pcell_t>()->tail;
		case Cell<pcell_t,value_t>::TYPECODE:
			return pcell.cast<pcell_t,value_t>()->tail;
		case Cell<value_t,pcell_t>::TYPECODE:
			return pcell.cast<value_t,pcell_t>()->tail;
		case Cell<value_t,value_t>::TYPECODE:
			return pcell.cast<value_t,value_t>()->tail;
		default:
			throw Error<Runtime>( "head dispatch failed" );
	}
}

inline bool is_singleton( pcell_t p )
{
    return !p.is_null() && tail( p ).is_null();
}

#endif
