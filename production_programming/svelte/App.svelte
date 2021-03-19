<script>


	import { onMount } from "svelte";

	export let name;


	async function openocd_get_version(){


		try {
			const serial = await fetch('/api/openocd/version', {method: 'GET'}).then((res)=>res.json());
			console.log(serial)
			

		} catch (error) {
			
		}


	}

	async function serialInterval(){


	
		try {
			const serial = await fetch('/api/serial', {method: 'GET'}).then((res)=>res.json());
			console.log(serial)
						
			serial.forEach(element => {

				//var str = String.fromCharCode.apply(null, element.data);
				console.log(element);

				ui_console.data = [...ui_console.data, element];
				let str=element;

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





	};


	onMount(()=>{
	
		setInterval(serialInterval, 1000)

	})

	let ui_console = {

		data: []
	}

	let grid = {

		hwcfg: "???",
		hwcfgstatus: "?",
		mcu: "?",
		mcustatus: "?",
		model: "???",
		serialno: ["?","?","?","?"]

	}

	let mcu_pins = [[
			{number: '1', function: 'none', class: ''},
			{number: '2', function: 'none', class: ''},
			{number: '3', function: 'none', class: ''},
			{number: '4', function: 'none', class: ''},
			{number: '5', function: 'none', class: ''},
			{number: '6', function: 'none', class: ''},
			{number: '7', function: 'none', class: ''},
			{number: '8', function: 'none', class: ''},
			{number: '9', function: 'none', class: ''},
			{number: '10', function: 'none', class: ''},
			{number: '11', function: 'none', class: ''},
			{number: '12', function: 'none', class: ''},
			{number: '13', function: 'none', class: ''},
			{number: '14', function: 'none', class: ''},
			{number: '15', function: 'none', class: ''},
			{number: '16', function: 'none', class: ''},
			{number: '17', function: 'none', class: ''},
			{number: '18', function: 'none', class: ''},
			{number: '19', function: 'none', class: ''},
			{number: '20', function: 'none', class: ''},
			{number: '21', function: 'none', class: ''},
			{number: '22', function: 'none', class: ''},
			{number: '23', function: 'none', class: ''},
			{number: '24', function: 'none', class: ''},
			{number: '25', function: 'none'}
		],
		[
			{number: '26', function: 'none', class: ''},
			{number: '27', function: 'none', class: ''},
			{number: '28', function: 'none', class: ''},
			{number: '29', function: 'none', class: ''},
			{number: '30', function: 'none', class: ''},
			{number: '31', function: 'none', class: ''},
			{number: '32', function: 'none', class: ''},
			{number: '33', function: 'none', class: ''},
			{number: '34', function: 'none', class: ''},
			{number: '35', function: 'none', class: ''},
			{number: '36', function: 'none', class: ''},
			{number: '37', function: 'none', class: ''},
			{number: '38', function: 'none', class: ''},
			{number: '39', function: 'none', class: ''},
			{number: '40', function: 'none', class: ''},
			{number: '41', function: 'none', class: ''},
			{number: '42', function: 'none', class: ''},
			{number: '43', function: 'none', class: ''},
			{number: '44', function: 'none', class: ''},
			{number: '45', function: 'none', class: ''},
			{number: '46', function: 'none', class: ''},
			{number: '47', function: 'none', class: ''},
			{number: '48', function: 'none', class: ''},
			{number: '49', function: 'none', class: ''},
			{number: '50', function: 'none'}
		],
		[
			{number: '51', function: 'none', class: ''},
			{number: '52', function: 'none', class: ''},
			{number: '53', function: 'none', class: ''},
			{number: '54', function: 'none', class: ''},
			{number: '55', function: 'none', class: ''},
			{number: '56', function: 'none', class: ''},
			{number: '57', function: 'none', class: ''},
			{number: '58', function: 'none', class: ''},
			{number: '59', function: 'none', class: ''},
			{number: '60', function: 'none', class: ''},
			{number: '61', function: 'none', class: ''},
			{number: '62', function: 'none', class: ''},
			{number: '63', function: 'none', class: ''},
			{number: '64', function: 'none', class: ''},
			{number: '65', function: 'none', class: ''},
			{number: '66', function: 'none', class: ''},
			{number: '67', function: 'none', class: ''},
			{number: '68', function: 'none', class: ''},
			{number: '69', function: 'none', class: ''},
			{number: '70', function: 'none', class: ''},
			{number: '71', function: 'none', class: ''},
			{number: '72', function: 'none', class: ''},
			{number: '73', function: 'none', class: ''},
			{number: '74', function: 'none', class: ''},
			{number: '75', function: 'none'}
		],
		[
			{number: '76', function: 'none', class: ''},
			{number: '77', function: 'none', class: ''},
			{number: '78', function: 'none', class: ''},
			{number: '79', function: 'none', class: ''},
			{number: '80', function: 'none', class: ''},
			{number: '81', function: 'none', class: ''},
			{number: '82', function: 'none', class: ''},
			{number: '83', function: 'none', class: ''},
			{number: '84', function: 'none', class: ''},
			{number: '85', function: 'none', class: ''},
			{number: '86', function: 'none', class: ''},
			{number: '87', function: 'none', class: ''},
			{number: '88', function: 'none', class: ''},
			{number: '89', function: 'none', class: ''},
			{number: '90', function: 'none', class: ''},
			{number: '91', function: 'none', class: ''},
			{number: '92', function: 'none', class: ''},
			{number: '93', function: 'none', class: ''},
			{number: '94', function: 'none', class: ''},
			{number: '95', function: 'none', class: ''},
			{number: '96', function: 'none', class: ''},
			{number: '97', function: 'none', class: ''},
			{number: '98', function: 'none', class: ''},
			{number: '99', function: 'none', class: ''},
			{number: '100', function: 'none', class: ''}
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
				<div class="pin {pin.class}">{pin.number}</div>
			{/each}
		</div>
		<div class="side rot90">
		{#each mcu_pins[1] as pin}
			<div class="pin {pin.class}">{pin.number}</div>
			{/each}
		</div>


		<div class="side rot180">
			{#each mcu_pins[2] as pin}
				<div class="pin {pin.class}">{pin.number}</div>
			{/each}
		</div>
		<div class="side rot270">
			{#each mcu_pins[3] as pin}
				<div class="pin {pin.class}">{pin.number}</div>
			{/each}
		</div>

	</div>



	<div class="serial_console">
		<div class="consoletitle">Debug UART Console:</div>
		{#each [...ui_console.data].reverse() as entry}
		<div class="consoleline">{entry}</div>
		{/each}		
	</div>


<input type="button" on:click={openocd_get_version} value="Start OpenOCD">

</main>

<style>

	.consoletitle{
		background-color: dodgerblue;
		font-weight: bolder;
		padding: 10px;
	}	
	.consoleline{
		margin-top: 5px;
		background-color: rgb(159, 203, 247);
	}

	.serial_console{
		float: left;
		text-align: left;
		font-family: 'Courier New', Courier, monospace;
		height: 400px;
		width: calc(100% - 450px);
		overflow: auto;
		margin-left: 50px;
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
		background-color:tan;
		width: 40px;
		height: 10px;
		font-size: 50%;
		margin: 2px;
	}

	.pinerror{

		background-color: crimson;

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
		width: 330px;
		height: 330px;
		position: relative;
		
		z-index: 1;
		top: 35px;
		border-radius: 8px;
		left: 35px;

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

	main {
		text-align: center;
		padding: 1em;
		max-width: 120px;
		margin: 0 auto;
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