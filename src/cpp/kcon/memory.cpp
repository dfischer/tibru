#include "memory.h"
#include "runtime.h"

using namespace kcon;

/** TestAllocator */

template<class Scheme>
void TestAllocator<Scheme>::gc( const Roots& roots )
{
    ++_gc_count;

    std::set<pcell_t> all;
    all.swap( _allocated );

    for( auto r : roots )
        _mark( _allocated, *r );

    for( auto p : all )
        if( _allocated.find( p ) == _allocated.end() )
            delete p;

    if( _allocated.size() == _ncells )
        throw Error<Runtime,OutOfMemory>( "Out of memory" );
}

template<class Scheme>
void TestAllocator<Scheme>::_mark( std::set<pcell_t>& live, pcell_t p )
{
    if( live.find( p ) == live.end() )
    {
        live.insert( p );

        if( p->head().is_pcell() )
            _mark( live, p->head().pcell() );

        if( p->tail().is_pcell() )
            _mark( live, p->tail().pcell() );
    }
}

/** SimpleAllocator */

template<class Scheme>
void SimpleAllocator<Scheme>::_mark( std::set<pcell_t>& live, pcell_t p )
{
    if( live.find( p ) == live.end() )
    {
        live.insert( p );

        if( p->head().is_pcell() )
            _mark( live, p->head().pcell() );

        if( p->tail().is_pcell() )
            _mark( live, p->tail().pcell() );
    }
}

template<class Scheme>
void SimpleAllocator<Scheme>::gc( const Roots& roots )
{
    ++_gc_count;

    std::set<pcell_t> live;
    for( auto r : roots )
        _mark( live, *r );

    _free_list = 0;
    for( size_t i = 0; i != _ncells; ++i )
    {
        auto p = reinterpret_cast<Cell*>( &_page[i] );
        if( live.find( p ) == live.end() )
            _free_list = new (p) FreeCell{ _free_list, 0 };
    }

    if( _free_list == 0 )
        throw Error<Runtime,OutOfMemory>( "Out of memory" );
}

template class TestAllocator<SimpleScheme>;
template class SimpleAllocator<SimpleScheme>;
