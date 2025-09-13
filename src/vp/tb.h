/*
 * Copyright (c) 2019 -2021 MINRES Technolgies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_VP_TB_H_
#define SRC_VP_TB_H_

#include "system.h"
#include <generic/rst_gen.h>
#include <generic/spi_mem.h>
#include <generic/terminal.h>
#include <systemc>

namespace vp {

class tb : public sc_core::sc_module {
public:
    tb(sc_core::sc_module_name const& nm);
    vp::system top{"top"};
    vpvper::generic::rst_gen rst_gen{"rst_gen"};
    vpvper::generic::terminal term;
    sc_core::sc_signal<bool> rst_n{"rst_n"};
    sc_core::sc_vector<sc_core::sc_signal<bool>> pins_o{"pins_o", 32};
    sc_core::sc_vector<sc_core::sc_signal<bool>> pins_oe_o{"pins_oe_o", 32};
    sc_core::sc_vector<sc_core::sc_signal<bool>> pins_i{"pins_i", 32};
    sc_core::sc_signal<bool> uart0_tx_o{"uart0_tx_o"};
    sc_core::sc_signal<bool> uart0_rx_i{"uart0_rx_i"};
    sc_core::sc_vector<sc_core::sc_signal<bool>> t0_clear_i{"t0_clear_i", vpvper::minres::timer::CLEAR_CNT};
    sc_core::sc_vector<sc_core::sc_signal<bool>> t0_tick_i{"t0_tick_i", vpvper::minres::timer::TICK_CNT - 1};
    spi::spi_channel spi{"spi", 1};
    vpvper::generic::spi_mem qspi_mem{"qspi_mem"};
    sc_core::sc_signal<sc_core::sc_time> clk_i{"clk_i"};
};

} // namespace vp

#endif /* SRC_VP_TB_H_ */
