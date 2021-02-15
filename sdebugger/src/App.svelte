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

	
	
	function serialParser(serial){
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
			<div 
				class:heartbeat="{entry.type == 'heartbeat'}"
				class:output="{entry.type == 'output'}"
				class:input="{entry.type == 'input'}">
				{entry.data}
			</div>
		{/each}
	</div>
	
</main>

<style>

	.debug{
		display:flex;
		flex-direction: column;
		height:600px;
		background: #f0f0f0;
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
		color: blue;
		font-weight: bold;
	}

	.input{
		color: black;
		font-weight: bold;
	}

	.heartbeat{
		color: orange;
		font-weight: bold;
	}
</style>