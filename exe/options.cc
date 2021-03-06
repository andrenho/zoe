#include "exe/options.hh"

#include <getopt.h>
#include <iostream>
using namespace std;

#ifdef DEBUG
#include "compiler/parser.hh"
#endif

Options::Options(int argc, char* argv[])
{
    //
    // load options using getopt_long(3)
    //
    while(1) {
        static struct option const long_options[] = {
            // these options are only avaliable when compiled in debugging mode
            { "trace",          no_argument, nullptr, 'T' },
#ifdef DEBUG
            { "debug-bison",    no_argument, nullptr, 'B' },
#endif
            { "disassemble",    no_argument, nullptr, 'D' },
            { "help",           no_argument, nullptr, 'h' },
            { "version",        no_argument, nullptr, 'v' },
            { nullptr, 0, nullptr, 0 },
        };

        int opt_idx = 0;
        static const char* opts = "hvTD"
#ifdef DEBUG
        "B"
#endif
        ;

        switch(getopt_long(argc, argv, opts, long_options, &opt_idx)) {
            case 'T':
                trace = true;
                break;
#ifdef DEBUG
            case 'B':
                yydebug = 1;
                break;
#endif
            case 'D':
                disassemble = true;
                break;
            case 'v':
                cout << "zoe " VERSION " - a programming language.\n";
                cout << "Avaliable under the LGPLv3 license. See COPYING file.\n";
                cout << "Written by André Wagner.\n";
                exit(EXIT_SUCCESS);
            case 'h':
                PrintHelp(cout, EXIT_SUCCESS);
                break;

            case 0:
                break;
            case -1:
                goto done;
            case '?':
                PrintHelp(cout, EXIT_FAILURE);
                break;
            default:
                abort();
        }
    }
done:
    if(optind < argc) {
        mode = OperationMode::NONINTERACTIVE;
        while(optind < argc) {
            scripts_filename.push_back(argv[optind++]);
        }
    }
}


void Options::PrintHelp(ostream& ss, int status) const
{
    ss << "Usage: zoe [OPTION]... [SCRIPT [ARGS]...]\n";
    ss << "Avaliable options are:\n";
#ifdef DEBUG
    ss << "   -B, --debug-bison     activate BISON debugger\n";
#endif
    ss << "   -D, --disassemble     disassemble when using REPL\n";
    ss << "   -T, --trace           trace assembly code execution\n";
    ss << "   -h, --help            display this help and exit\n";
    ss << "   -v, --version         show version and exit\n";
    exit(status);
}


// vim: ts=4:sw=4:sts=4:expandtab:foldmethod=marker:syntax=cpp
