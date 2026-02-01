/*
 * Copyright (c) 2019 -2021 MINRES Technolgies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CLIParser.h"
#include <iss/log_categories.h>

#include <scc/configurable_tracer.h>
#include <scc/configurer.h>
#include <scc/hierarchy_dumper.h>
#include <scc/perf_estimator.h>
#include <scc/report.h>
#include <scc/scv/scv_tr_db.h>
#include <scc/tracer.h>
#ifdef WITH_LLVM
#include <iss/llvm/jit_helper.h>
#endif

#include "vp/tb.h"
#include <boost/program_options.hpp>
#include <csetjmp>
#include <csignal>
#include <fstream>
#include <iostream>
#ifdef ERROR
#undef ERROR
#endif

const std::string core_path{"tb.top.core_complex"};

using namespace sysc;
using namespace sc_core;

namespace {
const size_t ERRORR_IN_COMMAND_LINE = 1;
const size_t SUCCESS = 0;
} // namespace

jmp_buf abrt;
void ABRThandler(int sig) { longjmp(abrt, sig); }

int sc_main(int argc, char* argv[]) {
    signal(SIGINT, ABRThandler);
    signal(SIGABRT, ABRThandler);
    signal(SIGSEGV, ABRThandler);
    signal(SIGTERM, ABRThandler);
    ///////////////////////////////////////////////////////////////////////////
    // SystemC >=2.2 got picky about multiple drivers so disable check
    ///////////////////////////////////////////////////////////////////////////
    sc_report_handler::set_actions(SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_, SC_DO_NOTHING);
    ///////////////////////////////////////////////////////////////////////////
    // CLI argument parsing & logging setup
    ///////////////////////////////////////////////////////////////////////////
    CLIParser parser(argc, argv);
    if(!parser.is_valid())
        return ERRORR_IN_COMMAND_LINE;
    scc::stream_redirection cout_redir(std::cout, scc::log::INFO);
    scc::stream_redirection cerr_redir(std::cerr, scc::log::ERROR);
    ///////////////////////////////////////////////////////////////////////////
    // set up infrastructure
    ///////////////////////////////////////////////////////////////////////////
#ifdef WITH_LLVM
    iss::init_jit_debug(argc, argv);
#endif
    ///////////////////////////////////////////////////////////////////////////
    // create the performance estimation module
    ///////////////////////////////////////////////////////////////////////////
    scc::perf_estimator estimator;
    ///////////////////////////////////////////////////////////////////////////
    // set up configuration
    ///////////////////////////////////////////////////////////////////////////
    scc::configurer cfg(parser.get<std::string>("config-file"));
    ///////////////////////////////////////////////////////////////////////////
    // process CLI paramter settings
    ///////////////////////////////////////////////////////////////////////////
    if(parser.is_set("parameter"))
        for(auto& p : parser.get<std::vector<std::string>>("parameter")) {
            auto token = util::split(p, '=');
            if(token.size() == 2)
                cfg.set_value_from_str(token[0], token[1]);
            else
                SCCERR() << "Invalid parameter specification '" << p << "', should be '<param name>=<param_value>'";
        }
    ///////////////////////////////////////////////////////////////////////////
    // rocess CLI switche and set/overwrite config from command line settings
    ///////////////////////////////////////////////////////////////////////////
    cfg.set_value(core_path + ".gdb_server_port", parser.get<unsigned short>("gdb-port"));
    cfg.set_value(core_path + ".dump_ir", parser.is_set("dump-ir"));
    cfg.set_value(core_path + ".backend", parser.get<std::string>("backend"));
    cfg.set_value(core_path + ".core_type", parser.get<std::string>("isa"));
    if(parser.is_set("plugin")) {
        auto plugins = util::join(parser.get<std::vector<std::string>>("plugin"), ",");
        cfg.set_value(core_path + ".plugins", plugins);
    }
    if(parser.is_set("elf"))
        cfg.set_value(core_path + ".elf_file", parser.get<std::string>("elf"));
    if(parser.is_set("quantum"))
        tlm::tlm_global_quantum::instance().set(sc_core::sc_time(parser.get<unsigned>("quantum"), sc_core::SC_NS));
    if(parser.is_set("reset")) {
        auto str = parser.get<std::string>("reset");
        uint64_t start_address = str.find("0x") == 0 ? std::stoull(str.substr(2), nullptr, 16) : std::stoull(str, nullptr, 10);
        cfg.set_value(core_path + ".reset_address", start_address);
    }
    if(parser.is_set("disass")) {
        cfg.set_value(core_path + ".enable_disass", true);
        auto file_name = parser.get<std::string>("disass");
        if(file_name.length() > 0) {
            LOG_OUTPUT(disass)::stream() = fopen(file_name.c_str(), "w");
            LOGGER(disass)::print_time() = false;
            LOGGER(disass)::print_severity() = false;
        }
    }
    ///////////////////////////////////////////////////////////////////////////
    // set up tracing & transaction recording
    ///////////////////////////////////////////////////////////////////////////
    std::unique_ptr<scc::configurable_tracer> tracer;
    if(auto trace_level = parser.get<unsigned>("trace-level")) {
        auto file_name = parser.get<std::string>("trace-file");
        auto trace_default_on = parser.is_set("trace-default-on");
        auto enable_tx_trace = static_cast<bool>(trace_level & 0x2);
        cfg.set_value("scc_tracer.default_trace_enable", !parser.is_set("trace-default-off"));
        cfg.set_value("scc_tracer.tx_trace_type", static_cast<unsigned>(scc::tracer::file_type::FTR));
        cfg.set_value("scc_tracer.sig_trace_type", static_cast<unsigned>(scc::tracer::file_type::FST));
        tracer = scc::make_unique<scc::configurable_tracer>(file_name, enable_tx_trace, static_cast<bool>(trace_level & 0x1));
        if(enable_tx_trace)
            cfg.set_value(core_path + ".enable_instr_trace", true);
    }
    ///////////////////////////////////////////////////////////////////////////
    // instantiate top level
    ///////////////////////////////////////////////////////////////////////////
    auto i_system = scc::make_unique<vp::tb>("tb");
    ///////////////////////////////////////////////////////////////////////////
    // dump configuration and/or hierarchical structure if requested
    ///////////////////////////////////////////////////////////////////////////
    if(parser.get<std::string>("dump-config").size() > 0) {
        std::ofstream of{parser.get<std::string>("dump-config")};
        if(of.is_open())
            cfg.dump_configuration(of, true);
    }
    cfg.configure();
    std::unique_ptr<scc::hierarchy_dumper> dumper;
    if(parser.is_set("dump-structure"))
        dumper.reset(new scc::hierarchy_dumper(parser.get<std::string>("dump-structure"), scc::hierarchy_dumper::D3JSON));
    ///////////////////////////////////////////////////////////////////////////
    // run simulation
    ///////////////////////////////////////////////////////////////////////////
    if(auto res = setjmp(abrt)) {
        switch(res) {
            case SIGHUP:
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
            case SIGUSR1:
            case SIGUSR2:
                SCCINFO() << "Simulation stopped with signal " << res << ".";
                break;
            default:
                SCCERR() << "Simulation aborted with signal " << res << "!";
        }
    } else {
        try {
            if(parser.is_set("max_time")) {
                sc_core::sc_start(scc::parse_from_string(parser.get<std::string>("max_time")));
            } else
                sc_core::sc_start();
            if(sc_core::sc_start_of_simulation_invoked() && !sc_core::sc_end_of_simulation_invoked())
                sc_core::sc_stop();
        } catch(sc_core::sc_report& rep) {
            sc_core::sc_report_handler::get_handler()(rep, sc_core::SC_DISPLAY | sc_core::SC_STOP);
        }
    }
    return 0;
}
