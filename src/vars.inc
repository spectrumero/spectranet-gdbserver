defc v_rst8_handler = 0x3B02
defc gdbserver_state = 0x3B03
defc gdbserver_registers = gdbserver_state + 0

defc gdbserver_register_sp = gdbserver_registers
defc gdbserver_register_pc = gdbserver_registers + 2
defc gdbserver_register_hl = gdbserver_registers + 4
defc gdbserver_register_de = gdbserver_registers + 6
defc gdbserver_register_bc = gdbserver_registers + 8
defc gdbserver_register_af = gdbserver_registers + 10

defc gdbserver_rst8_handler = gdbserver_state + 12
defc gdbserver_sockets = gdbserver_rst8_handler + 72
defc gdbserver_trap_handler = gdbserver_sockets + 4
