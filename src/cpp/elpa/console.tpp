#ifndef ELPA_CONSOLE_TPP_INCLUDED
#define ELPA_CONSOLE_TPP_INCLUDED

#include "console.h"
#include "tests.h"
#include "runtime.h"
#include "shell.h"
#include "container/range.h"

namespace elpa {

template<MetaInterpreter class InterpreterT>
auto Console<InterpreterT>::help( int ret_code ) -> int
{
    std::cout << "usage: kcon [option] <filenames>\n";
    std::cout << "Options are:\n";
    std::cout << "-mem=n      : Limit memory usage to only n cells\n";
    std::cout << "-fast       : Turn off illegal operation checks and assertions\n";
    std::cout << "-safe       : Perfrom illegal operation checks only\n";
    std::cout << "-debug      : Perfrom illegal operation checks and assertions\n";
    std::cout << "-superdebug : Likedebug but memory allocator shifts cell on each allocation\n";
    std::cout << "-noisy      : Show all output\n";
    std::cout << "-repl       : Enter REPL after running all scripts\n";
    std::cout << "-tests      : Run internal tests before starting\n";
    std::cout << "-help       : Show this help\n";
    std::cout << std::endl;
    return ret_code;
}

template<MetaInterpreter class InterpreterT>
template<class Env>
auto Console<InterpreterT>::run( size_t ncells, const std::vector< std::string >& filenames, bool noisy, bool repl ) -> int
{
    Shell< Env > shell( ncells );

    typename Env::elpa_ostream eos( std::cout );

    for( auto filename : filenames )
        if( noisy )
            shell.process( filename, eos );
        else
            shell.process( filename );

    if( repl )
        shell.interactive( std::cin, std::cout );
    else if( !noisy  )
        shell.print( eos, shell.it() );

    return 0;
}

template<MetaInterpreter class InterpreterT>
auto Console<InterpreterT>::go( int argc, const char* argv[] ) -> int
{
    bool testing = false;
    size_t ncells = 0;
    std::string mode = "";
    bool noisy = false;
    bool repl = false;
    std::vector< std::string > filenames;

    for( const std::string arg : container::range( argv + 1, argv + argc ) )
    {
        if( arg.length() > 1 && arg[0] == '-' )
        {
            if( arg == "-tests" )
                testing = true;
            else if( arg.substr(0,5) == "-mem=" )
            {
                for( auto c : arg.substr(5) )
                {
                    if( !isdigit(c) )
                    {
                        std::cerr << "Invalid option: " << arg << std::endl;
                        return help( 1 );
                    }

                    ncells = 10 * ncells + (c - '0');
                }
            }
            else if( arg == "-superdebug" || arg == "-debug" || arg == "-safe" || arg == "-fast" )
                mode = arg;
            else if( arg == "-repl" )
                repl = true;
            else if( arg == "-noisy" )
                noisy = true;
            else if( arg == "-help" || arg == "--help" )
                return help( 0 );
            else
            {
                std::cerr << "Invalid option: " << arg << std::endl;
                return help( 1 );
            }
        }
        else
        {
            filenames.push_back( arg );
        }
    }

    if( ncells == 0 )
        ncells = 8 * 1024 * 1024;

    if( mode == "" )
        mode = "-debug";

    if( filenames.size() == 0 )
        repl = true;

    if( testing )
    {
        try
        {
            elpa::run_tests();
            this->run_tests();
            std::cout << "\n** All tests passed **\n";
        }
        catch( const Error<Test>& e )
        {
            std::cerr << "Test failed: " + e.message() << std::endl;
            return 2;
        }
        catch( const Error<Assertion>& e )
        {
            std::cerr << "Assertion failed: " + e.message() << std::endl;
            return 2;
        }
        catch( const std::exception& e )
        {
            std::cerr << e.what() << std::endl;
            return 2;
        }
        catch( ... )
        {
            std::cerr << "Unknown exception" << std::endl;
            return 2;
        }
    }

    try
    {
        if( mode == "-superdebug" )
            return run< Env<Debug, SimpleScheme, TestAllocator, InterpreterT> >( ncells, filenames, noisy, repl );
        else if( mode == "-debug" )
            return run< Env<Debug, SimpleScheme, SimpleAllocator, InterpreterT> >( ncells, filenames, noisy, repl );
        else if( mode == "-safe" )
            return run< Env<Safe, OptScheme, OptAllocator, InterpreterT> >( ncells, filenames, noisy, repl );
        else if( mode == "-fast" )
            return run< Env<Fast, OptScheme, OptAllocator, InterpreterT> >( ncells, filenames, noisy, repl );
    }
    catch( const Error<Runtime,OutOfMemory>& )
    {
        std::cerr << "Out of memory" << std::endl;
        return 1;
    }

    std::cerr << "Unknown mode: "s + mode;
    return 1;
}

}   //namespace

#endif
