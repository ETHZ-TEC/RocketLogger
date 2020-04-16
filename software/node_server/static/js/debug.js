"use strict";


/**
 * Initialization when document is fully loaded
 */
$(() => {
	const socket = io('http://' + location.hostname + ':5000/');

	socket.on('connect', () => {
		console.log(`socket.io connect: ${socket.id}`);
		socket.send('hi');
	});
	socket.on('message', (msg) => {
		console.log(msg);
	});
	socket.on('disconnect', () => {
		console.log(`socket.io disconnect: ${socket.id}`);
	});
	
	socket.on('control', (msg) => {
		console.log(`control: ${JSON.stringify(msg)}`);
	});
	socket.on('data', (msg) => {
		let timestamp = new BigInt64Array(msg.ts);
		let data = new Float32Array(msg.data);
		console.log(`data: ${msg.t}, ${timestamp[0]}, [ ${data} ]`);
	});
	socket.on('status', (msg) => {
		console.log(`status: ${JSON.stringify(msg)}`);
	});
});
