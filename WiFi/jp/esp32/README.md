## ESP32 Firmware

This directory contains ESP32-specific code to interface with the SJ1 board and a remote server.

#### Purpose

    Since the SJ1 board does not have wireless capabilities, and the project is based around the SJ1 board, the ESP32 is a cheap but effective powerhouse to add wireless communication to the project.

#### How It Works

    1. A "remote server" or a computer with a browser serving a web page, sends command packets to the ESP32.  The ESP32 checks the packet is valid and passes it on to the SJ1 board over UART.

    2. The SJ1 board sends a diagnostic packet over UART to the ESP32.  The ESP32 checks the packet is valid and passes it on to the remote server over a socket.

#### Tasks

    To be finalized.

#### Flow

<pre>
connection1 &rarr; socket_rx_task1 &rarr;
connection2 &rarr; socket_rx_task2 &rarr; ServerQueue &rarr; uart_tx_task &rarr; UART_TX
connection3 &rarr; socket_rx_task3 &rarr;
</pre>

<pre>
<!-- Don't worry, this lines up when compiled -->
                                      &rarr; socket_tx_task1 &rarr; send1
UART_RX &rarr; uart_rx_task &rarr; ClientQueue  &rarr; socket_tx_task2 &rarr; send2
                                      &rarr; socket_tx_task3 &rarr; send3
</pre>