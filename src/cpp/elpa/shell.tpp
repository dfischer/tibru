#ifndef ELPA_SHELL_TPP
#define ELPA_SHELL_TPP

#include "shell.h"
#include "runtime.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <fstream>
#include <algorithm>

namespace elpa {

template<class Env>
auto Shell<Env>::_process_command( const std::string& cmd, elpa_istream& eis, elpa_ostream& eos, bool noisy ) -> bool
{
	if( cmd == "def" )
    {
    	std::string name;
    	elem_t elem;
    	eis >> name >> elem >> endofline;

    	if( _manager.constants().find( name ) != _manager.constants().end() )
            throw Error<Runtime>( "Can't redefine constant '"s + name + "'" );

    	if( elem.is_byte() )
    	{
    	    uint8_t val = Scheme::byte_value( elem.byte() );

    	    if( _defns_byte_counts[val] < Scheme::MAX_TAG )
    	    {
                _defns_byte_counts[val]++;
                elem = Scheme::new_byte( val, _defns_byte_counts[val] );

                _defns_no_it[name] = elem;
    	    }
            else
            {
                elem = Scheme::new_byte( val );
            }
    	}
        else
        {
            _defns_no_it[name] = elem;
        }

        _defns_with_it[name] = elem;

    	return true;
    }
    else if( cmd == "process" )
    {
    	std::string filename;
    	eis >> filename >> endofline;
    	process( filename, eos );
    	return true;
    }
    else if( cmd == "include" )
    {
    	std::string filename;
    	eis >> filename >> endofline;
    	process( filename );
    	return true;
    }
    else if( cmd == "names" )
    {
        std::string status;
        eis >> status >> endofline;
        if( status != "on" && status != "off" )
            throw Error<Command>( "Name command requires 'on' or 'off' argument" );

        _use_names = (status == "on");
        return true;
    }
    else if( _manager.process_command( cmd, eis, eos, noisy ) )
    {
        return true;
    }

    eis >> endofline;

    if( cmd == "quit" || cmd == "exit" )
        return false;
    else if( cmd == "dec" )
        _num_format = std::dec;
    else if( cmd == "hex" )
        _num_format = std::hex;
    else if( cmd == "flat" )
        _format = flat;
    else if( cmd == "deep" )
        _format = deep;
    else if( cmd == "line" )
        _line_format = true;
    else if( cmd == "list" )
        _line_format = false;
    else if( cmd == "defs" )
    {
    	if( noisy )
    		for( auto defn : _defns_with_it )
    			eos << defn.first << std::endl;
    }
    else if( cmd == "sys" )
    {
    	if( noisy )
    	{
    		eos << "System: " << Env::System::name() << std::endl;
    		eos << "Scheme: " << Env::Scheme::name() << std::endl;
    		eos << "Allocator: " << Env::Allocator::name() << std::endl;
    		uint64_t nalloc = _manager.interpreter().allocator().num_allocated();
    		uint64_t ntotal = _manager.interpreter().allocator().num_total();
    		uint64_t nmax = _manager.interpreter().allocator().num_max();
    		eos << "Mem alloc: " << nalloc << " cells";
    		if( ntotal > 0 )
    			eos << " (" << (nalloc * 100) / ntotal << "%)";
    		eos << std::endl;
    		eos << "Mem total: " << nalloc << " cells" << std::endl;
    		eos << "Mem max: " << nmax << " cells" << std::endl;
    		eos << "GC count: " << _manager.interpreter().allocator().gc_count() << std::endl;
    	}
    }
    else if( cmd == "gc" )
    	_manager.interpreter().allocator().gc();
    else if( cmd == "help" )
    {
        if( noisy )
        {
            eos << "Evaluate an expression of the form <name>|byte|[<expr> <expr>+]|<reader><expr>|<expr><macro> and define 'it' as its value.\n";
            eos << "Or process an operator on an expression of the form <op><expr> and define 'it' as its value.\n";
            eos << "Or run a command.\n";

            eos << "\nCommands:\n";
            eos << ":def <name> <expr> - Define a named expression\n";
            eos << ":process <filename> - Process all statements in the specified file\n";
            eos << ":include <filename> - Silently process all statements in the specified file\n";

            eos << ":dec  - Show bytes in decimal notation (default)\n";
            eos << ":hex  - Show bytes in hex notation\n";
            eos << ":flat - Show cells as flattened right associative lists (default)\n";
            eos << ":deep - Show cells as pairs\n";
            eos << ":line - Show expressions on a single line (default)\n";
            eos << ":list - Show expressions over multiple lines in list format\n";
            eos << ":names [on|off] - Show named expressions as defined names (works best in debug modes)\n";
            eos << ":defs - Show all defined names\n";
            eos << ":sys  - Show information about the system\n";
            eos << ":gc   - Run the garbage collector\n";
            eos << ":help - Show this help\n";
            eos << ":exit - End the shell session\n";
            eos << ":quit - End the shell session\n";
            _manager.print_commands( eos );

            eos << "\nOperators:\n";
    		for( auto op : _manager.operators() )
                eos << op << std::endl;

            _manager.print_help( eos );
        }
    }
    else
        throw Error<Command>( "Unknown command '"s + cmd + "'" );

    return true;
}

template<class Env>
void Shell<Env>::print( elpa_ostream& eos, elem_t elem )
{
    eos << _format << _num_format;
    if( !_line_format )
    {
        int indent = 0;
        for( ; elem.is_pcell(); elem = elem.pcell()->tail() )
        {
            for( int i = 0; i < indent; ++i ) eos << ' ';
            ++indent;

            eos << elem.pcell()->head() << std::endl;
        }

        for( int i = 0; i < indent; ++i ) eos << ' ';
    }

    if( _use_names )
        eos << std::make_pair( &_defns_no_it, elem ) << std::endl;
    else
        eos << elem << std::endl;
}

template<class Env>
auto Shell<Env>::_process_input( std::istream& is, elpa_ostream& eos, bool noisy ) -> bool
{
    elpa_istream eis( is, _manager.interpreter().allocator(), _defns_with_it, _manager.readers(), _manager.macros() );

    char c;
    if( eis >> c )
    {
        if( c == '%' )
        {
            while( c != '\n' )
                eis.get( c  );
        }
        else if( c == ':' )
        {
            std::string cmd;
            if( !(eis >> std::noskipws >> cmd >> std::skipws) )
                throw Error<Syntax>( "Expected command after ':'" );

            return _process_command( cmd, eis, eos, noisy );
        }
        else
        {
            const Operators& ops = _manager.operators();
            if( std::find_if( ops.begin(), ops.end(), [c]( const typename Operators::value_type& v ){ return v[0] == c; } ) == ops.end() )
            {
                eis.putback( c );
                c = '\0';
            }

            try
            {
                elem_t elem;
                eis >> elem >> endofline;

                if( c != '\0' )
                {
                    bool more = true;
                    for( size_t niter = 0; more; ++niter )
                    {
                        elem = _manager.process_operator( c, elem, niter, more );

                        if( noisy )
                        {
                            if( more )
                                eos << (niter+1) << ".\t";

                            print( eos, elem );

                            if( !more && niter > 0 )
                                eos << std::endl;
                        }
                    }
                }
                else
                    if( noisy )
                        print( eos, elem );

                _defns_with_it["it"] = elem;

                return true;
            }
            catch( const Error<Syntax,EOS>& )
            {
                throw MoreToRead();
            }
        }
    }

    return true;
}

template<class Env>
void Shell<Env>::interactive( std::istream& in, std::ostream& out )
{
	elpa_ostream eos( out );

    bool keep_processing = true;
    while( keep_processing )
    {
        try
        {
            std::string input;

            std::string prompt = ">>> ";
            while( true )
            {
                try
                {
                    out << prompt;
                    std::string line;
                    std::getline( in, line );
                    input += (line + "\n"s);
                    prompt = "... ";

					std::istringstream iss( input );
                    keep_processing = _process_input( iss, eos );
                    break;
                }
                catch( const MoreToRead& )
                {
                }
            }
        }
        catch( const Error<Syntax>& e )
        {
            out << "Syntax: " << e.message() << std::endl;
            out << "Run :help for more details" << std::endl;
        }
        catch( const Error<IllegalOp>& e )
        {
            out << "IllegalOp: " << e.message() << std::endl;
            out << "Run :help for more details" << std::endl;
        }
        catch( const Error<Command>& e )
        {
            out << "Command: " << e.message() << std::endl;
            out << "Run :help for more details" << std::endl;
        }
        catch( const Error<Runtime>& e )
        {
            out << "Runtime: " << e.message() << std::endl;
            out << "Run :help for more details" << std::endl;
        }
    }
}


template<class Env>
auto Shell<Env>::process( std::istream& in, elpa_ostream& eos ) -> elem_t
{
	while( in )
		_process_input( in, eos, true );

	return _defns_with_it.find("it") != _defns_with_it.end() ? _defns_with_it["it"] : elem_t();
}

template<class Env>
auto Shell<Env>::process( std::istream& in ) -> elem_t
{
	std::ostringstream oss;
	elpa_ostream eos( oss );
	while( in )
		_process_input( in, eos, false );
	System::assert( oss.str() == "", "Processing produced output" );
	return _defns_with_it.find("it") != _defns_with_it.end() ? _defns_with_it["it"] : elem_t();
}

template<class Env>
auto Shell<Env>::parse( const std::string& in ) -> elem_t
{
	std::istringstream iss( in );
	return process( iss );
}

template<class Env>
void Shell<Env>::process( const std::string& filename, elpa_ostream& eos )
{
	std::ifstream ifs( filename );
	if( !ifs )
		throw Error<Runtime>( "File not found '"s + filename + "'" );

	if( !_processing.insert( filename ).second )
		throw Error<Runtime>( "File included recursively '"s + filename + "'" );

    try
    {
        process( ifs, eos );
    }
    catch( ... )
    {
        _processing.erase( filename );
        throw;
    }

	System::assert( _processing.erase( filename ) == 1, "Failed to register processed file" );
}

template<class Env>
void Shell<Env>::process( const std::string& filename )
{
	std::ifstream ifs( filename );
	if( !ifs )
		throw Error<Runtime>( "File not found '"s + filename + "'" );

	if( !_processing.insert( filename ).second )
		throw Error<Runtime>( "File included recursively '"s + filename + "'" );

	process( ifs );

	System::assert( _processing.erase( filename ) == 1, "Failed to register processed file" );
}

}   //namespace

#endif
