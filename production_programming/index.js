const express = require('express')
const app = express();
const path = require('path');
const localhostport = 3000

const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')
const uart = new SerialPort('/dev/ttyUSB0', {
  baudRate: 2000000
})

app.use(express.json())


const { spawn } = require('child_process');



const parser = uart.pipe(new Readline({ delimiter: '\r\n' }))



var consoleData = [];


parser.on('data', function(line){
  //console.log(line);
  consoleData.push({context:"uart", data:line});
})

// Read data that is available but keep the stream in "paused mode"
uart.on('readable', function () {
  uart.read();
  //console.log('Data:', uart.read())
})



// Switches the uart into "flowing mode"
uart.on('data', function (data) {
  //console.log('yes:', data)
})

app.get('/api/console', function (req, res) {
  var temp = [];
  temp = consoleData;
  consoleData = [];
  res.send(temp);
})


app.post('/api/uart/send', function (req, res) {

  console.log(req.body.data);
  
  uart.write(req.body.data);

  consoleData.push({context:"uart", data: req.body.data});

  // var temp = [];
  // temp = consoleData;
  // consoleData = [];
  res.send("OK");
})

let openocd;
let telnet;
let fuser;

app.get('/api/openocd/start', function (req, res) {
  consoleData.push({context:"openocd", data:"openocd start!"});

  openocd = spawn('openocd', {cwd: '../build_scripts'});

  openocd.stdout.on('data', (data) => {
    console.log(`openocd::stdout: ${data}`);
    consoleData.push({context:"openocd", data:data});
  });
  
  openocd.stderr.on('data', (data) => {
    //console.error(`openocd::stderr: ${data}`);
    console.log(String.fromCharCode(...data));
    consoleData.push({context:"openocd", data:String.fromCharCode(...data)});
  });
  
  openocd.on('close', (code) => {
    console.log(`openocd exited with code ${code}`);
    consoleData.push({context:"openocd", data:`openocd exited with code ${code}`});
  });
  
}) 

app.get('/api/fuser/kill', function (req, res) {
  consoleData.push({context:"fuser", data:"fuser kill!"});

  fuser = spawn('fuser', ['-k','6666/tcp', '-k','4444/tcp', '-k','3333/tcp', ]);

  fuser.stdout.on('data', (data) => {
    consoleData.push({context:"fuser", data:data});
  });
  
  fuser.stderr.on('data', (data) => {
    consoleData.push({context:"fuser", data:String.fromCharCode(...data)});
  });
  
  fuser.on('close', (code) => {
    console.log(`fuser exited with code ${code}`);
    consoleData.push({context:"fuser", data:`fuser exited with code ${code}`});
  });
  
}) 

app.get('/api/openocd/stop', function (req, res) {

  console.log("try stop");
  if (openocd!=undefined){
    if (!openocd.killed){
      consoleData.push({context:"openocd", data: "openocd killed!"});
      openocd.kill();
    }
  }

}) 


app.get('/api/telnet/start', function (req, res) {


  telnet = spawn('telnet', ['localhost', '4444']);
  consoleData.push({context:"telnet", data:"telnet start!"});

  telnet.stdout.on('data', (data) => {
    //console.log(`openocd::stdout: ${data}`);
    consoleData.push({context:"telnet", data:String.fromCharCode(...data)});
  });
  
  telnet.stderr.on('data', (data) => {
    //console.error(`openocd::stderr: ${data}`);
    //console.log(...data);
    consoleData.push({context:"telnet", data:String.fromCharCode(...data)});
  });
  
  telnet.on('close', (code) => {
    //console.log(`openocd exited with code ${code}`);
    consoleData.push({context:"telnet", data:`telnet exited with code ${code}`});
  });
  

}) 

app.get('/api/telnet/stop', function (req, res) {

  console.log("try start");
  if (telnet!=undefined){
    if (!telnet.killed){
      consoleData.push({context:"telnet", data:"telnet killed!"});
      telnet.kill();
    }
  }

}) 

app.post('/api/telnet/send', function (req, res) {

  
  if (telnet!=undefined){
    if (!telnet.killed){
      if(telnet) telnet.stdin.write(req.body.data+"\n");
    }
  }


  //consoleData.push({context:"telnet", data: req.body.data});

  // var temp = [];
  // temp = consoleData;
  // consoleData = [];
  res.send("OK");
})


//app.use('/', express.static('public'))
//app.use('/static', express.static('public'))

app.use(express.static('public'));
app.get('*', (req, res) => {
   res.sendFile(path.resolve(__dirname, 'public', 'index.html'));
})

app.listen(localhostport, '0.0.0.0', () => {
  console.log(`Example app listening at http://localhost:${localhostport}`)
})
 
