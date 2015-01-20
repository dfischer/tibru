#ifndef ELPA_SHELL_H
#define ELPA_SHELL_H

#include "memory.h"
#include "stream.h"
#include "interpreter.h"
#include "container/elpa_map.h"
#include <iostream>
#include <iomanip>
#include <set>

namespace elpa {

struct Command;

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT, MetaInterpreter class InterpreterT>
class ShellManagerBase
{
protected:
    typedef InterpreterT<System,SchemeT,AllocatorT> Interpreter;

    Interpreter _interpreter;

    ShellManagerBase( size_t ncells )
        : _interpreter( ncells ) {}
public:
    typedef std::vector<std::string> Operators;

    Interpreter& interpreter() { return _interpreter; }

    virtual auto process_command( const std::string& cmd, elpa_istream<System, SchemeT, AllocatorT>& eis, elpa_ostream<System, SchemeT>& eos, bool noisy ) -> bool { return false; }
    virtual void print_commands( elpa_ostream<System, SchemeT>& eos ) const {}
};

template<class System, MetaScheme class SchemeT, MetaAllocator class AllocatorT>
class NullShellManager : public ShellManagerBase<System, SchemeT, AllocatorT, NullInterpreter>
{
public:
    typedef SchemeT<System> Scheme;
    typedef typename elpa_istream<System, SchemeT, AllocatorT>::Readers Readers;
    typedef typename elpa_istream<System, SchemeT, AllocatorT>::Macros Macros;
    typedef typename ShellManagerBase<System, SchemeT, AllocatorT, NullInterpreter>::Operators Operators;
    typedef typename Scheme::elem_t elem_t;

    NullShellManager( size_t ncells )
        : ShellManagerBase<System, SchemeT, AllocatorT, NullInterpreter>( ncells ) {}

    static auto readers() -> const Readers& { static Readers readers; return readers; }
    static auto macros() -> const Macros& { static Macros macros; return macros; }
    static auto operators() -> const Operators& { static const Operators ops; return ops; }
    static void print_help( elpa_ostream<System,SchemeT>& eos ) {}

    auto process_operator( char op, elem_t elem, size_t niter, bool& more ) -> elem_t { more = false; return elem; }
};

template<class Env>
struct Shell
{
	typedef typename Env::System System;
	typedef typename Env::Scheme Scheme;
    typedef typename Env::Interpreter::ShellManager ShellManager;
    typedef typename Env::elpa_istream elpa_istream;
    typedef typename Env::elpa_ostream elpa_ostream;
    typedef typename Env::elem_t elem_t;
    typedef typename elpa_ostream::ElpaManip ElpaManip;
    typedef typename elpa_ostream::Manip Manip;
    typedef typename elpa_ostream::BaseManip BaseManip;
    typedef typename ShellManager::Operators Operators;

    template<class K, class V>
    using elpa_map = typename Env::template elpa_map<K, V>;
private:
    ElpaManip _format;
    BaseManip _num_format;
    bool _line_format;
    bool _use_names;

    ShellManager _manager;
    elpa_map<std::string, elem_t> _defns_no_it;
    elpa_map<std::string, elem_t> _defns_with_it;
    elpa_map<std::string, elem_t> _defns_none;
    std::map<uint8_t, uint8_t> _defns_byte_counts;
    std::set<std::string> _processing;

    void _print( elpa_ostream& eos, elem_t elem );
    auto _process_command( const std::string& cmd, elpa_istream& eis, elpa_ostream& eos, bool noisy ) -> bool;
    auto _process_input( std::istream& is, elpa_ostream& eos, bool noisy=true ) -> bool;
public:
    struct MoreToRead {};

    Shell( size_t ncells )
        : _format( flat ), _num_format( std::dec ), _line_format( true ), _use_names( false ), _manager( ncells ),
          _defns_no_it( _manager.interpreter().allocator() ), _defns_with_it( _manager.interpreter().allocator() ), _defns_none( _manager.interpreter().allocator() ) {}

    void interactive( std::istream& in, std::ostream& out );
    auto process( std::istream& in, elpa_ostream& eos ) -> elem_t;
	auto process( std::istream& in ) -> elem_t;
	void process( const std::string& filename, elpa_ostream& eos );
	void process( const std::string& filename );
	auto parse( const std::string& in ) -> elem_t;

	const elpa_map<std::string, elem_t>& names() const
	{
	    return _use_names ? _defns_no_it : _defns_none;
    }
};

}   //namespace

#endif
