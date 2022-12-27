"use strict";

import { AggregatingBuffer } from '../buffer.js';


describe('AggregatingBuffer class', () => {
    describe('construction', () => {
        test('Float32 w/o initial value', () => {
            const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);

            const data_view = buffer.getView();
            expect(data_view.every(value => value === 0)).toBeTruthy();
        });

        test('Uint16 w/o initial value', () => {
            const buffer = new AggregatingBuffer(Uint16Array, 1000, 3, 10);

            const data_view = buffer.getView();
            expect(data_view.every(value => value === 0)).toBeTruthy();
        });

        test('Float32 with -17 initial value', () => {
            const initial_value = -17;
            const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10, initial_value);

            const data_view = buffer.getView();
            expect(data_view.every(value => value === initial_value)).toBeTruthy();
        });

        test('Uint16 with 31 initial value', () => {
            const initial_value = 31;
            const buffer = new AggregatingBuffer(Uint16Array, 1000, 3, 10, initial_value);

            const data_view = buffer.getView();
            expect(data_view.every(value => value === initial_value)).toBeTruthy();
        });

        test('Float32 with NaN initial value', () => {
            const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10, NaN);

            const data_view = buffer.getView();
            expect(data_view.every(value => isNaN(value))).toBeTruthy();
        });

        test('Uint16 with NaN initial value to initialize to 0', () => {
            const buffer = new AggregatingBuffer(Uint16Array, 1000, 3, 10, NaN);

            const data_view = buffer.getView();
            expect(data_view.every(value => value === 0)).toBeTruthy();
        });

        test('String w/o initial value', () => {
            const stringAggregatingBuffer = () => {
                new AggregatingBuffer(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer w/o initial value', () => {
            const arrayBufferAggregatingBuffer = () => {
                new AggregatingBuffer(ArrayBuffer, 1000, 3, 10);
            };
            expect(arrayBufferAggregatingBuffer).toThrow(TypeError);
        });
    });

    describe('buffer values', () => {
        let buffer;
        beforeEach(() => {
            buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);
            buffer._data.forEach((_, i, arr) => { arr[i] = i; });
        });

        test('before add()', () => {
            const data_view = buffer.getView();
            expect(data_view[0]).toBe(0);
            expect(data_view[73]).toBe(73);
            expect(data_view[data_view.length - 31]).toBe(data_view.length - 31);
            expect(data_view[data_view.length - 1]).toBe(data_view.length - 1);
        });

        test('after add()', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            const data_view = buffer.getView();
            // 1st buffer
            expect(data_view[0]).toBe(1);
            expect(data_view[1]).toBe(2);
            expect(data_view[999]).toBe(1000);
            // 2nd buffer
            expect(data_view[1000]).toBe(1010);
            expect(data_view[1001]).toBe(1011);
            expect(data_view[1989]).toBe(1999);
            expect(data_view[1990]).toBe(2000);
            expect(data_view[1999]).toBe(2090);
            // 3rd buffer
            expect(data_view[2000]).toBe(2100);
            expect(data_view[2001]).toBe(2101);
            expect(data_view[data_view.length - data_to_add.length - 13]).toBe(data_view.length - 13);
            expect(data_view[data_view.length - data_to_add.length - 1]).toBe(data_view.length - 1);
            expect(data_view[data_view.length - data_to_add.length]).toBe(NaN);
            expect(data_view[data_view.length - 1]).toBe(NaN);
        });
    });
});
