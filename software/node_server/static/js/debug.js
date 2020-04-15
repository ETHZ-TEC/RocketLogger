"use strict";

const socket = io('http://' + location.hostname + ':5000/');

/**
 * Initialization when document is fully loaded
 */
$(() => {
	socket.on('connect', () => {
		console.log(`socket.io connect: ${socket.id}`);
		socket.on('disconnect', (msg) => {
			console.log(`socket.io disconnect: ${socket.id}`);
		});
		socket.on('message', (msg) => {
			console.log(msg);
		});
		socket.on('control', (msg) => {
			console.log(`control: ${JSON.stringify(msg)}`);
		});
		socket.on('data', (msg) => {
			let timestamp = new BigInt64Array(msg.ts);
			let data = new Float32Array(msg.data);
			console.log(`data: ${msg.time}, ${timestamp[0]}, [ ${data} ]`);
		});
		socket.on('status', (msg) => {
			console.log(`status: ${JSON.stringify(msg)}`);
		});
		socket.send('hi');
	});
});
