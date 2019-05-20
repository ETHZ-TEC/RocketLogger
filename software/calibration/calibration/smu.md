# SMU2450 Remote Control

**Source:**
SMU2450 Reference Manual p. 3-65ff 
- [Product Page](https://www.tek.com/keithley-source-measure-units/keithley-smu-2400-graphical-series-sourcemeter)
- [Manual Download](https://download.tek.com/manual/2450-901-01_D_May_2015_Ref.pdf)


## Setting up a sweep using TSP commands

To set up a sweep using TSP commands, you send one of the following commands:
- smu.source.sweeplinear(): Sets up a linear sweep for a fixed number of measurement points.
- smu.source.sweeplinearstep(): Sets up a linear source sweep configuration list and trigger model with a fixed number of steps.
- smu.source.sweeplist(): Sets up a sweep based on a configuration list, which allows you to customize the sweep.
- smu.source.sweeplog(): Sets up a logarithmic sweep for a set number of measurement points.

### To create a sweep:
1. Set the source function using smu.source.func.
2. Set the source range using smu.source.range.
3. Set any other source settings that apply to your sweep. You must set source settings before the sweep function is called.
4. If you are using smu.source.sweeplist(), set up the source configuration list for your sweep.
5. Set the parameters for the sweep command.
6. Set the measurement function using smu.measure.func.
7. Set the measurement range using smu.measure.range.
8. Make any other settings appropriate to your sweep.
9. Send trigger.model.initiate() to start the sweep.

### Aborting a sweep
Sweeps can be stopped for the following reasons:
- The limit set by the abort on limit setting was exceeded
- The trigger model is aborted
You can stop the sweep while it is in progress. When you stop the sweep, all sweep commands in the trigger model are terminated.


## Examples

### Linear sweep with a voltage source
The following examples perform a linear sweep that uses a voltage source. They perform the following actions:
- Reset the instrument to its defaults.
- In TSP only, name the configuration list that is created for this sweep RES.
- Set the source function to voltage.
- Set the source range to 20 V.
- Set the source limit for measurements to 0.02 A
- Set the measure function to current.
- Set the current range to automatic.
- Set up a linear sweep that sweeps from 0 to 10 V in 21 steps with a source delay of 200 ms.
- Start the sweeps.
- Wait until all commands are complete and then query the source value and measurement reading.

No buffer is defined, so the data is stored in `defbuffer1`. See Reading buffers (on page 3-10) for more information about reading buffers.

#### Using TSP commands:
```
reset()
smu.measure.func = smu.FUNC_DC_CURRENT
smu.measure.autorange = smu.ON
smu.source.func = smu.FUNC_DC_VOLTAGE
smu.source.range = 20
smu.source.ilimit.level = 0.02
smu.source.sweeplinear("RES", 0, 10, 21, 200e-3)
trigger.model.initiate()
waitcomplete()
printbuffer(1, 21, defbuffer1.sourcevalues, defbuffer1.readings)
```

### Logarithmic sweep with a current source
The following examples perform a logarithmic sweep using a current source. They perform the following actions:
- Reset the instrument to its defaults.
- Set the measure function to current.
- Set the source function to current.
- Set the source range to 100 mA.
- Set up a logarithmic sweep from 100 μA to 100 mA in 10 steps with a source delay of 10 ms, a sweep count of 1, and a fixed source range. In TSP only, name the configuration list that is created for this sweep RES.
- Set the current range to 100 μA.
- Start the sweep.

No buffer is defined, so the data is stored in defbuffer1. See Reading buffers (on page 3-10) for more information on reading buffers.

#### Using TSP commands:
```
reset()
smu.source.func = smu.FUNC_DC_CURRENT
smu.source.range = 100e-3
smu.source.vlimit.level = 20
smu.source.sweeplog("RES", 100e-6, 100e-3, 10, 10e-3)
smu.measure.func = smu.FUNC_DC_VOLTAGE
smu.measure.range = 20
trigger.model.initiate()
waitcomplete()
printbuffer(1, 10, defbuffer1.sourcevalues, defbuffer1.readings)
```

## Function Reference smu.source.sweeplinear():

```
smu.source.sweeplinear(configListName, start, stop, points, delay, count, rangeType, failAbort, dual, bufferName)
```

- _configListName_
  
  A string that contains the name of the configuration list that the instrument will create for this sweep

- _start_
  
  The voltage or current source level at which the sweep starts:
  - Current: -1.05 A to 1.05 A
  - Voltage: -210 V to 210 V

- _stop_
  
  The voltage or current at which the sweep stops:
  - Current: -1.05 A to 1.05 A
  - Voltage: -210 V to 210 V

- _points_
  
  The number of source-measure points between the start and stop values of the sweep (2 to 1e6); to calculate the number of source-measure points in a sweep, use the following formula: `Points = [(Stop - Start) / Step] + 1`

- _delay_
  
  The delay between measurement points; default is `smu.DELAY_AUTO`, which enables autodelay, or a specific delay value from 50 μs to 10 ks, or 0 for no delay.

- _count_
  
  The number of times to run the sweep; default is 1:
  - Infinite loop: `smu.INFINITE`
  - Finite loop: 1 to 268,435,455

- _rangeType_
  The source range that is used for the sweep:
  - Most sensitive source range for each source level in the sweep: `smu.RANGE_AUTO`
  - Best fixed range: `smu.RANGE_BEST` (default)
  - Present source range for the entire sweep: `smu.RANGE_FIXED`

- _failAbort_
  - Complete the sweep if the source limit is exceeded: `smu.OFF`
  - Abort the sweep if the source limit is exceeded: `smu.ON` (default)

- _dual_
  Determines if the sweep runs from start to stop and then from stop to start:
  - Sweep from start to stop only: `smu.OFF` (default)
  - Sweep from start to stop, then stop to start: `smu.ON`

- _bufferName_
  
  The name of a reading buffer; the default buffers (`defbuffer1` or `defbuffer2`) or the name of a user-defined buffer; if no buffer is specified, this parameter defaults to `defbuffer1`
  