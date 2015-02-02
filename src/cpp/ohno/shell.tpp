#ifndef OHNO_SHELL_TPP_INCLUDED
#define OHNO_SHELL_TPP_INCLUDED

#include "shell.h"

namespace ohno {

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
OhNoShellManager<System, SchemeT, AllocatorT>::OhNoShellManager( size_t ncells )
    : Base( ncells )
{
    this->_def_macro( '@', []( Allocator& alloc, elem_t tail, std::vector<std::string>& names ) -> elem_t {
        auto_root<elem_t> e1( alloc, tail.pcell("% Requires at least a 3 element list")->head() );
        auto_root<elem_t> e2( alloc, tail.pcell()->tail().pcell("% Requires at least a 3 element list")->head() );
        auto_root<elem_t> e3( alloc, tail.pcell()->tail().pcell()->tail().pcell("% Requires at least a 3 element list")->head() );

        const size_t e1_count = _count_undef( e1 );
        const size_t e2_count = _count_undef( e2 );
        const size_t e3_count = _count_undef( e3 );

        std::vector<std::string> e1_names;
        for( size_t i = 0; i < e1_count; ++i )
        {
            e1_names.push_back( names.back() );
            names.pop_back();
        }

        std::vector<std::string> e2_names;
        for( size_t i = 0; i < e2_count; ++i )
        {
            e2_names.push_back( names.back() );
            names.pop_back();
        }

        std::vector<std::string> e3_names;
        for( size_t i = 0; i < e3_count; ++i )
        {
            e3_names.push_back( names.back() );
            names.pop_back();
        }

        for( size_t i = 0; i < e1_count; ++i )
        {
            names.push_back( e1_names.back() );
            e1_names.pop_back();
        }

        for( size_t i = 0; i < e3_count; ++i )
        {
            names.push_back( e3_names.back() );
            e3_names.pop_back();
        }

        for( size_t i = 0; i < e2_count; ++i )
        {
            names.push_back( e2_names.back() );
            e2_names.pop_back();
        }

        elem_t r = tail.pcell()->tail().pcell()->tail().pcell()->tail();
        r = alloc.new_Cell( e1, r );
        r = alloc.new_Cell( e3, r );
        return alloc.new_Cell( e2, r );
    } );
}

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
auto OhNoShellManager<System, SchemeT, AllocatorT>::_count_undef( elem_t e ) -> size_t
{
    if( e.is_undef() )
        return 1;
    else if( e.is_pcell() )
    {
        if( e.pcell()->tail().is_def() )
            return _count_undef( e.pcell()->head() ) + _count_undef( e.pcell()->tail() );
        else
            return _count_undef( e.pcell()->head() );
    }

    return 0;
}

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
void OhNoShellManager<System, SchemeT, AllocatorT>::print_help( elpa_ostream<System,SchemeT>& eos )
{
    Base::print_help( eos );
    eos << "% - Swap last two elements of a list\n";
}

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
auto OhNoShellManager<System, SchemeT, AllocatorT>::process_command( const std::string& cmd, elpa_istream<System, SchemeT, AllocatorT>& eis, elpa_ostream<System, SchemeT>& eos, bool noisy ) -> bool
{
    if( cmd == "fn" )
    {

    }
    else
        return Base::process_command( cmd, eis, eos, noisy );

    return true;
}

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
void OhNoShellManager<System, SchemeT, AllocatorT>::print_commands( elpa_ostream<System, SchemeT>& eos ) const
{
    Base::print_commands( eos );
    eos << ":fn   - Create an oh-no function\n";
}

}   //namespace

#endif
