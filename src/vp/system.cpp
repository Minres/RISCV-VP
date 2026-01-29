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

#define UART0_IRQ 16
#define TIMER0_IRQ0 17
#define TIMER0_IRQ1 18
#define QSPI_IRQ 19
#define I2S_IRQ 20
#define CAM_IRQ 21
#define DMA_IRQ 22
#define GPIO_ORQ 23
#define ETH0_IRQ 24
#define ETH1_IRQ 25
#define MDIO0_IRQ 26
#define MDIO1_IRQ 27

system::system(sc_core::sc_module_name nm)
: sc_core::sc_module(nm)
, NAMED(ahb_router, 5, 2)
, NAMED(apbBridge, PipelinedMemoryBusToApbBridge_map.size(), 1) {
    mtime_clk = (1.0 / 32768) * 1_sec;

    clint_int_s.init(core_complex.clint_irq_i.size());
    core_complex.ibus(ahb_router.target[0]);
    core_complex.dbus(ahb_router.target[1]);
    core_complex.mtime_i(mtime_s);
    core_complex.clint_irq_i(clint_int_s);

    ahb_router.bind_target(mem_ram.target, 1, 0x00000000, 128_kB);
    ahb_router.bind_target(apbBridge.target[0], 2, 0x10000000, 128_MB);
    ahb_router.bind_target(eth0.socket, 3, 0x18000000, 4_KiB);
    ahb_router.bind_target(eth1.socket, 4, 0x18001000, 4_KiB);
    ahb_router.bind_target(qspi.xip_sck, 0, 0x20000000, 16_MB);
    size_t i = 0;
    for(const auto& e : PipelinedMemoryBusToApbBridge_map) {
        apbBridge.initiator.at(i)(e.target);
        apbBridge.set_target_range(i, e.start, e.size);
        i++;
    }

    core_complex.clk_i(clk_i);
    gpio0.clk_i(clk_i);
    uart0.clk_i(clk_i);
    timer0.clk_i(clk_i);
    aclint.clk_i(clk_i);
    qspi.clk_i(clk_i);
    eth0.clk_i(clk_i);
    eth1.clk_i(clk_i);

    core_complex.rst_i(rst_s);
    gpio0.rst_i(rst_s);
    uart0.rst_i(rst_s);
    timer0.rst_i(rst_s);
    aclint.rst_i(rst_s);
    qspi.rst_i(rst_s);
    eth0.rst_i(rst_s);
    eth1.rst_i(rst_s);

    aclint.mtime_clk_i(mtime_clk);
    aclint.mtime_o(mtime_s);
    aclint.mtime_int_o[0](clint_int_s[sysc::riscv::TIMER_IRQ]);
    aclint.msip_int_o[0](clint_int_s[sysc::riscv::SW_IRQ]);

    uart0.irq_o(clint_int_s[UART0_IRQ]);
    timer0.interrupt_o[0](clint_int_s[TIMER0_IRQ0]);
    timer0.interrupt_o[1](clint_int_s[TIMER0_IRQ1]);
    qspi.irq_o(clint_int_s[QSPI_IRQ]);
    eth0.irq_o[0](clint_int_s[ETH0_IRQ]);
    eth1.irq_o[0](clint_int_s[ETH1_IRQ]);
    eth0.irq_o[1](clint_int_s[MDIO0_IRQ]);
    eth1.irq_o[1](clint_int_s[MDIO1_IRQ]);

    gpio0.pins_i(pins_i);
    gpio0.pins_o(pins_o);
    gpio0.oe_o(pins_oe_o);

    uart0.tx_o(uart0_tx_o);
    uart0.rx_i(uart0_rx_i);

    timer0.clear_i(t0_clear_i);
    timer0.tick_i(t0_tick_i);

    qspi.spi_i(mspi0);

    eth0.eth_tx(eth0_tx);
    eth0_rx(eth0.eth_rx);
    eth1.eth_tx(eth1_tx);
    eth1_rx(eth1.eth_rx);

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
