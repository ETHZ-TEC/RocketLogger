/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PWM_H_
#define PWM_H_

/// uDev device export for ePWM0A module
#define EPWM0A_DEVICE "/dev/pwm/ehrpwm0a/"
/// uDev device export for ePWM1A module
#define EPWM1A_DEVICE "/dev/pwm/ehrpwm1a/"
/// uDev device export for ePWM1B module
#define EPWM1B_DEVICE "/dev/pwm/ehrpwm1b/"

/// Default PWM period in nanoseconds
#define PWM_PERIOD_DEFAULT 490
/// Default PWM period in nanoseconds
#define PWM_DUTY_CYCLE_DEFAULT (PWM_PERIOD_DEFAULT / 2)

/**
 * Initialize PWM modules.
 *
 * Enable PWM output via udev exported devices.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int pwm_init(void);

/**
 * Deinitialize PWM modules.
 *
 * Disable PWM output via udev exported devices.
 */
void pwm_deinit(void);

#endif /* PWM_H_ */
