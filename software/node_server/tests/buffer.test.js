"use strict";

import { buffer_init, buffer_add, buffer_get_view } from '../buffer.js';


test('buffer_init w/o initial value', () => {
    const buffer = {};
    buffer_init(buffer, Float32Array, 1000, 3, 10);
    buffer.data.forEach((_, i, arr) => { arr[i] = i; });
    const data_view = buffer_get_view(buffer);

    expect(data_view[0]).toBe(0);
    expect(data_view[73]).toBe(73);
    expect(data_view[data_view.length - 31]).toBe(data_view.length - 31);
    expect(data_view[data_view.length - 1]).toBe(data_view.length - 1);
});

test('buffer_init with initial value', () => {
    const buffer = {};
    buffer_init(buffer, Float32Array, 1000, 3, 10, NaN);
    const data_view = buffer_get_view(buffer);

    expect(data_view[0]).toBe(NaN);
    expect(data_view[73]).toBe(NaN);
    expect(data_view[data_view.length - 17]).toBe(NaN);
    expect(data_view[data_view.length - 1]).toBe(NaN);
});


test('buffer content after buffer_add', () => {
    const buffer = {};
    buffer_init(buffer, Float32Array, 1000, 3, 10);
    buffer.data.forEach((_, i, arr) => { arr[i] = i; });

    const data_to_add = new Float32Array(100).fill(NaN);
    buffer_add(buffer, data_to_add);
    const data_view = buffer_get_view(buffer);

    expect(data_view[0]).toBe(1);
    expect(data_view[1]).toBe(2);
    expect(data_view[data_view.length - data_to_add.length - 13]).toBe(data_view.length - 13);
    expect(data_view[data_view.length - data_to_add.length - 1]).toBe(data_view.length - 1);
    expect(data_view[data_view.length - data_to_add.length]).toBe(NaN);
    expect(data_view[data_view.length - 1]).toBe(NaN);
});
