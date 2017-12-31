## Embedded MP3 System

This project aims to be a music player which can retrieve mp3 files from an SD card and play them through an interface to the user.

### High Level Explanation

The microcontroller is responsible for communicating with the decoder.  An SD card with uploaded mp3 files is read and segments of each file
are transferred to the decoder.  Buttons are used for sending commands, such as: play/pause, stop, change song, switch screens.
A terminal command handler also provides extra commands since there are not enough buttons for each command.  A speaker or pair of earphones can be connected to the decoder. An LCD screen displays the list of songs on one screen mode, and individual song details on another screen mode.

### Components

    1. SJ1 Development Board based off of NXP LPC1758 (Cortex M-3)
    2. VS1053b MP3 decoder with breakout board
    3. Multiple tactile buttons
    4. SD card
    5. 20x4 LCD screen

### Framework

    1. SJSU-Dev framework
        Initializes the board and peripherals, low level drivers, communicates with FreeRTOS
    2. FreeRTOS
        Real-time operating system for task management and synchronization

### Project File Structure

There are a lot of framework files, but the project-specific files can be found under <b>firmware/mp3/L5_Application</b>.

    app     : Application level functions
    base    : Terminal command handler, and system drivers (from framework + modified)
    drivers : Device drivers
    tasks   : RTOS tasks

    └──firmware/mp3/L5_Application
        ├── app
        |   ├── decoder
        |   ├── genre_lut
        |   ├── mp3_track_list
        |   ├── msg_protocol
        |   └── utilities
        ├── base
        |   ├── handlers
        |   └── terminal
        ├── drivers
        |   ├── lcd
        |   └── mp3_uart
        ├── tasks
        |   ├── ButtonTask
        |   ├── DMATask
        |   ├── DecoderTask
        |   ├── LCDTask
        |   ├── RxTask
        |   ├── TxTask
        |   └── WatchdogTask
        ├── main.cpp
        └── common.hpp


### Tasks

##### <u>Methodology</u>
The tasks are separated into 3 different priorities.  There were time where splitting the tasks amongst a wider range of priorities might have been useful, but decided for a simpler design.
<b>Low priority tasks</b> are the tasks that regularly operate and are executing 100% of the time.
<b>Medium priority tasks</b> are the tasks that are blocked, waiting on a queue or semaphore.
There are 2 <b>high priority tasks</b>.  
<i>DMATask</i> is blocked waiting on a semaphore to signal a DMA request.  Once it is unblocked, it takes over the system by suspending context switches until the DMA process is complete.
<i>WatchdogTask</i> periodically checks an event group for the correct bits.  It is blocked most of the time but wakes up periodically.

<pre>
<u>ButtonTask</u>
Purpose  : Responds to GPIO interrupts and forwards the interrupted pin number to different tasks.
Priority : <span style="color:blue">Medium</span>
Reason   : Blocked normally. Needs to be serviced quickly so is a higher priority than normally operating tasks (Low).
</pre>

<pre>
<u>DMATask (Currently untested)</u>
Purpose  : Responds to "DMA" request in order to receive large mp3 files over UART and store onto SD card for playback.
Priority : <span style="color:red">High</span>
Reason   : Blocked normally. Needs to be serviced immediately and take over control of the system.
</pre>

<pre>
<u>DecoderTask</u>
Purpose  : Main task which interfaces with the decoder and plays the songs.
Priority : <span style="color:green">Low</span>
Reason   : Regular operation, lowest priority.
</pre>

<pre>
<u>LCDTask</u>
Purpose  : Handles what information and when gets printed to the LCD screen or terminal.
Priority : <span style="color:blue">Medium</span>
Reason   : Blocked normally. Needs to be serviced quickly so is a higher priority than normally operating tasks (Low).
</pre>

<pre>
<u>RxTask</u>
Purpose  : Receives command requests over UART.
Priority : <span style="color:blue">Medium</span>
Reason   : Blocked normally. Needs to be serviced quickly to respond to commands like ButtonTask and LCDTask.
</pre>

<pre>
<u>TxTask</u>
Purpose  : Transmits diagnostic messages over UART
Priority : <span style="color:green">Low</span>
Reason   : Regular operation, lowest priority.
</pre>

<pre>
<u>WatchdogTask (Currently unused)</u>
Purpose  : Ensures every task, that is not normally blocked, consistently responds and is not stuck.
Priority : <span style="color:red">High</span>
Reason   : Blocks until periodically checks that each event group bit is set.  Must not be shadowed so it is the highest priority.
</pre>

#### In The Works

Currently basic functionality is implemented and validated. (LCD screen not tested yet)
Next steps are to begin implementing communication with ESP32.  The purpose of the ESP32 is its WiFi capabilities.  It will be responsible for passing data (packets)  to/from the SJ1 and a browser-based server.
SJ1 will send diagnostic packets to the ESP32, which will pass it to the server.
Server will send command packets to the ESP32, which will pass it to the SJ1.
After this is implemented and validated, the next steps will be to cut out the SJ1 board and do all the processing and peripheral interfacing on the ESP32.