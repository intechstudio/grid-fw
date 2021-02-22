<script>

	import { onMount } from "svelte";

	const url = "ws://localhost:1040";
	let socket = null;

	function start(url){
		socket = new WebSocket(url);
		socket.onmessage = function (event) {
			let message = JSON.parse(event.data);
			serial = [...serial, {type: message.type, data: serialParser(message.data)}];
		}
		socket.onclose = function (){
			socket = null;
			setTimeout(function (){start(url)},1000)
		}
	}

	let serial = [];

	let brc = []; // must be array, to be encoded by Grid Editor
	let command = '';

	
	function hexdump(buffer, blockSize) {
		
		if(typeof buffer === 'string'){
			console.log("buffer is string");
			//do nothing
		}else if(buffer instanceof ArrayBuffer && buffer.byteLength !== undefined){
			console.log("buffer is ArrayBuffer");
			buffer = String.fromCharCode.apply(String, [].slice.call(new Uint8Array(buffer)));
		}else if(Array.isArray(buffer)){
			console.log("buffer is Array");
			buffer = String.fromCharCode.apply(String, buffer);
		}else if (buffer.constructor === Uint8Array) {
			console.log("buffer is Uint8Array");
			buffer = String.fromCharCode.apply(String, [].slice.call(buffer));
		}else{
			console.log("Error: buffer is unknown...");
			return false;
		}
		
		
		blockSize = blockSize || 16;
		var lines = [];
		var lines_addr = [];
		var lines_hex = [];
		var lines_chars = [];
		var hex = "0123456789ABCDEF";
		for (var b = 0; b < buffer.length; b += blockSize) {
			var block = buffer.slice(b, Math.min(b + blockSize, buffer.length));
			var addr = ("0000" + b.toString(16)).slice(-4);
			var codes = block.split('').map(function (ch) {
				var code = ch.charCodeAt(0);
				return " " + hex[(0xF0 & code) >> 4] + hex[0x0F & code];
			}).join("");
			codes += "   ".repeat(blockSize - block.length);
			var chars = block.replace(/[\x00-\x1F\x20]/g, '.');
			chars +=  " ".repeat(blockSize - block.length);
			//lines.push(addr + " " + codes + "  " + chars);
			lines_addr.push(addr);
			lines_hex.push(codes);
			lines_chars.push(chars);

			lines.push(codes);
		}


		var ret = [lines_addr.join("\n"),lines_hex.join("\n"),lines_chars.join("\n")];

		return ret;
	}

//tests:
var buffer = 'very very long string; very very long string; very very long string;';		//long string	- working
console.log( hexdump( buffer , 16 ) ) ;

var buffer = new Uint8Array([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]).buffer;	//arrayBuffer	- working
console.log( hexdump( buffer , 16 ) ) ;

var buffer = new Uint8Array([21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40]);	//Uint8Array	- working
console.log( hexdump( buffer , 16 ) ) ;

var buffer = [41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60];			//bytearray	- working
console.log( hexdump( buffer , 16 ) ) ;


	function serialParser(serial){
		
		// SUKU MAGIC:
		return hexdump( serial , 16 );


		// TOFI MAGIC:
		let _serial = ''
		_serial = serial.map(arg => {
			return String.fromCharCode(arg)
		});
		_serial = _serial.join('');
		return _serial;
	}

	function heartbeat(e){
		socket.send(JSON.stringify({type: 'filter', data: {class: 'heartbeat', status: !e.target.checked}}))
	}

	function sendSerialCommand(){
		socket.send(JSON.stringify({type: 'command', data: {brc: brc, command: command}}))
	}

	function clear(){
		serial = []
	}

	onMount(()=>{
		start(url);
	})

</script>

<main>
	<h1>Grid debug project</h1>

	<button on:click={clear}>clear</button>

	<div class="debug-control-block">
		<div class="input-block">
			<input id="heartbeat" type="checkbox" checked on:change={(e)=>{heartbeat(e)}}>
			<label for="heartbeat">heartbeat</label>
		</div>

		<div class="input-block">
			<label for="dx">dx</label>
			<input type="text" id="dx" class="" bind:value={brc[0]}>
		</div>

		<div class="input-block">
			<label for="dy">dy</label>
			<input type="text" id="dy" class=""  bind:value={brc[1]}>
		</div>

		<div class="input-block">
			<label for="age">age</label>
			<input type="text" id="age" class=""  bind:value={brc[2]}>
		</div>

		<div class="input-block">
			<label for="rot">rot</label>
			<input type="text" id="rot" class=""  bind:value={brc[3]}>
		</div>

		<div class="input-block">
			<label for="command">command</label>
			<input type="text" id="command" class=""  bind:value={command}>
		</div>

		<input type="button" on:click={sendSerialCommand} value="Send Command">
	</div>

	<div class="debug">
		
		{#each [...serial].reverse() as entry}
			<div style="display: flex; margin-bottom:10px;"
				class:heartbeat="{entry.type == 'heartbeat'}"
				class:output="{entry.type == 'output'}"
				class:input="{entry.type == 'input'}">

				<div class="dump addr">{entry.data[0]}</div>
				<div class="dump codes">{entry.data[1]}</div>
				<div class="dump chars">{entry.data[2]}</div>

				
			</div>
		{/each}
	</div>
	
</main>

<style>

	.addr{
		display:flex;
		flex-direction: column;
		background: rgba(0,0,0,0.2);
		color: #fff;
		width: 40px;
		margin-right: 20px;

	}	
	.codes{
		display:flex;
		flex-direction: column;
		/* background: #37b642; */
		width: 360px;

	}	
	.chars{
		display:flex;
		flex-direction: column;
		/* background: #374ab6; */
		width: 120px;


	}

	.debug{
		height:600px;
		font-family: monospace;
		overflow: scroll;
		justify-content: flex-start;
		text-align: left;
	}

	.input-block{
		display:flex;
		flex-direction: row;
		justify-content: flex-start;
		align-items: center;
		margin-left: 8px;
		margin-right: 8px;
	}

	.input-block label{
		margin-left: 4px;
		margin-right: 8px;
	}

	input{
		padding: 0;
		padding: 2px;
		margin: 0;
	}

	#command{
		width:120px;
	}

	input[type=text]{
		width: 40px;
	}

	input[type=button]{
		padding-left:16px;
		padding-right:16px;
	}

	input[type=button]:active{
		background-color: yellow;
		outline: none;
	}

	.debug-control-block{
		display:flex;
		flex-direction: row;
		margin-top: 8px;
		margin-bottom: 8px;
	}



	@media (min-width: 640px) {
		main {
			max-width: none;
		}
	}

	.output{
		background-color: rgb(208, 208, 255);
		font-weight: bold;
	}

	.input{
		background-color: rgb(208, 255, 218);
		font-weight: bold;
	}

	.heartbeat{
		background-color: rgb(255, 230, 215);
		font-weight: bold;
	}
</style>