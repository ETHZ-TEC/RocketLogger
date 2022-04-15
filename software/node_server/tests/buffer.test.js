"use strict";

import { AggregatingBuffer } from '../buffer.js';


test('AggregatingBuffer w/o initial value', () => {
    const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);
    buffer._data.forEach((_, i, arr) => { arr[i] = i; });

    const data_view = buffer.getView();
    expect(data_view[0]).toBe(0);
    expect(data_view[73]).toBe(73);
    expect(data_view[data_view.length - 31]).toBe(data_view.length - 31);
    expect(data_view[data_view.length - 1]).toBe(data_view.length - 1);
});

test('AggregatingBuffer with initial value', () => {
    const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10, NaN);

    const data_view = buffer.getView();
    expect(data_view[0]).toBe(NaN);
    expect(data_view[73]).toBe(NaN);
    expect(data_view[data_view.length - 17]).toBe(NaN);
    expect(data_view[data_view.length - 1]).toBe(NaN);
});


test('AggregatingBuffer values after add()', () => {
    const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);
    buffer._data.forEach((_, i, arr) => { arr[i] = i; });

    const data_to_add = new Float32Array(100).fill(NaN);
    buffer.add(data_to_add);

    const data_view = buffer.getView();
    expect(data_view[0]).toBe(1);
    expect(data_view[1]).toBe(2);
    expect(data_view[data_view.length - data_to_add.length - 13]).toBe(data_view.length - 13);
    expect(data_view[data_view.length - data_to_add.length - 1]).toBe(data_view.length - 1);
    expect(data_view[data_view.length - data_to_add.length]).toBe(NaN);
    expect(data_view[data_view.length - 1]).toBe(NaN);
});