<script>


	import { onMount } from "svelte";
	import { element } from "svelte/internal";

	export let name;


	let uart_input_field = "";
	let telnet_input_field = "";
	let telnet_history = [];
	let telnet_history_index = 0;


	let uart_console_enabled = true;
	let openocd_console_enabled = true;
	let telnet_console_enabled = true;

	function keyup_telnet(e){
		if (e.keyCode === 13) {
  			e.preventDefault();
			telnet_send();
		}	
		if (e.keyCode === 38) {

			if (telnet_history_index < telnet_history.length){

				telnet_history_index++;
			}

			telnet_input_field = telnet_history[telnet_history.length-(telnet_history_index-1)-1];

		}	
		if (e.keyCode === 40) {

			if (telnet_history_index > 0){

				telnet_history_index--;
			}
			telnet_input_field = telnet_history[telnet_history.length-(telnet_history_index)];


		}	
	}

	async function fuser_kill(){
		try {
			const serial = await fetch('/api/fuser/kill', {method: 'GET'}).then((res)=>res.json());
			//console.log(serial)
		} catch (error) {
			
		}
	}
	async function openocd_start(){
		try {
			const serial = await fetch('/api/openocd/start', {method: 'GET'}).then((res)=>res.json());
			//console.log(serial)
		} catch (error) {
			
		}
	}
	async function openocd_stop(){
		try {
			const serial = await fetch('/api/openocd/stop', {method: 'GET'}).then((res)=>res.json());
			//console.log(serial)
		} catch (error) {
			
		}
	}
	async function telnet_start(){
		try {
			const serial = await fetch('/api/telnet/start', {method: 'GET'}).then((res)=>res.json());
			//console.log(serial)
		} catch (error) {
			
		}
	}
	async function telnet_stop(){
		try {
			const serial = await fetch('/api/telnet/stop', {method: 'GET'}).then((res)=>res.json());
			//console.log(serial)
		} catch (error) {
			
		}
	}
	async function telnet_send(){

		telnet_history.push(telnet_input_field);
		telnet_history_index = 0;

		try {
			const serial = await fetch('/api/telnet/send', {
				method: 'POST', 
				body: JSON.stringify({data: telnet_input_field}), 
				headers: {
					'Accept': 'application/json',
					'Content-Type': 'application/json'
				}
			}).then((res)=>{
				console.log(res)
				telnet_input_field = "";
				return;
				
			});
			

		} catch (error) {
			
		}


	}
	async function uart_send(){

		console.log(uart_input_field);

		try {
			const serial = await fetch('/api/uart/send', {
				method: 'POST', 
				body: JSON.stringify({data: uart_input_field}), 
				headers: {
					'Accept': 'application/json',
					'Content-Type': 'application/json'
				}
			}).then((res)=>{
				console.log(res)
				uart_input_field = "";
				return;
				
			});
			

		} catch (error) {
			
		}


	}

	async function consolePoll(){

		let serial = []
	
		try {
			 serial = await fetch('/api/console', {method: 'GET'}).then((res)=>res.json());
			
			 serial.forEach(element => {
				//var str = String.fromCharCode.apply(null, element.data);
				console.log(element);

				ui_console = [element,...ui_console];

				//ui_console.data = [...ui_console.data, element.data];
				//ui_console.context = [...ui_console.context, element.context];


				let str = element.data;

				if (str.startsWith("test")){

					if (str.startsWith("test.boundary.")){

						var test_result = str.substring(16, 16+25);
						console.log("Boundary Test Result is Here!");
						console.log(parseInt(str.substring(14,15)));
						console.log(test_result);
						


						var indices = [];
						for(var i=0; i<test_result.length;i++) {
							if (test_result[i] === "1") indices.push(i);
						}
						console.log(indices);

						for (var i=0; i<indices.length; i++){

							let side = parseInt(str.substring(14,15));
							mcu_pins[side][indices[i]].class += "pinerror"

						}
						
						
					}
					else if(str.startsWith("test.hwcfg.")){
						
						grid.hwcfg = parseInt(str.substring(11,str.length));

						if(grid.hwcfg == 192){grid.model = "EN16 RevA"; grid.hwcfgstatus = "OK";}

					}
					else if(str.startsWith("test.serialno.")){
						
						grid.serialno = (str.substring(14,str.length)).split(" ");


					}
					else if(str.startsWith("test.mcu.")){
						
						grid.mcu = str.split(".")[2];

						if (grid.mcu == "ATSAMD51N20A"){
							grid.mcustatus = "OK";
						}

					}

					
				}


				});
			

		} catch (error) {
			
		}

	}
		


	onMount(()=>{
	
		setInterval(consolePoll, 1000)

	})

	let ui_console = [{context: "uart", data: "123"},{context: "telnet", data: "456"}, ];

	let grid = {

		hwcfg: "???",
		hwcfgstatus: "?",
		mcu: "?",
		mcustatus: "?",
		model: "???",
		serialno: ["?","?","?","?"]

	}

	let mcu_pins = [[
			{number: '1', function: '1', class: ''},
			{number: '2', function: '2', class: ''},
			{number: '3', function: '3', class: ''},
			{number: '4', function: '4', class: ''},
			{number: '5', function: '5', class: ''},
			{number: '6', function: '6', class: ''},
			{number: '7', function: '7', class: ''},
			{number: '8', function: '8', class: ''},
			{number: '9', function: '9', class: ''},
			{number: '10', function: '10', class: ''},
			{number: '11', function: 'GND', class: 'pingnd'},
			{number: '12', function: 'VDD', class: 'pinpwr'},
			{number: '13', function: '13', class: ''},
			{number: '14', function: 'SYNC2', class: ''},
			{number: '15', function: 'WEST TX', class: ''},
			{number: '16', function: 'WEST RX', class: ''},
			{number: '17', function: '17', class: ''},
			{number: '18', function: '18', class: ''},
			{number: '19', function: '19', class: ''},
			{number: '20', function: '20', class: ''},
			{number: '21', function: '21', class: ''},
			{number: '22', function: '22', class: ''},
			{number: '23', function: '23', class: ''},
			{number: '24', function: 'GND', class: 'pingnd'},
			{number: '25', function: 'VDD', class: 'pinpwr'}
		],
		[
			{number: '26', function: 'QSPI IO 0', class: ''},
			{number: '27', function: 'QSPI IO 1', class: ''},
			{number: '28', function: 'QSPI IO 2', class: ''},
			{number: '29', function: 'QSPI IO 3', class: ''},
			{number: '30', function: 'VDD', class: 'pinpwr'},
			{number: '31', function: 'GND', class: 'pingnd'},
			{number: '32', function: 'QSPI SCK', class: ''},
			{number: '33', function: 'QSPI CE', class: ''},
			{number: '34', function: '34', class: ''},
			{number: '35', function: 'HWC SH', class: ''},
			{number: '36', function: 'HWC CLK', class: ''},
			{number: '37', function: 'HWC DAT', class: ''},
			{number: '38', function: 'GND', class: 'pingnd'},
			{number: '39', function: 'VDD', class: 'pinpwr'},
			{number: '40', function: '40', class: ''},
			{number: '41', function: 'MAPMODE', class: ''},
			{number: '42', function: 'SOUTH RX', class: ''},
			{number: '43', function: 'SOUTH TX', class: ''},
			{number: '44', function: 'UI PWR EN', class: ''},
			{number: '45', function: '45', class: ''},
			{number: '46', function: '46', class: ''},
			{number: '47', function: '47', class: ''},
			{number: '48', function: '48', class: ''},
			{number: '49', function: '49', class: ''},
			{number: '50', function: 'GND', class: 'pingnd'}
		],
		[
			{number: '51', function: 'VDD', class: 'pinpwr'},
			{number: '52', function: '52', class: ''},
			{number: '53', function: '53', class: ''},
			{number: '54', function: '54', class: ''},
			{number: '55', function: '55', class: ''},
			{number: '56', function: 'EAST RX', class: ''},
			{number: '57', function: 'EAST TX', class: ''},
			{number: '58', function: 'SYNC1', class: ''},
			{number: '59', function: '59', class: ''},
			{number: '60', function: '60', class: ''},
			{number: '61', function: '61', class: ''},
			{number: '62', function: '62', class: 'pingnd'},
			{number: '63', function: '63', class: 'pinpwr'},
			{number: '64', function: '64', class: ''},
			{number: '65', function: '65', class: ''},
			{number: '66', function: '66', class: ''},
			{number: '67', function: '67', class: ''},
			{number: '68', function: '68', class: ''},
			{number: '69', function: '69', class: ''},
			{number: '70', function: '70', class: ''},
			{number: '71', function: '71', class: ''},
			{number: '72', function: 'SYS SCL', class: ''},
			{number: '73', function: 'SYS SDA', class: ''},
			{number: '74', function: 'USB DN', class: ''},
			{number: '75', function: 'USB DP', class: ''}
		],
		[
			{number: '76', function: 'GND', class: 'pingnd'},
			{number: '77', function: 'PWR', class: 'pinpwr'},
			{number: '78', function: '78', class: ''},
			{number: '79', function: '79', class: ''},
			{number: '80', function: 'DBG RX', class: ''},
			{number: '81', function: 'DBG TX', class: ''},
			{number: '82', function: 'SYS INT 0', class: ''},
			{number: '83', function: '83', class: ''},
			{number: '84', function: '84', class: ''},
			{number: '85', function: 'NORTH TX', class: ''},
			{number: '86', function: 'NORTH RX', class: ''},
			{number: '87', function: '87', class: ''},
			{number: '88', function: 'RESET', class: 'pinrst'},
			{number: '89', function: 'CORE', class: 'pinpwr'},
			{number: '90', function: 'GND', class: 'pingnd'},
			{number: '91', function: 'VSW', class: 'pinpwr'},
			{number: '92', function: 'VDD', class: 'pinpwr'},
			{number: '93', function: 'SWCLK', class: ''},
			{number: '94', function: 'SWDIO', class: ''},
			{number: '95', function: 'SWO LED', class: ''},
			{number: '96', function: '96', class: ''},
			{number: '97', function: '97', class: ''},
			{number: '98', function: '98', class: ''},
			{number: '99', function: '99', class: ''},
			{number: '100', function: '100', class: ''}
		]
	
	];

</script>

<main>

	<div class="boundary_check container">

		<div class="chip">

		    <div class="chip_info">
				<table border="1" width="300px">

					<tr>
						<td>MCU: </td><td>{grid.mcu}</td><td>{grid.mcustatus}</td>
					</tr>	

					<tr>
						<td>HWCFG:</td><td>{grid.hwcfg}</td><td>{grid.hwcfgstatus}</td>
					</tr>
					<tr>
						<td>Model:</td><td colspan="2">{grid.model}</td>
					</tr>

					<tr>
						<td>S/N:</td><td>{grid.serialno[0]}</td><td>{grid.serialno[1]}</td>
					</tr>
					<tr>
						<td></td><td>{grid.serialno[2]}</td><td>{grid.serialno[3]}</td>
					</tr>

				</table>

			</div>

		</div>

		<div class="side rot0">
			{#each mcu_pins[0] as pin}
				<div class="pin {pin.class}">{pin.function}</div>
			{/each}
		</div>
		<div class="side rot90">
		{#each mcu_pins[1] as pin}
			<div class="pin {pin.class}">{pin.function}</div>
			{/each}
		</div>


		<div class="side rot180">
			{#each mcu_pins[2] as pin}
				<div class="pin {pin.class}">{pin.function}</div>
			{/each}
		</div>
		<div class="side rot270">
			{#each mcu_pins[3] as pin}
				<div class="pin {pin.class}">{pin.function}</div>
			{/each}
		</div>

	</div>



	
	<div class="serial_container">
		<div class="serial_console">
			{#each ui_console as entry}
				{#if entry.context == "uart" && uart_console_enabled}
					<div class="consoleline {entry.context}">{entry.data}</div>
				{/if}
				{#if entry.context == "openocd" && openocd_console_enabled}
					<div class="consoleline {entry.context}">{entry.data}</div>
				{/if}
				{#if entry.context == "telnet" && telnet_console_enabled}
					<div class="consoleline {entry.context}">{entry.data}</div>
				{/if}
				{#if entry.context == "fuser" }
					<div class="consoleline {entry.context}">{entry.data}</div>
				{/if}
			{/each}	
		</div>
	</div>



	<div>

		
	
		<input type="button" on:click={fuser_kill} value="Reset Ports">

		<input type="button" on:click={openocd_start} value="OpenOCD Start">
		<input type="button" on:click={openocd_stop} value="OpenOCD Stop">
		<input type="button" on:click={telnet_start} value="Telnet Start">
		<input type="button" on:click={telnet_stop} value="Telnet Stop">
	</div>
	<div>
		<input type="text"  bind:value={uart_input_field} placeholder="UART command">
		<input type="button" on:click={uart_send} value="Send">

	</div>
	<div>
		<input type="text" on:keyup={keyup_telnet}  bind:value={telnet_input_field} placeholder="Telnet command">
		<input type="button" on:click={telnet_send} value="Send">

		<input name="uart_enable" id="uart_enable" type="checkbox" bind:checked={uart_console_enabled}><label for="uart_enable">UART</label>
		<input name="openocd_enable" id="openocd_enable" type="checkbox" bind:checked={openocd_console_enabled}><label for="openocd_enable">OpenOCD</label>
		<input name="telnet_enable" id="telnet_enable" type="checkbox" bind:checked={telnet_console_enabled}><label for="telnet_enable">Telnet</label>
	
	</div>
	<div>
		Chip: 
		<input type="button" on:click={function(){telnet_input_field = "reset";}} value="reset">
		<input type="button" on:click={function(){telnet_input_field = "reset init";}} value="reset init">
		<input type="button" on:click={function(){telnet_input_field = "atsame5 chip-erase";}} value="erase">

	</div>
	<div>
		Bootloader: 
		<input type="button" on:click={function(){telnet_input_field = "atsame5 bootloader";}} value="check">
		<input type="button" on:click={function(){telnet_input_field = "atsame5 bootloader 16384";}} value="lock">
		<input type="button" on:click={function(){telnet_input_field = "atsame5 bootloader 0";}} value="unlock">
		<input type="button" on:click={function(){telnet_input_field = "program bootloader-intech_grid-v3.3.0-8-g945e9ec-dirty.elf verify";}} value="install">

	</div>
	<div>
		Firmware: 
		<input type="button" on:click={function(){telnet_input_field = "program ../grid_make/gcc/AtmelStart.bin verify 0x4000";}} value="install">

	</div>

</main>

<style>
	label {
		/* Other styling... */
		display: inline-block;
	}
	.consoleline{
		padding-left: 10px;
	}

	.consoleline.telnet{
		background-color: rgb(0, 203, 247);
	}
	.consoleline.uart{
		background-color: rgb(58, 247, 0);
	}
	.consoleline.openocd{
		background-color: rgb(247, 0, 214);
	}

	.serial_container{
		display:flex;
		flex-direction:column;
		height: 400px;
		width: calc(100% - 450px);	
	}

	.serial_console{
		text-align: left;
		font-family: 'Courier New', Courier, monospace;
		font-size: 75%;
		display:flex;
		flex-direction:column-reverse;
		height: 100%;
		width: 100%;
		overflow: auto;			
	}


	.rot0{
		transform: rotate(0deg);
		top:49px;
	}
	.rot90{
		transform: rotate(-90deg);
		bottom: -125px;
		left: 175px;
	}
	.rot180{
		transform: rotate(-180deg);
		right: 0px;
		bottom: 49px;
	}
	.rot270{
		transform: rotate(-270deg);
		right: 175px;
		top: -125px;
	}

	.pin{
		background-color:lightgray;
		width: 45px;
		height: 10px;
		font-size: 50%;
		margin: 2px;
		text-align: center;
	}

	.pinerror{

		background-color: rgba(127,0,0,0.4);
		height: 10px;
		margin-left: -6px;
	}	
	
	.rot0 > .pinerror{
		border-left: 8px solid red;
	}	
	.rot90 > .pinerror{
		border-left: 8px solid red;
	}	
	.rot180 > .pinerror{
		border-right: 8px solid red;
	}	
	.rot270 > .pinerror{
		border-right: 8px solid red;
	}


	.pinpwr{

		background-color:rgb(255, 208, 147);

	}
	.pinrst{

		background-color:rgb(125, 216, 125);

	}

	.pingnd{

		background-color:rgb(147, 154, 255);

	}

	.rot180 > .pin{
		transform: rotate(-180deg);
	}
	
	.rot270 > .pin{
		transform: rotate(-180deg);
	}

	.side{
		
		width: 50px;
		height: 302px;
		position: absolute;
		

	}

	.chip{
		
		background-color:rgb(62, 71, 80);
		color: white;
		width: 300px;
		height: 300px;
		position: relative;
		
		z-index: 1;
		top: 50px;
		border-radius: 8px;
		left: 50px;

	}

	.chip_info{
		margin: 0;
		position: absolute;
		top: 50%;
		-ms-transform: translateY(-50%) translateX(-50%);
		transform: translateY(-50%) translateX(-50%);
		left: 50%;
		text-align: left;
	}

	.boundary_check {
		float: left;
		position: relative;
		height: 400px;
		width: 400px;
	}


	h1 {
		color: #ff3e00;
		text-transform: uppercase;
		font-size: 4em;
		font-weight: 100;
	}

	@media (min-width: 640px) {
		main {
			max-width: none;
		}
	}
</style>