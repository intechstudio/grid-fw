<script>

	import { onMount } from "svelte";

	const url = "ws://localhost:1040";
	let socket = null;

	function start(url){
		socket = new WebSocket(url);
		socket.onmessage = function (event) {
			let message = JSON.parse(event.data);

			let temp = serialParser(message.data);
			if (temp != undefined){			
				serial = [...serial, {type: message.type, data: temp}];
				if (serial.length > 30){
					serial.shift();
				}
			}

		}
		socket.onclose = function (){
			socket = null;
			setTimeout(function (){start(url)},1000)
		}
	}

	var testchart = create_chart("testchart", 250,250,0,1000);


	let charts = [];

	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));
	charts.push(create_chart(100, 200, 0, 1000));

	let serial = [];

	let brc = []; // must be array, to be encoded by Grid Editor
	let command = '';

	
	function hexdump(buffer, blockSize) {
		
		if(typeof buffer === 'string'){
			//console.log("buffer is string");
			//do nothing
		}else if(buffer instanceof ArrayBuffer && buffer.byteLength !== undefined){
			//console.log("buffer is ArrayBuffer");
			buffer = String.fromCharCode.apply(String, [].slice.call(new Uint8Array(buffer)));
		}else if(Array.isArray(buffer)){
			//console.log("buffer is Array");
			buffer = String.fromCharCode.apply(String, buffer);
		}else if (buffer.constructor === Uint8Array) {
			//console.log("buffer is Uint8Array");
			buffer = String.fromCharCode.apply(String, [].slice.call(buffer));
		}else{
			//console.log("Error: buffer is unknown...");
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


	function create_chart(cw, ch, minValue, maxValue){

		var new_chart = {
		
		chartwidth: cw,
			chartheight: ch,
			minvalue: minValue,
			maxvalue: maxValue,
		slope: 0,

		values: [],

		points: [],

		render: function(){

			this.slope = (this.chartheight - 0) / (this.maxvalue - this.minvalue);


			for (let j=0; j<this.values[0].length; j++){
				var pointsstring = "";

				for(let i = 0; i<this.values.length; i++){

					var input = this.values[i][j];
					var output = this.chartheight - this.slope * (input - this.minvalue);

					pointsstring += (i*(this.chartwidth/(this.values.length-1))) + "," + output+" ";

				}

				this.points[j] = pointsstring;

				//document.getElementById("chart_"+this.id+"_"+[j]).setAttribute("points", pointsstring);

				//console.log(this.points[0]);

			}


		}
		};

		return new_chart;
	}



	function serialParser(serial){
		


		let stx = String.fromCharCode(2);
		let etx = String.fromCharCode(3);

		let buffer =  String.fromCharCode.apply(String, serial);

		let search = buffer.search(stx+"021d")

		if (search != -1){

			//console.log(buffer)

			let trim =  buffer.split(etx);
			let parts = trim[0].split('!');
			parts.shift();


			for (let i = 0; i<charts.length; i++){

				let topush = parts[i].split(',');

				charts[i].name = topush[0];
				charts[i].min = topush[1];
				charts[i].avg = topush[2];
				charts[i].max = topush[3];
				charts[i].values.push([topush[1], topush[2], topush[3]]);
				charts[i].render();

				if (charts[i].values.length > 100){
					charts[i].values.shift();
				}

			}





			//console.log(parts);
			//console.log(search);
			
			//return '';

		}
		else{

			//console.log("no");
		
			let temp = String.fromCharCode.apply(String, serial);
			console.log(temp)
			
		    return hexdump( serial , 16 );

			
		}




		// SUKU MAGIC:



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

	<div class="charts">

		{#each charts as entry}

			<svg class="chart" height="{entry.chartheight}" width="{entry.chartwidth}" viewBox="0 0 {entry.chartwidth} {entry.chartheight}">
				<polyline id="chart_testchart_0"
				fill="none"
				stroke="#00d974"
				stroke-width="1"
				points="{entry.points[0]}"/>
		
				<polyline id="chart_testchart_1"
				fill="none"
				stroke="#0074d9"
				stroke-width="1"
				points="{entry.points[1]}"/>
		
				<polyline id="chart_testchart_2"
				fill="none"
				stroke="#d97400"
				stroke-width="1"
				points="{entry.points[2]}"/>
		
				<text class="name" x="0" y="15" fill="black">{entry.name}</text>
				<text class="max" x="0" y="25%" fill="red">{entry.max}</text>
				<text class="avg" x="0" y="50%" fill="red">{entry.avg}</text>
				<text class="min" x="0" y="75%" fill="red">{entry.min}</text>
			</svg>

		{/each}

	</div>




</main>

<style>

	.charts{
		position: absolute;
		bottom: 0px;
		left: 0px;
		width: 100%;
	}

	.chart{

		margin: 5px;
		border: 1px solid rgba(0,0,0,0.2);
		background-color: white;
	}

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
		height:500px;
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