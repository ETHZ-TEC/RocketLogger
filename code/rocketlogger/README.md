RocketLogger Software Documentation
===================================

**[TBD]**

The RocketLogger software includes the three following binaries, which use de RocketLogger library (ToDo: link?).

### RocketLogger Binary

Main RocketLogger binary defined in [rocketlogger.c](@ref rocketlogger.c). Provides a CLI to control an monitor the sampling.

### RocketLogger Deamon

RocketLogger deamon program defined in [rl_deamon.c](@ref rl_deamon.c). Continuously waits on interrupt on button GPIO and starts/stops RocketLogger.

### RocketLogger Server

RocketLogger server program defined in [rl_server.c](@ref rl_server.c). Returns status and current sampling data (if available) when running and default configuration otherwise, for use in a webserver.