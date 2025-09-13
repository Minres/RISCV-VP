/*
 * Copyright (c) 2019 -2021 MINRES Technolgies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system.h"

#include <minres/timer.h>
#include <minres/uart.h>
#include <scc/utilities.h>

namespace vp {

using namespace sc_core;
using namespace vpvper::minres;

SC_HAS_PROCESS(system); // NOLINT
system::system(sc_core::sc_module_name nm)
: sc_core::sc_module(nm)
, NAMED(ahb_router, 3, 2)
, NAMED(apbBridge, PipelinedMemoryBusToApbBridge_map.size(), 1) {
    mtime_clk = (1.0 / 32768) * 1_sec;

    core_complex.ibus(ahb_router.target[0]);
    core_complex.dbus(ahb_router.target[1]);

    ahb_router.bind_target(qspi.xip_sck, 0, 0x20000000, 16_MB);
    ahb_router.bind_target(mem_ram.target, 1, 0x00000000, 32_MB);
    ahb_router.bind_target(apbBridge.target[0], 2, 0x10000000, 256_MB);

    size_t i = 0;
    for(const auto& e : PipelinedMemoryBusToApbBridge_map) {
        apbBridge.bind_target(e.target, i, e.start, e.size);
        i++;
    }

    gpio0.clk_i(clk_i);
    uart0.clk_i(clk_i);
    timer0.clk_i(clk_i);
    aclint.clk_i(clk_i);
    irq_ctrl.clk_i(clk_i);
    qspi.clk_i(clk_i);
    core_complex.clk_i(clk_i);
    // mem_ram.clk_i(clk_i);

    gpio0.rst_i(rst_s);
    uart0.rst_i(rst_s);
    timer0.rst_i(rst_s);
    aclint.rst_i(rst_s);
    irq_ctrl.rst_i(rst_s);
    qspi.rst_i(rst_s);
    core_complex.rst_i(rst_s);

    aclint.mtime_clk_i(mtime_clk);
    aclint.mtime_o(mtime_s);
    aclint.mtime_int_o(mtime_int_s);
    aclint.msip_int_o(msip_int_s);
    irq_ctrl.irq_o(core_int_s);
    irq_ctrl.pending_irq_i(irq_int_s);

    uart0.irq_o(irq_int_s[0]);
    timer0.interrupt_o[0](irq_int_s[1]);
    timer0.interrupt_o[1](irq_int_s[2]);
    qspi.irq_o(irq_int_s[3]);

    core_complex.mtime_i(mtime_s);
    core_complex.timer_irq_i(mtime_int_s);
    core_complex.ext_irq_i(core_int_s);
    core_complex.local_irq_i(local_int_s);
    core_complex.sw_irq_i(msip_int_s);

    gpio0.pins_i(pins_i);
    gpio0.pins_o(pins_o);
    gpio0.oe_o(pins_oe_o);

    uart0.tx_o(uart0_tx_o);
    uart0.rx_i(uart0_rx_i);

    timer0.clear_i(t0_clear_i);
    timer0.tick_i(t0_tick_i);

    qspi.spi_i(mspi0);

    SC_METHOD(gen_reset);
    sensitive << erst_n;
}
void system::gen_reset() {
    if(erst_n.read())
        rst_s = 0;
    else
        rst_s = 1;
}

} // namespace vp
