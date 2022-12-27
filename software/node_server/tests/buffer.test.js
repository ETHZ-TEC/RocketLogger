"use strict";

import { AggregatingBuffer, AggregatingDataStore } from '../buffer.js';


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

        test('initial state', () => {
            const data_view = buffer.getView();
            expect(data_view.at(0)).toBe(0);
            expect(data_view.at(73)).toBe(73);
            expect(data_view.at(-31)).toBe(data_view.length - 31);
            expect(data_view.at(-1)).toBe(data_view.length - 1);
        });

        test('add() once', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            const data_view = buffer.getView();
            // 1st buffer
            expect(data_view.at(0)).toBe(1);
            expect(data_view.at(1)).toBe(2);
            expect(data_view.at(999)).toBe(1000);
            // 2nd buffer
            expect(data_view.at(1000)).toBe(1010);
            expect(data_view.at(1001)).toBe(1011);
            expect(data_view.at(1989)).toBe(1999);
            expect(data_view.at(1990)).toBe(2000);
            expect(data_view.at(1999)).toBe(2090);
            // 3rd buffer
            expect(data_view.at(2000)).toBe(2100);
            expect(data_view.at(2001)).toBe(2101);
            expect(data_view.at(-data_to_add.length - 13)).toBe(data_view.length - 13);
            expect(data_view.at(-data_to_add.length - 1)).toBe(data_view.length - 1);
            expect(data_view.at(-data_to_add.length)).toBe(data_to_add.at(0));
            expect(data_view.at(-1)).toBe(data_to_add.at(-1));
        });
    });
});

describe('AggregatingDataStore class', () => {
    describe('construction', () => {
        test('Float32Array', () => {
            const buffer = new AggregatingDataStore(Float32Array, 1000, 3, 10);

            const data_view = buffer.getView();
            expect(data_view.every(isNaN)).toBeTruthy();
        });

        test('Float64Array', () => {
            const buffer = new AggregatingDataStore(Float64Array, 1000, 3, 10);

            const data_view = buffer.getView();
            expect(data_view.every(isNaN)).toBeTruthy();
        });

        test('Int8Array', () => {
            const int8ArrayAggregatingBuffer = () => {
                new AggregatingDataStore(Int8Array, 1000, 3, 10);
            };
            expect(int8ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Uint32Array', () => {
            const uint32ArrayAggregatingBuffer = () => {
                new AggregatingDataStore(Uint32Array, 1000, 3, 10);
            };
            expect(uint32ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('String', () => {
            const stringAggregatingBuffer = () => {
                new AggregatingDataStore(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer', () => {
            const arrayBufferAggregatingBuffer = () => {
                new AggregatingDataStore(ArrayBuffer, 1000, 3, 10);
            };
            expect(arrayBufferAggregatingBuffer).toThrow(TypeError);
        });
    });

    describe('buffer values', () => {
        let buffer;
        beforeEach(() => {
            buffer = new AggregatingDataStore(Float32Array, 1000, 3, 10);
        });

        test('initial state', () => {
            expect(buffer._get_start_index()).toBe(buffer._data.length);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(0);

            const data_view = buffer.getView();
            expect(data_view.at(0)).toBeNaN();
            expect(data_view.at(73)).toBeNaN();
            expect(data_view.at(-31)).toBeNaN();
            expect(data_view.at(-1)).toBeNaN();
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, (_, i) => i);
            buffer.add(data_to_add);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.at(-buffer._size - 13)).toBeNaN();
            expect(data_view.at(-buffer._size - 1)).toBeNaN();
            expect(data_view.at(-data_to_add.length - 13)).toBeNaN();
            expect(data_view.at(-data_to_add.length - 1)).toBeNaN();
            expect(data_view.at(-data_to_add.length)).toBe(data_to_add.at(0));
            expect(data_view.at(-1)).toBe(data_to_add.at(-1));
        });

        test('prepend() once', () => {
            const data_to_prepend = Float32Array.from({ length: 100 }, (_, i) => -100 + i);
            buffer.prepend(data_to_prepend);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.at(-data_to_prepend.length - 13)).toBeNaN();
            expect(data_view.at(-data_to_prepend.length - 1)).toBeNaN();
            expect(data_view.at(-data_to_prepend.length)).toBe(data_to_prepend.at(0));
            expect(data_view.at(-1)).toBe(data_to_prepend.at(-1));
        });

        test('add() once, prepend() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, (_, i) => i);
            const data_to_prepend = Float32Array.from({ length: 100 }, (_, i) => -100 + i);
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.at(-data_to_add.length - data_to_prepend.length - 13)).toBeNaN();
            expect(data_view.at(-data_to_add.length - data_to_prepend.length - 1)).toBeNaN();
            expect(data_view.at(-data_to_add.length - data_to_prepend.length)).toBe(data_to_prepend.at(0));
            expect(data_view.at(-data_to_add.length - 1)).toBe(data_to_prepend.at(-1));
            expect(data_view.at(-data_to_add.length)).toBe(data_to_add.at(0));
            expect(data_view.at(-1)).toBe(data_to_add.at(-1));
        });

        test('add() once, prepend() excessive data', () => {
            const data_to_add = Float32Array.from({ length: 100 }, (_, i) => i);
            const data_to_prepend = Float32Array.from({ length: 1000 }, (_, i) => -1000 + i);
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(buffer._size);

            const data_view = buffer.getView();
            expect(data_view.at(-buffer._size - 13)).toBeNaN();
            expect(data_view.at(-buffer._size - 1)).toBeNaN();
            expect(data_view.at(-buffer._size)).toBe(data_to_prepend.at(data_to_add.length - buffer._size));
            expect(data_view.at(-data_to_add.length - 1)).toBe(data_to_prepend.at(-1));
            expect(data_view.at(-data_to_add.length)).toBe(data_to_add.at(0));
            expect(data_view.at(-1)).toBe(data_to_add.at(-1));
        });

        test('prepend() full, add() with aggregate', () => {
            const data_to_prepend = Float32Array.from({ length: 1000 }, (_, i) => i);
            const data_to_add = Float32Array.from({ length: 100 }, (_, i) => data_to_prepend.length + i);
            const aggregates_count = data_to_add.length / 10;
            buffer.prepend(data_to_prepend);
            buffer.add(data_to_add);

            const data_valid_view = buffer.getValidView();
            expect(data_valid_view.length).toBe(data_to_prepend.length + aggregates_count);

            const data_view = buffer.getView();
            expect(data_view.at(-data_to_prepend.length - aggregates_count - 13)).toBeNaN();
            expect(data_view.at(-data_to_prepend.length - aggregates_count - 1)).toBeNaN();
            expect(data_view.at(-data_to_prepend.length - aggregates_count)).toBe(data_to_prepend.at(0));
            expect(data_view.at(-data_to_prepend.length - aggregates_count + 1)).toBe(data_to_prepend.at(aggregates_count));
            expect(data_view.at(-data_to_prepend.length)).toBe(data_to_prepend.at(10 * aggregates_count));
            expect(data_view.at(-data_to_add.length - 1)).toBe(data_to_prepend.at(-1));
            expect(data_view.at(-data_to_add.length)).toBe(data_to_add.at(0));
            expect(data_view.at(-1)).toBe(data_to_add.at(-1));
        });
    });
});
