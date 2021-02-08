const express = require('express')
const app = express()
const path = require('path')
const bodyParser = require('body-parser');
require('dotenv').config()

const SerialPort = require('serialport')
const Readline = SerialPort.parsers.Readline;

const port = 3000

app.use(express.json());

app.use(bodyParser.urlencoded({ extended: true })); 

// pass back data for the client
app.use('/data', (req,res,next)=>{
  res.json('ok');
});

var serialData = [];

app.use('/serial', (req,res,next)=>{

  serialData = [];

  var path = req.body.port;
  var baud = req.body.baud;

  const serialport = new SerialPort(path, {baudRate: +baud});

  serialport.on('open', function() {
    console.log('Port is open.', serialport.path);
  });

  const parser = serialport.pipe(new Readline({ delimiter: "\r\n"}))

  parser.on('data', function(data) {
    serialData.push(data.toString());
  });

})

app.use('/get_data', (req,res,next)=>{
  res.send(serialData);
  serialData = [];
});

app.use('/static', express.static(__dirname + '/static'));



app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname + '/index.html'));
});


// Start up the Node server
app.listen(port, () => { 
  console.log(`Example app listening on port ${port}!`)
})