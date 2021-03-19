const express = require('express')
const app = express();
const path = require('path');
const localhostport = 3000

const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')
const port = new SerialPort('/dev/ttyUSB0', {
  baudRate: 2000000
})


const { spawn } = require('child_process');



const parser = port.pipe(new Readline({ delimiter: '\r\n' }))



var serialdata = [];


parser.on('data', function(line){
  console.log(line);
  serialdata.push(line);
})

// Read data that is available but keep the stream in "paused mode"
port.on('readable', function () {
  port.read();
  //console.log('Data:', port.read())
})



// Switches the port into "flowing mode"
port.on('data', function (data) {
  //console.log('yes:', data)
})

app.get('/api/serial', function (req, res) {
  var temp = [];
  temp = serialdata;
  serialdata = [];
  res.send(temp);
})



app.get('/api/openocd/version', function (req, res) {

  const openocd = spawn('openocd', ['-v']);


  openocd.stdout.on('data', (data) => {
    console.log(`openocd::stdout: ${data}`);
  });
  
  openocd.stderr.on('data', (data) => {
    console.error(`openocd::stderr: ${data}`);
    //console.log(...data);
    serialdata.push(String.fromCharCode(...data));
  });
  
  openocd.on('close', (code) => {
    console.log(`openocd exited with code ${code}`);
  });
  

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
 
