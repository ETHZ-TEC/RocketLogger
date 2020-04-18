/**
 * Copyright (c) 2020, ETH Zurich, Computer Engineering Group
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

/// Maximum data buffer length
const RL_DATA_BUFFER_LENGTH = 10000;

/// Data buffer struture
let rl_data = {};
/// Data buffer metadata
let rl_metadata = {};

/**
 * Initialization when document is fully loaded
 */
$(() => {
    rl_socket.on('data', (res) => {
        console.log(`rl data: t=${res.t}, ${res.metadata}`);
        // decode and process channel data
        for (const meta of res.metadata) {
            // decode data
            let data;
            if (meta.digital) {
                data = Array.from(new Uint32Array(res.data[meta.name]));
            } else if (meta.scale) {
                data = Array.from(new Int32Array(res.data[meta.name]),
                    (val) => { return val * meta.scale; });
            } else {
                data = Array.from(new Int32Array(res.data[meta.name]));
            }

            // add data to un-typed array buffer
            if (rl_data[meta.name]) {
                rl_data[meta.name] = rl_data[meta.name].concat(data);
            } else {
                rl_data[meta.name] = data;
            }

            // drop old values
            if (rl_data[meta.name].length > RL_DATA_BUFFER_LENGTH) {
                rl_data[meta.name].splice(0,
                    rl_data[meta.name].length - RL_DATA_BUFFER_LENGTH);
            }
        }
        rl_metadata = res.metadata;
        /// @todo trigger plot update
    });
});
