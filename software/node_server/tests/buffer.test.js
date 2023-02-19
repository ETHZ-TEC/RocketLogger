"use strict";

import { AggregatingBuffer, AggregatingDataStore, AggregatingBinaryStore, MaxAggregatingDataStore, MinAggregatingDataStore } from '../buffer.js';


describe('AggregatingBuffer class', () => {
    describe('construction', () => {
        test('Int8Array', () => {
            const buffer = new AggregatingBuffer(Int8Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint8Array', () => {
            const buffer = new AggregatingBuffer(Uint8Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint8ClampedArray', () => {
            const buffer = new AggregatingBuffer(Uint8ClampedArray, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Int16Array', () => {
            const buffer = new AggregatingBuffer(Int16Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint16Array', () => {
            const buffer = new AggregatingBuffer(Uint16Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Int32Array', () => {
            const buffer = new AggregatingBuffer(Int32Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint32Array', () => {
            const buffer = new AggregatingBuffer(Uint32Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Float32Array', () => {
            const buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Float64Array', () => {
            const buffer = new AggregatingBuffer(Float64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('BigInt64Array', () => {
            const buffer = new AggregatingBuffer(BigInt64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0n)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('BigUint64Array', () => {
            const buffer = new AggregatingBuffer(BigUint64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0n)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('String', () => {
            const stringAggregatingBuffer = () => {
                new AggregatingBuffer(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer', () => {
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
        });

        test('initial state', () => {
            expect(buffer.size()).toBe(0);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(0);
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every((v, i) => v === data_to_add[i])).toBeTruthy();
        });

        test('add() once NaN', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });
    });

    describe('buffer values (full buffer)', () => {
        let buffer;
        beforeEach(() => {
            buffer = new AggregatingBuffer(Float32Array, 1000, 3, 10);
            buffer._data.forEach((_, i, arr) => { arr[i] = i; });
            buffer._size = buffer._data.length;
        });

        test('initial state', () => {
            expect(buffer.size()).toBe(3000);
            expect(buffer._data.every((v, i) => v === i)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(3000);
            expect(data_view.every((v, i) => v === i)).toBeTruthy();
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(3000);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(3000);
            // 1st buffer section
            expect(data_view.at(0)).toBe(1);
            expect(data_view.at(1)).toBe(2);
            expect(data_view.at(999)).toBe(1000);
            // 2nd buffer section
            expect(data_view.at(1000)).toBe(1010);
            expect(data_view.at(1001)).toBe(1011);
            expect(data_view.at(1989)).toBe(1999);
            expect(data_view.at(1990)).toBe(2000);
            expect(data_view.at(1999)).toBe(2090);
            // 3rd buffer section
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
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Float64Array', () => {
            const buffer = new AggregatingDataStore(Float64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint8ClampedArray', () => {
            const uInt8ClampedArrayAggregatingBuffer = () => {
                new AggregatingDataStore(Uint8ClampedArray, 1000, 3, 10);
            };
            expect(uInt8ClampedArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Uint16Array', () => {
            const uint16ArrayAggregatingBuffer = () => {
                new AggregatingDataStore(Uint16Array, 1000, 3, 10);
            };
            expect(uint16ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Int32Array', () => {
            const int32ArrayAggregatingBuffer = () => {
                new AggregatingDataStore(Int32Array, 1000, 3, 10);
            };
            expect(int32ArrayAggregatingBuffer).toThrow(TypeError);
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
            expect(buffer.size()).toBe(0);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(0);
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every((v, i) => v === data_to_add[i])).toBeTruthy();
        });

        test('add() once NaN', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });

        test('prepend() once', () => {
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('prepend() once NaN', () => {
            const data_to_prepend = new Float32Array(100).fill(NaN);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });


        test('add() once, prepend() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once NaN, prepend() once', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every(isNaN)).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once, prepend() excessive data', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(buffer._capacity);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(buffer._capacity);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
        });

        test('prepend() full, add() with aggregate', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            const aggregates_count = data_to_add.length / buffer._aggregation_factor;
            buffer.prepend(data_to_prepend);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_prepend.length + aggregates_count);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length + aggregates_count);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(aggregates_count, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
            // aggregated values
            expect(data_view.subarray(0, aggregates_count).every((v, i) => v === data_to_prepend[i * buffer._aggregation_factor])).toBeTruthy();
        });
    });
});

describe('AggregatingBinaryStore class', () => {
    describe('construction', () => {
        test('Uint16Array', () => {
            const buffer = new AggregatingBuffer(Uint16Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Int8Array', () => {
            const int8ArrayAggregatingBuffer = () => {
                new AggregatingBinaryStore(Int8Array, 1000, 3, 10);
            };
            expect(int8ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Uint32Array', () => {
            const uint32ArrayAggregatingBuffer = () => {
                new AggregatingBinaryStore(Uint32Array, 1000, 3, 10);
            };
            expect(uint32ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Float32Array', () => {
            const float32ArrayAggregatingBuffer = () => {
                new AggregatingBinaryStore(Float32Array, 1000, 3, 10);
            };
            expect(float32ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Float64Array', () => {
            const float64ArrayAggregatingBuffer = () => {
                new AggregatingBinaryStore(Float64Array, 1000, 3, 10);
            };
            expect(float64ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('String', () => {
            const stringAggregatingBuffer = () => {
                new AggregatingBinaryStore(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer', () => {
            const arrayBufferAggregatingBuffer = () => {
                new AggregatingBinaryStore(ArrayBuffer, 1000, 3, 10);
            };
            expect(arrayBufferAggregatingBuffer).toThrow(TypeError);
        });
    });

    describe('buffer values', () => {
        let buffer;
        beforeEach(() => {
            buffer = new AggregatingBinaryStore(Uint16Array, 1000, 3, 10);
        });

        test('initial state', () => {
            expect(buffer.size()).toBe(0);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(0);
        });

        test('add() once', () => {
            const data_to_add = Uint16Array.from({ length: 100 }, (_, i) => i);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every((v, i) => v === data_to_add[i])).toBeTruthy();
        });

        test('add() full, add() with aggregate', () => {
            const data_to_fill = Uint16Array.from({ length: 1000 }, () => Math.random() * 0xffff);
            const data_to_add = Uint16Array.from({ length: 100 }, () => Math.random() * 0xffff);
            const aggregates_count = data_to_add.length / buffer._aggregation_factor;
            buffer.add(data_to_fill);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_fill.length + aggregates_count);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_fill.length + aggregates_count);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(aggregates_count, -data_to_add.length).every((v, i) => v === data_to_fill[i + data_to_add.length])).toBeTruthy();
            // aggregated values
            expect(data_view.subarray(0, aggregates_count).every((v, i) =>
                (v & 0x00ff) === data_to_fill.subarray(i * buffer._aggregation_factor, (i + 1) * buffer._aggregation_factor).reduce((a, b) => a & b, 0x00ff))).toBeTruthy();
            expect(data_view.subarray(0, aggregates_count).every((v, i) =>
                (v | 0x00ff) === data_to_fill.subarray(i * buffer._aggregation_factor, (i + 1) * buffer._aggregation_factor).reduce((a, b) => a | b, 0x00ff))).toBeTruthy();
        });
    });
});

describe('MaxAggregatingDataStore class', () => {
    describe('construction', () => {
        test('Float32Array', () => {
            const buffer = new MaxAggregatingDataStore(Float32Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Float64Array', () => {
            const buffer = new MaxAggregatingDataStore(Float64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint8ClampedArray', () => {
            const uInt8ClampedArrayAggregatingBuffer = () => {
                new MaxAggregatingDataStore(Uint8ClampedArray, 1000, 3, 10);
            };
            expect(uInt8ClampedArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Uint16Array', () => {
            const uint16ArrayAggregatingBuffer = () => {
                new MaxAggregatingDataStore(Uint16Array, 1000, 3, 10);
            };
            expect(uint16ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Int32Array', () => {
            const int32ArrayAggregatingBuffer = () => {
                new MaxAggregatingDataStore(Int32Array, 1000, 3, 10);
            };
            expect(int32ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('String', () => {
            const stringAggregatingBuffer = () => {
                new MaxAggregatingDataStore(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer', () => {
            const arrayBufferAggregatingBuffer = () => {
                new MaxAggregatingDataStore(ArrayBuffer, 1000, 3, 10);
            };
            expect(arrayBufferAggregatingBuffer).toThrow(TypeError);
        });
    });

    describe('buffer values', () => {
        let buffer;
        beforeEach(() => {
            buffer = new MaxAggregatingDataStore(Float32Array, 1000, 3, 10);
        });

        test('initial state', () => {
            expect(buffer.size()).toBe(0);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(0);
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every((v, i) => v === data_to_add[i])).toBeTruthy();
        });

        test('add() once NaN', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });

        test('prepend() once', () => {
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('prepend() once NaN', () => {
            const data_to_prepend = new Float32Array(100).fill(NaN);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });


        test('add() once, prepend() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once NaN, prepend() once', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every(isNaN)).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once, prepend() excessive data', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(buffer._capacity);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(buffer._capacity);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
        });

        test('prepend() full, add() with aggregate', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            const aggregates_count = data_to_add.length / buffer._aggregation_factor;
            buffer.prepend(data_to_prepend);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_prepend.length + aggregates_count);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length + aggregates_count);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(aggregates_count, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
            // aggregated values
            expect(data_view.subarray(0, aggregates_count).every((v, i) =>
                v === data_to_prepend.subarray(i * buffer._aggregation_factor, (i + 1) * buffer._aggregation_factor).reduce((a, b) => a > b ? a : b))).toBeTruthy();
        });
    });
});

describe('MinAggregatingDataStore class', () => {
    describe('construction', () => {
        test('Float32Array', () => {
            const buffer = new MinAggregatingDataStore(Float32Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Float64Array', () => {
            const buffer = new MinAggregatingDataStore(Float64Array, 1000, 3, 10);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();
            expect(buffer.size()).toBe(0);
        });

        test('Uint8ClampedArray', () => {
            const uInt8ClampedArrayAggregatingBuffer = () => {
                new MinAggregatingDataStore(Uint8ClampedArray, 1000, 3, 10);
            };
            expect(uInt8ClampedArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Uint16Array', () => {
            const uint16ArrayAggregatingBuffer = () => {
                new MinAggregatingDataStore(Uint16Array, 1000, 3, 10);
            };
            expect(uint16ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('Int32Array', () => {
            const int32ArrayAggregatingBuffer = () => {
                new MinAggregatingDataStore(Int32Array, 1000, 3, 10);
            };
            expect(int32ArrayAggregatingBuffer).toThrow(TypeError);
        });

        test('String', () => {
            const stringAggregatingBuffer = () => {
                new MinAggregatingDataStore(String, 1000, 3, 10);
            };
            expect(stringAggregatingBuffer).toThrow(TypeError);
        });

        test('ArrayBuffer', () => {
            const arrayBufferAggregatingBuffer = () => {
                new MinAggregatingDataStore(ArrayBuffer, 1000, 3, 10);
            };
            expect(arrayBufferAggregatingBuffer).toThrow(TypeError);
        });
    });

    describe('buffer values', () => {
        let buffer;
        beforeEach(() => {
            buffer = new MinAggregatingDataStore(Float32Array, 1000, 3, 10);
        });

        test('initial state', () => {
            expect(buffer.size()).toBe(0);
            expect(buffer._data.every(v => v === 0)).toBeTruthy();

            const data_view = buffer.getView();
            expect(data_view.length).toBe(0);
        });

        test('add() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every((v, i) => v === data_to_add[i])).toBeTruthy();
        });

        test('add() once NaN', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_add.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });

        test('prepend() once', () => {
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('prepend() once NaN', () => {
            const data_to_prepend = new Float32Array(100).fill(NaN);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length);
            expect(data_view.every(isNaN)).toBeTruthy();
        });


        test('add() once, prepend() once', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once NaN, prepend() once', () => {
            const data_to_add = new Float32Array(100).fill(NaN);
            const data_to_prepend = Float32Array.from({ length: 100 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(data_to_add.length + data_to_prepend.length);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_add.length + data_to_prepend.length);
            expect(data_view.subarray(-data_to_add.length).every(isNaN)).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i])).toBeTruthy();
        });

        test('add() once, prepend() excessive data', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            buffer.add(data_to_add);
            buffer.prepend(data_to_prepend);

            expect(buffer.size()).toBe(buffer._capacity);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(buffer._capacity);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(0, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
        });

        test('prepend() full, add() with aggregate', () => {
            const data_to_add = Float32Array.from({ length: 100 }, () => Math.random());
            const data_to_prepend = Float32Array.from({ length: 1000 }, () => Math.random());
            const aggregates_count = data_to_add.length / buffer._aggregation_factor;
            buffer.prepend(data_to_prepend);
            buffer.add(data_to_add);

            expect(buffer.size()).toBe(data_to_prepend.length + aggregates_count);

            const data_view = buffer.getView();
            expect(data_view.length).toBe(data_to_prepend.length + aggregates_count);
            expect(data_view.subarray(-data_to_add.length).every((v, i) => v === data_to_add[i])).toBeTruthy();
            expect(data_view.subarray(aggregates_count, -data_to_add.length).every((v, i) => v === data_to_prepend[i + data_to_add.length])).toBeTruthy();
            // aggregated values
            expect(data_view.subarray(0, aggregates_count).every((v, i) =>
                v === data_to_prepend.subarray(i * buffer._aggregation_factor, (i + 1) * buffer._aggregation_factor).reduce((a, b) => a < b ? a : b))).toBeTruthy();
        });
    });
});
