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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

"use strict";

const util = require('util');
const exec = util.promisify(require('child_process').exec);
const stat = util.promisify(require('fs').stat);

module.exports = {
    /// helper function to display byte values
    bytes_to_string(bytes) {
        if (bytes === 0) {
            return "0 B";
        }
        let log1k = Math.floor(Math.log10(bytes) / 3);
        let value = (bytes / Math.pow(1000, log1k));

        switch (log1k) {
            case 0:
                return value.toFixed(0) + " B";
            case 1:
                return value.toFixed(2) + " kB";
            case 2:
                return value.toFixed(2) + " MB";
            case 3:
                return value.toFixed(2) + " GB";
            case 4:
                return value.toFixed(2) + " TB";
            default:
                return bytes.toPrecision(5) + " B";
        }
    },

    /// helper function to display dates
    date_to_string(date) {
        return date.toISOString().split('.')[0].replace('T', ' ')
    },

    /// check if two files or paths are located on the same filesystem
    async is_same_filesystem(first, second) {
        const files_stat = await Promise.all([
            stat(first),
            stat(second),
        ]);
        return files_stat[0].dev === files_stat[1].dev;
    },

    /// helper function to reboot the system
    async system_reboot() {
        const cmd = 'sudo shutdown --reboot now';
        const { stdout } = await exec(cmd, { timeout: 500 });
        return stdout;
    },

    /// helper function to shutdown the system
    async system_poweroff() {
        const cmd = 'sudo shutdown --poweroff now';
        const { stdout } = await exec(cmd, { timeout: 500 });
        return stdout;
    },
};
