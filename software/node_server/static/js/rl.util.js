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

"use strict";

/// format display byte values
function bytes_to_string(bytes) {
    if (bytes === 0) {
        return '0 B';
    }
    const log1k = Math.floor(Math.log10(bytes) / 3);
    const value = (bytes / Math.pow(1000, log1k));

    switch (log1k) {
        case 0:
            return value.toFixed(0) + ' B';
        case 1:
            return value.toFixed(2) + ' kB';
        case 2:
            return value.toFixed(2) + ' MB';
        case 3:
            return value.toFixed(2) + ' GB';
        case 4:
            return value.toFixed(2) + ' TB';
        default:
            return bytes.toPrecision(5);
    }
}

/// extend single digit date values
function date_zero_extend(value) {
    if (value < 10) {
        return '0' + value.toFixed();
    }
    return value.toFixed();
}

/// get formatted date string from date object
function date_to_string(date, join = '-') {
    const year = date.getFullYear().toString();
    const month = date_zero_extend(date.getMonth() + 1);
    const day = date_zero_extend(date.getDate());
    return year + join + month + join + day;
}

/// get formatted time string from date object
function time_to_string(time, join = ':') {
    const hour = date_zero_extend(time.getHours());
    const minute = date_zero_extend(time.getMinutes());
    const second = date_zero_extend(time.getSeconds());
    return hour + join + minute + join + second;
}

/// get file prefix string from date object
function date_to_prefix_string(date) {
    return date_to_string(date, '') + '_' + time_to_string(date, '');
}

/// get time string from unit timestamp
function unix_to_datetime_string(seconds) {
    const date = new Date(seconds * 1000);
    return date_to_string(date) + ' ' + time_to_string(date);
}

/// get time duration string from unit timestamp
function unix_to_timespan_string(seconds) {
    if (seconds === null || isNaN(seconds) || seconds === 0) {
        return '0 s';
    }

    const date = new Date(seconds * 1000);
    const year = date.getUTCFullYear() - 1970;
    const month = date.getUTCMonth();
    const day = date.getUTCDate() - 1;
    const hour = date.getUTCHours();
    const minute = date.getUTCMinutes();
    const second = date.getUTCSeconds();

    let str = '';
    if (year > 0 || str.length > 0) {
        str = str + year.toFixed() + ' year' + ((year !== 1) ? 's ' : ' ');
    }
    if (month > 0 || str.length > 0) {
        str = str + date_zero_extend(month) + ' month' + ((month !== 1) ? 's ' : ' ');
    }
    if (day > 0 || str.length > 0) {
        str = str + date_zero_extend(day) + ' day' + ((day !== 1) ? 's ' : ' ');
    }
    if (hour > 0 || str.length > 0) {
        str = str + date_zero_extend(hour) + ' h ';
    }
    if (minute > 0 || str.length > 0) {
        str = str + date_zero_extend(minute) + ' min ';
    }
    if (second > 0 || str.length > 0) {
        str = str + date_zero_extend(second) + ' s';
    }

    return str.trim();
}


/// DOM manipulation helpers

function show(element) {
    removeClass(element, 'd-none');
}
function hide(element) {
    addClass(element, 'd-none');
}

function addClass(element, ...classes) {
    selectElement(element).classList.add(...classes);
}
function removeClass(element, ...classes) {
    selectElement(element).classList.remove(...classes);
}
function replaceClass(element, remove_class, add_class) {
    removeClass(element, remove_class);
    addClass(element, add_class);
}

function triggerEvent(element, event) {
    selectElement(element).dispatchEvent(new Event(event));
}

function selectElement(element) {
    return typeof element === 'string' ? document.querySelector(element) : element;
}
