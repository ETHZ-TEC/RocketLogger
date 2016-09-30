.origin 0                        // start of program in PRU memory
.entrypoint START                // program entry point
 
#define PRU0_R31_VEC_VALID 32    // allows notification of program completion
#define PRU_EVTOUT_0    3        // the event number that is sent back

// ADC commands
#define SDATAC 0x11000000
#define RREG   0x20000000

#define TIME_CLOCK 5			// lower SCLK for commands

#define STATUS_MASK 0x1

 
// --------------------------- Macros ------------------------------------------ //
 
 
// WAIT
 .macro wait
.mparam time

	mov r20, time
WAITLOOP:
	QBEQ ENDWAIT, r20, 0
	SUB r20, r20, 1
	QBA WAITLOOP
ENDWAIT:

.endm


// SPI CYCLE (SPI cycle when sending commands)
.macro  spi_cycle       
.mparam oup, inp


// clock high phase
	SET r30.t0 // set the clock high
	
	QBBC OUPLOW, oup.t31 // write data
	SET r30.t1
	QBA OUPHIGH
OUPLOW:
	CLR r30.t1
	ADD r0, r0, 0 // nop line (to have fixed timing)
OUPHIGH:
	LSL oup, oup, 1 // shift output register
	
	wait TIME_CLOCK

// clock low phase
	CLR r30.t0 // set the clock low
	
	QBBC INPLOW, r31.t2 // get data
	OR inp, inp, 0x00000001
	QBA INPHIGH
INPLOW:
	ADD r0, r0, 0 // nop line
	ADD r0, r0, 0 // nop line
INPHIGH:
	LSL inp, inp, 1 //shift input register
	
	wait TIME_CLOCK
	
.endm
 
 
// SEND COMMAND (send commands and write registers)
.macro  send_command       
.mparam cmd

	CLR r30.t3 // set the CS line low (active low)
	CLR r30.t5
	MOV r4, 24 // going to write 24 bits
	
CMD_BIT:                   // loop for each of the 24 bits
	SUB r4, r4, 1             // count down through the bits
	spi_cycle cmd, r10         // perform spi_cycle
	QBNE CMD_BIT, r4, 0
	
	wait 1000 // need to wait for ADC
	
	SET r30.t3 // pull the CS line high (end of sample)
	SET r30.t5
	
.endm


// RECEIVE WORD (receive a sample, this is a bit ugly, since it was optimized for time)
.macro receive_word
.mparam inp0, inp1, precision

	MOV r4, precision // load precision
	SET r30.t0 // set the clock high
RECEIVE_BIT:                  // loop for each of the 24 bits
	
	// clock high phase
	SUB r4, r4, 1             // count down through the bits
	LSL inp0, inp0, 1 //shift input register
	LSL inp1, inp1, 1 //shift input register
	
	ADD r0, r0, 0 // nop line

	// clock low phase
	CLR r30.t0 // set the clock low
	ADD r0, r0, 0 // nop line
	
	QBBS INP0HIGH, r31.t2 // get data0
	ADD r0, r0, 0 // nop line
	QBA INP0LOW
	
INP0HIGH:
	OR inp0, inp0, 0x00000001
	
INP0LOW:

	QBBS INP1HIGH, r31.t16 // get data1
	ADD r0, r0, 0 // nop line
	QBA INP1LOW
	
INP1HIGH:
	OR inp1, inp1, 0x00000001
	
INP1LOW:
	SET r30.t0 // set the clock high
	QBNE RECEIVE_BIT, r4, 0

.endm


// RECEIVE DATA (receive all samples)
.macro receive_data

	CLR r30.t3 // set the CS line low (active low)
	CLR r30.t5
	
	wait 1
	
	receive_word r24, r25, 24 // read status word
	
	// read all channels
	receive_word r10, r15, r3
	receive_word r11, r16, r3
	receive_word r12, r17, r3
	receive_word r13, r18, r3
	receive_word r14, r19, r3
	
	SET r30.t3 // pull the CS line high (end of sample)
	SET r30.t5

.endm


// SIGN EXTEND 24-Bit

.macro sign_extend_24
.mparam reg
	
	AND reg, reg, r27
	QBBC POS24, reg.t23
	
	OR reg, reg, r26
	
POS24:

.endm

 
// --------------------------- Main Program ------------------------------------------ //
 
START:
    // Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    r0, C4, 4, 4     // load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        // clear bit 4 (STANDBY_INIT)
	SBCO    r0, C4, 4, 4     // store the modified r0 back at the load addr
 
	MOV r1, 0x00000000 // load the base address into r1
	
	// masks for sign extension
	MOV r26, 0xFF
	LSL r26, r26, 24
	NOT r27, r26
	
	SET r30.t3 // CS high
	SET r30.t5
	CLR r30.t1 // MOSI low
	CLR r30.t7 // START low
	MOV r10, 0 // receive register
	
	
	// load number of commands to set
	LBBO r7, r1, 28, 4
	
	// load command memory position
	MOV r11, 32
	
	
	
CONFIGURE:
	// check if all comands sent
	QBEQ LOAD, r7, 0
	SUB r7, r7, 1
	
	// send command
	LBBO r2, r1, r11, 4
	send_command r2
	ADD r11, r11, 4 // increase command position
	
	wait 1000 //wait 10 // enough time for cs toggle and command to settle
	
	LSR r10, r10, 1
	
	QBA CONFIGURE

LOAD:
	
	// load configuration
	LBBO r8, r1, 12, 4  // buffer0 address
	LBBO r9, r1, 24, 4 // number of samples to get
	LBBO r6, r1, 20, 4 // buffer size
	MOV r5, 0 //buffer used (toggles)
	LBBO r3, r1, 4, 4 // precision
	LBBO r28, r1, 8, 4 // size
	SBBO r5, r8, 0, 4 // store buffer number
	ADD r8, r8, 4
	
	
SAMPLING:
	// set ADC start signal to one
	SET r30.t7

SAMPLINGLOOP:
	LBBO r0, r1, 0, 4 // load status

	// check if buffer full
	QBNE BUFFERNOTFULL, r6, 0
	
	// buffer full
	
	// check if still running
	QBBC FINISHED, r0.t0
	
	// check if in continuous mode
	QBBS NOTFINISHED, r0.t1
	
	// check if all samples done
	QBEQ FINISHED, r9, 0

NOTFINISHED:
	// load buffer address
	QBBC BUFFER1, r5.t0
	LBBO r8, r1, 12, 4  //  load buffer0 address
	QBA BUFFER0
BUFFER1:
	LBBO r8, r1, 16, 4  //  load buffer1 address
BUFFER0:

	// switch buffer
	ADD r5, r5, 1//NOT r5, r5
	
	// store buffer number
	SBBO r5, r8, 0, 4
	ADD r8, r8, 4
	
	// buffer full (post event)
	MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0 // interrupt to userspace program
	LBBO r6, r1, 20, 4 // reload buffer size
	
	// store buffer number
	
BUFFERNOTFULL:

	// update counters
	SUB r6, r6, 1
	QBEQ READ, r9, 0
	SUB r9, r9, 1
	
READ:
	// reset input register
	MOV r10, 0
	
	// wait for data ready
	WBC r31.t15
	
	// receive data
	receive_data
	
	// mask and store status (low range valid)
	AND r24, r24, STATUS_MASK
	AND r25, r25, STATUS_MASK
	LSL r25, r25, 8
	OR r24, r24, r25
	SBBO r24, r8, 0, 2
	ADD r8, r8, 2
	
	// sign extension and storing
	QBEQ HIGHPRECISION, r3, 24
	
	// I1
	SBBO r10, r8, 0, 2
	ADD r8, r8, 2
	SBBO r11, r8, 0, 2
	ADD r8, r8, 2
	SBBO r12, r8, 0, 2
	ADD r8, r8, 2
	
	// V1,2
	SBBO r14, r8, 0, 2
	ADD r8, r8, 2
	SBBO r13, r8, 0, 2
	ADD r8, r8, 2
	
	// I2
	SBBO r15, r8, 0, 2
	ADD r8, r8, 2
	SBBO r16, r8, 0, 2
	ADD r8, r8, 2
	SBBO r17, r8, 0, 2
	ADD r8, r8, 2
	
	// V3,4
	SBBO r19, r8, 0, 2
	ADD r8, r8, 2
	SBBO r18, r8, 0, 2
	ADD r8, r8, 2
	
	QBA LOWPRECISION
	
HIGHPRECISION:

	sign_extend_24 r10
	sign_extend_24 r11
	sign_extend_24 r12
	sign_extend_24 r13
	sign_extend_24 r14
	sign_extend_24 r15
	sign_extend_24 r16
	sign_extend_24 r17
	sign_extend_24 r18
	sign_extend_24 r19
	
	// I1
	SBBO r10, r8, 0, 4
	ADD r8, r8, 4
	SBBO r11, r8, 0, 4
	ADD r8, r8, 4
	SBBO r12, r8, 0, 4
	ADD r8, r8, 4
	
	// V1,2
	SBBO r14, r8, 0, 4
	ADD r8, r8, 4
	SBBO r13, r8, 0, 4
	ADD r8, r8, 4
	
	// I2
	SBBO r15, r8, 0, 4
	ADD r8, r8, 4
	SBBO r16, r8, 0, 4
	ADD r8, r8, 4
	SBBO r17, r8, 0, 4
	ADD r8, r8, 4
	
	// V3,4
	SBBO r19, r8, 0, 4
	ADD r8, r8, 4
	SBBO r18, r8, 0, 4
	ADD r8, r8, 4
	
LOWPRECISION:

	QBA SAMPLINGLOOP

FINISHED:

	
	// clear all ADC signals
	CLR r30.t7 // START
	CLR r30.t3 // CS1
	CLR r30.t5 // CS2
	
	// set SDATAC command (RREG needed for SDATAC to work!)
	MOV r2, SDATAC
	send_command r2
	MOV r2, RREG
	send_command r2

	// interrupt to userspace program
	MOV r31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
	
	// pause PRU
	HALT


